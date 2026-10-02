/* SPDX-License-Identifier: MIT
 * Independent IBM architectural field vectors; no implementation table input. */
#include "mf_classic.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"system:%d: %s\n",__LINE__,#x); return 1; } } while (0)
int main(void)
{
    static const struct vector { const char *name; mf_octet bytes[6]; unsigned length; enum mf_profile minimum; } v[] = {
        {"LPSW",{0x82,0,0xd1,0x23},4,MF_S360}, {"SIO",{0x9c,0,0xd1,0x23},4,MF_S360},
        {"LCTL",{0xb7,0x6f,0xd1,0x23},4,MF_S370}, {"STCTL",{0xb6,0x6f,0xd1,0x23},4,MF_S370},
        {"SIGP",{0xae,0x6f,0xd1,0x23},4,MF_S370}, {"STCK",{0xb2,5,0xd1,0x23},4,MF_S370},
        {"STNSM",{0xac,0xfb,0xd1,0x23},4,MF_S370}, {"STOSM",{0xad,0xfb,0xd1,0x23},4,MF_S370},
        {"BSM",{0x0b,0x6f},2,MF_ESA390}, {"MSCH",{0xb2,0x32,0xd1,0x23},4,MF_ESA390},
        {"SSCH",{0xb2,0x33,0xd1,0x23},4,MF_ESA390}, {"STSCH",{0xb2,0x34,0xd1,0x23},4,MF_ESA390},
        {"TSCH",{0xb2,0x35,0xd1,0x23},4,MF_ESA390}, {"PR",{1,1},2,MF_ESA390},
        {"LPSWE",{0xb2,0xb2,0xd1,0x23},4,MF_Z900},
        {"STMG",{0xeb,0x6f,0xd1,0x23,0x45,0x24},6,MF_Z900},
        {"LMG",{0xeb,0x6f,0xd1,0x23,0x45,4},6,MF_Z900},
        {"BRCL",{0xc0,0x64,0xff,0xff,0xff,0xfe},6,MF_Z900},
        {"LARL",{0xc0,0x60,0xff,0xff,0xff,0xfe},6,MF_Z900}
    };
    struct mf_span name; struct mf_operands op; const struct mf_instruction *ins;
    mf_octet bytes[6], mnemonic[8]; unsigned length; size_t i,j,k;
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (i = 0; i < sizeof v / sizeof v[0]; ++i) {
        name.length = strlen(v[i].name); name.data = mnemonic;
        for (j = 0; j < name.length; ++j) {
            for (k = 0; k < 26 && alphabet[k] != v[i].name[j]; ++k) { }
            CHECK(k < 26); mnemonic[j] = (mf_octet)(0x41 + k);
        }
        ins = mf_machine_lookup(v[i].minimum,name); CHECK(ins != NULL);
        memset(&op,0,sizeof op);
        if (ins->format != MF_E) { op.b2 = 13; op.d2 = 0x123; }
        if (ins->format == MF_RR) { op.r1 = 6; op.r2 = 15; }
        if (ins->format == MF_RS || ins->format == MF_RSY) { op.r1 = 6; op.r3 = 15; }
        if (ins->format == MF_RSY) op.d2 = 0x45123;
        if (ins->format == MF_SI) { op.b1 = 13; op.d1 = 0x123; op.immediate = 0xfb; }
        if (ins->format == MF_RIL) { op.r1 = 6; op.immediate = 0xfffffffeUL; }
        CHECK(mf_encode(v[i].minimum,ins,&op,bytes,sizeof bytes,&length) == MF_OK);
        CHECK(length == v[i].length && !memcmp(bytes,v[i].bytes,length));
        if (v[i].minimum > MF_S360) CHECK(mf_encode((enum mf_profile)(v[i].minimum-1),ins,&op,bytes,sizeof bytes,&length) == MF_UNSUPPORTED);
        if (i == 1) CHECK(mf_encode(MF_Z900,ins,&op,bytes,sizeof bytes,&length) == MF_UNSUPPORTED);
        bytes[0] = 0x55;
        CHECK(mf_encode(v[i].minimum,ins,&op,bytes,v[i].length-1,&length) == MF_LIMIT);
        CHECK(!length && bytes[0] == 0x55);
        if (ins->format == MF_S || ins->format == MF_RSY) {
            op.d2 = ins->format == MF_S ? 4096 : 0x100000;
            CHECK(mf_encode(v[i].minimum,ins,&op,bytes,sizeof bytes,&length) == MF_RANGE);
        }
        if (ins->format == MF_E) {
            op.r1 = 1; CHECK(mf_encode(v[i].minimum,ins,&op,bytes,sizeof bytes,&length) == MF_RANGE);
        }
    }
    puts("system: 19 independent encodings, profile exclusions and field limits pass"); return 0;
}
