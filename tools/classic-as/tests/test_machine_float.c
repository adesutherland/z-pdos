/* SPDX-License-Identifier: MIT
 * Independent reached HFP bytes and historical register restrictions. */
#include "mf_classic.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"HFP:%d: %s\n",__LINE__,#x); return 1; } } while (0)
int main(void)
{
    static const struct vector { const char *name; mf_octet expected[4]; unsigned length; } v[] = {
        {"LPDR",{0x20,0x24,0,0},2}, {"LTDR",{0x22,0x24,0,0},2}, {"LCDR",{0x23,0x24,0,0},2},
        {"LDR",{0x28,0x24,0,0},2}, {"CDR",{0x29,0x24,0,0},2}, {"ADR",{0x2a,0x24,0,0},2},
        {"SDR",{0x2b,0x24,0,0},2}, {"MDR",{0x2c,0x24,0,0},2}, {"DDR",{0x2d,0x24,0,0},2},
        {"LRER",{0x35,0x24,0,0},2}, {"STD",{0x60,0x29,0xc1,0x23},4}, {"LD",{0x68,0x29,0xc1,0x23},4},
        {"CD",{0x69,0x29,0xc1,0x23},4}, {"AD",{0x6a,0x29,0xc1,0x23},4}, {"SD",{0x6b,0x29,0xc1,0x23},4},
        {"MD",{0x6c,0x29,0xc1,0x23},4}, {"DD",{0x6d,0x29,0xc1,0x23},4}, {"STE",{0x70,0x29,0xc1,0x23},4},
        {"LE",{0x78,0x29,0xc1,0x23},4}, {"AE",{0x7a,0x29,0xc1,0x23},4}
    };
    struct mf_span name; struct mf_operands op; const struct mf_instruction *instruction;
    mf_octet bytes[6]; unsigned length; size_t i;
    for (i = 0; i < sizeof v / sizeof v[0]; ++i) {
        name.data = (const mf_octet *)v[i].name; name.length = strlen(v[i].name);
        instruction = mf_machine_lookup(MF_S370, name); CHECK(instruction != NULL);
        memset(&op,0,sizeof op); op.r1 = 2; op.r2 = 4; op.x2 = 9; op.b2 = 12; op.d2 = 0x123;
        CHECK(mf_encode(MF_S370,instruction,&op,bytes,sizeof bytes,&length) == MF_OK);
        CHECK(length == v[i].length && !memcmp(bytes,v[i].expected,length));
        op.r1 = 3; length = 99; bytes[0] = 0x55;
        CHECK(mf_encode(MF_S370,instruction,&op,bytes,sizeof bytes,&length) == MF_RANGE);
        CHECK(!length && bytes[0] == 0x55);
        op.r1 = 8;
        CHECK(mf_encode(MF_S370,instruction,&op,bytes,sizeof bytes,&length) == MF_RANGE);
        op.r1 = 2;
        if (v[i].length == 2) {
            op.r2 = 1; CHECK(mf_encode(MF_S370,instruction,&op,bytes,sizeof bytes,&length) == MF_RANGE);
        } else {
            op.d2 = 4096; CHECK(mf_encode(MF_S370,instruction,&op,bytes,sizeof bytes,&length) == MF_RANGE);
        }
    }
    name.data = (const mf_octet *)"LRER"; name.length = 4;
    instruction = mf_machine_lookup(MF_S360,name); memset(&op,0,sizeof op);
    CHECK(mf_encode(MF_S360,instruction,&op,bytes,sizeof bytes,&length) == MF_UNSUPPORTED);
    puts("HFP: reached instruction bytes and register boundaries passed"); return 0;
}
