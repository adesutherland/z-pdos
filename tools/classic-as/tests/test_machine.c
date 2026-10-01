/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Independent literal instruction bytes and operand-boundary controls.
 */
#include "mf_classic.h"
#define CHECK(x) do { if (!(x)) return __LINE__; } while (0)
static const mf_octet n_LR[] = {0x4c,0x52};
static const mf_octet n_AR[] = {0x41,0x52};
static const mf_octet n_BCR[] = {0x42,0x43,0x52};
static const mf_octet n_L[] = {0x4c};
static const mf_octet n_ST[] = {0x53,0x54};
static const mf_octet n_LA[] = {0x4c,0x41};
static const mf_octet n_LM[] = {0x4c,0x4d};
static const mf_octet n_SLDL[] = {0x53,0x4c,0x44,0x4c};
static const mf_octet n_MVI[] = {0x4d,0x56,0x49};
static const mf_octet n_MVC[] = {0x4d,0x56,0x43};
static const mf_octet n_CLC[] = {0x43,0x4c,0x43};
static const mf_octet n_PACK[] = {0x50,0x41,0x43,0x4b};
static const mf_octet n_MP[] = {0x4d,0x50};
static const mf_octet n_BASR[] = {0x42,0x41,0x53,0x52};
static const mf_octet n_CLM[] = {0x43,0x4c,0x4d};
static const mf_octet n_ICM[] = {0x49,0x43,0x4d};
static const mf_octet n_CS[] = {0x43,0x53};
static const mf_octet n_CDS[] = {0x43,0x44,0x53};
static const mf_octet n_SVC[] = {0x53,0x56,0x43};
static const mf_octet n_M[] = {0x4d};
static const mf_octet n_MR[] = {0x4d,0x52};
static const mf_octet n_MVCL[] = {0x4d,0x56,0x43,0x4c};
static const mf_octet n_ZZZ[] = {0x5a,0x5a,0x5a};
static struct mf_span span(const mf_octet *data, size_t length)
{
    struct mf_span s; s.data = data; s.length = length; return s;
}
static void zero(struct mf_operands *op)
{
    op->r1 = op->r2 = op->r3 = op->x2 = op->b1 = op->b2 = 0;
    op->d1 = op->d2 = op->immediate = 0;
    op->length1 = op->length2 = 0;
}
#define LOOKUP(n,p) mf_machine_lookup(p,span(n_##n,sizeof(n_##n)))
static int vector(enum mf_profile profile, const struct mf_instruction *insn,
                  struct mf_operands *op, const mf_octet *expected, unsigned n)
{
    mf_octet output[6];
    unsigned length, i;
    for (i = 0; i < 6; ++i) output[i] = 0x55;
    if (mf_encode(profile,insn,op,output,6,&length) != MF_OK || length != n)
        return 0;
    if (!mf_span_equal(span(output,n),span(expected,n))) return 0;
    if (n < 6 && output[n] != 0x55) return 0;
    return 1;
}
int main(void)
{
    static const mf_octet lr[] = {0x18,0xff};
    static const mf_octet ar[] = {0x1a,0x10};
    static const mf_octet branch[] = {0x07,0xfe};
    static const mf_octet load[] = {0x58,0xff,0xff,0xff};
    static const mf_octet store[] = {0x50,0x10,0x20,0x00};
    static const mf_octet address[] = {0x41,0x00,0x00,0x00};
    static const mf_octet lm[] = {0x98,0x0f,0xff,0xff};
    static const mf_octet shift[] = {0x8d,0xe0,0xff,0xff};
    static const mf_octet immediate[] = {0x92,0xff,0xff,0xff};
    static const mf_octet mvc[] = {0xd2,0xff,0xff,0xff,0x00,0x00};
    static const mf_octet clc[] = {0xd5,0x00,0x00,0x00,0xff,0xff};
    static const mf_octet pack[] = {0xf2,0xf0,0xff,0xff,0x00,0x00};
    static const mf_octet multiply[] = {0xfc,0xf7,0x00,0x00,0x00,0x00};
    static const mf_octet basr[] = {0x0d,0xff};
    static const mf_octet clm[] = {0xbd,0xff,0xff,0xff};
    static const mf_octet clmzero[] = {0xbd,0xf0,0x10,1};
    static const mf_octet icm[] = {0xbf,0xff,0xff,0xff};
    static const mf_octet cs[] = {0xba,0xfe,0xda,0xbc};
    static const mf_octet cds[] = {0xbb,0xec,0x00,0x00};
    static const mf_octet svc[] = {0x0a,0xff};
    static const mf_octet lower[] = {0x6c,0x72};
    struct mf_operands op;
    struct mf_instruction invalid;
    const struct mf_instruction *insn;
    mf_octet output[6];
    unsigned length;
    zero(&op); op.r1 = op.r2 = 15;
    CHECK(vector(MF_S360,LOOKUP(LR,MF_S360),&op,lr,2));
    CHECK(mf_machine_lookup(MF_S360,span(lower,2)) == LOOKUP(LR,MF_S360));
    op.r1 = 1; op.r2 = 0;
    CHECK(vector(MF_S360,LOOKUP(AR,MF_S360),&op,ar,2));
    op.r1 = 15; op.r2 = 14;
    CHECK(vector(MF_S360,LOOKUP(BCR,MF_S360),&op,branch,2));
    op.r1 = 16; output[0] = 0x55; length = 99;
    CHECK(mf_encode(MF_S360,LOOKUP(LR,MF_S360),&op,output,6,&length) == MF_RANGE);
    CHECK(length == 0 && output[0] == 0x55);
    zero(&op); op.r1 = op.x2 = op.b2 = 15; op.d2 = 4095;
    CHECK(vector(MF_S360,LOOKUP(L,MF_S360),&op,load,4));
    op.d2 = 4096;
    CHECK(mf_encode(MF_S360,LOOKUP(L,MF_S360),&op,output,6,&length) == MF_RANGE);
    op.d2 = 0xffffffffUL;
    CHECK(mf_encode(MF_S360,LOOKUP(L,MF_S360),&op,output,6,&length) == MF_RANGE);
    zero(&op); op.r1 = 1; op.b2 = 2;
    CHECK(vector(MF_S360,LOOKUP(ST,MF_S360),&op,store,4));
    zero(&op);
    CHECK(vector(MF_S360,LOOKUP(LA,MF_S360),&op,address,4));
    op.r3 = op.b2 = 15; op.d2 = 4095;
    CHECK(vector(MF_S360,LOOKUP(LM,MF_S360),&op,lm,4));
    op.r1 = 14; op.r3 = 0;
    CHECK(vector(MF_S360,LOOKUP(SLDL,MF_S360),&op,shift,4));
    op.r1 = 15;
    CHECK(mf_encode(MF_S360,LOOKUP(SLDL,MF_S360),&op,output,6,&length) == MF_RANGE);
    op.r1 = 14; op.r3 = 1;
    CHECK(mf_encode(MF_S360,LOOKUP(SLDL,MF_S360),&op,output,6,&length) == MF_RANGE);
    zero(&op); op.immediate = 255; op.b1 = 15; op.d1 = 4095;
    CHECK(vector(MF_S360,LOOKUP(MVI,MF_S360),&op,immediate,4));
    op.immediate = 256;
    CHECK(mf_encode(MF_S360,LOOKUP(MVI,MF_S360),&op,output,6,&length) == MF_RANGE);
    zero(&op); op.b1 = 15; op.d1 = 4095; op.length1 = 256;
    CHECK(vector(MF_S360,LOOKUP(MVC,MF_S360),&op,mvc,6));
    op.length1 = 0;
    CHECK(mf_encode(MF_S360,LOOKUP(MVC,MF_S360),&op,output,6,&length) == MF_RANGE);
    op.length1 = 257;
    CHECK(mf_encode(MF_S360,LOOKUP(MVC,MF_S360),&op,output,6,&length) == MF_RANGE);
    zero(&op); op.b2 = 15; op.d2 = 4095; op.length1 = 1;
    CHECK(vector(MF_S360,LOOKUP(CLC,MF_S360),&op,clc,6));
    zero(&op); op.b1 = 15; op.d1 = 4095; op.length1 = 16; op.length2 = 1;
    CHECK(vector(MF_S360,LOOKUP(PACK,MF_S360),&op,pack,6));
    op.length2 = 17;
    CHECK(mf_encode(MF_S360,LOOKUP(PACK,MF_S360),&op,output,6,&length) == MF_RANGE);
    zero(&op); op.length1 = 16; op.length2 = 8;
    CHECK(vector(MF_S360,LOOKUP(MP,MF_S360),&op,multiply,6));
    op.length2 = 9;
    CHECK(mf_encode(MF_S360,LOOKUP(MP,MF_S360),&op,output,6,&length) == MF_RANGE);
    op.length1 = op.length2 = 8;
    CHECK(mf_encode(MF_S360,LOOKUP(MP,MF_S360),&op,output,6,&length) == MF_RANGE);
    zero(&op); op.r1 = op.r2 = 15;
    CHECK(vector(MF_S370,LOOKUP(BASR,MF_S370),&op,basr,2));
    CHECK(mf_encode(MF_S360,LOOKUP(BASR,MF_S360),&op,output,6,&length) == MF_UNSUPPORTED);
    zero(&op); op.r1 = op.r3 = op.b2 = 15; op.d2 = 4095;
    CHECK(vector(MF_S370,LOOKUP(ICM,MF_S370),&op,icm,4));
    CHECK(vector(MF_S370,LOOKUP(CLM,MF_S370),&op,clm,4));
    CHECK(mf_encode(MF_S360,LOOKUP(CLM,MF_S360),&op,output,6,&length) == MF_UNSUPPORTED);
    op.r3 = 0; op.b2 = 1; op.d2 = 1;
    CHECK(vector(MF_S370,LOOKUP(CLM,MF_S370),&op,clmzero,4));
    op.r3 = 16;
    CHECK(mf_encode(MF_S370,LOOKUP(CLM,MF_S370),&op,output,6,&length) == MF_RANGE);
    op.r3 = 15; op.d2 = 4096;
    CHECK(mf_encode(MF_S370,LOOKUP(CLM,MF_S370),&op,output,6,&length) == MF_RANGE);
    op.d2 = 4095;
    op.r3 = 14; op.b2 = 13; op.d2 = 0xabc;
    CHECK(vector(MF_S370,LOOKUP(CS,MF_S370),&op,cs,4));
    zero(&op); op.r1 = 14; op.r3 = 12;
    CHECK(vector(MF_S370,LOOKUP(CDS,MF_S370),&op,cds,4));
    op.r3 = 13;
    CHECK(mf_encode(MF_S370,LOOKUP(CDS,MF_S370),&op,output,6,&length) == MF_RANGE);
    zero(&op); op.immediate = 255;
    CHECK(vector(MF_S360,LOOKUP(SVC,MF_S360),&op,svc,2));
    op.immediate = 256;
    CHECK(mf_encode(MF_S360,LOOKUP(SVC,MF_S360),&op,output,6,&length) == MF_RANGE);
    zero(&op); op.r1 = 1;
    CHECK(mf_encode(MF_S360,LOOKUP(MR,MF_S360),&op,output,6,&length) == MF_RANGE);
    CHECK(mf_encode(MF_S360,LOOKUP(M,MF_S360),&op,output,6,&length) == MF_RANGE);
    CHECK(mf_encode(MF_S370,LOOKUP(MVCL,MF_S370),&op,output,6,&length) == MF_RANGE);
    zero(&op); output[0] = 0x55;
    CHECK(mf_encode(MF_S360,LOOKUP(L,MF_S360),&op,output,3,&length) == MF_LIMIT);
    CHECK(length == 0 && output[0] == 0x55);
    insn = LOOKUP(LR,MF_S360); invalid = *insn; invalid.opcode = 0xff;
    CHECK(mf_encode(MF_S360,&invalid,&op,output,6,&length) == MF_UNSUPPORTED);
    CHECK(mf_encode((enum mf_profile)99,insn,&op,output,6,&length) == MF_UNSUPPORTED);
    CHECK(!LOOKUP(ZZZ,MF_S360));
    return 0;
}
