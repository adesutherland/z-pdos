/* SPDX-License-Identifier: MIT
 * Independent PDPROBE layout, instruction, linkage and classic-record checks.
 * Expected values are calculated from public formats, not encoder helpers.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"classic-call:%d: %s\n",__LINE__,#x); return 1; } } while (0)
static unsigned long be(const unsigned char *p, unsigned n)
{ unsigned long v; v = 0; while (n--) v = v * 256UL + *p++; return v; }
int main(int argc, char **argv)
{
    static const unsigned char expected[92] = {
        0x47,0xf0,0xf0,0x0c,7,0xd7,0xc4,0xd7,0xd9,0xd6,0xc2,0xc5,
        0x90,0xec,0xd0,0x0c,0x18,0xcf,0x58,0xf0,0xd0,0x4c,
        0x50,0xd0,0xf0,4,0x50,0xf0,0xd0,8,0x18,0xdf,
        0x41,0xf0,0xf0,0x58,0x50,0xf0,0xd0,0x4c,0x47,0xf0,0xc0,0x2c,
        5,0xc0,0x18,0xb1,0x58,0xa0,0xc0,0x22,0x58,0xf0,0xb0,0,
        0x5a,0xf0,0xc0,0x26,0x58,0xd0,0xd0,4,0x58,0xe0,0xd0,0x0c,
        0x98,0x0c,0xd0,0x14,7,0xfe,0,0,0,0,0,0,
        0,0,0,88,0,0,0,1,0,0,0,46
    };
    static const unsigned char exported[8] = {0xd7,0xc4,0xd7,0xd9,0xd6,0xc2,0xc5,0x40};
    FILE *f; unsigned char card[80], bytes[92], seen[92];
    unsigned sd, ld, end, reloc80, reloc88; size_t n, i, at, length;
    CHECK(argc == 2); f = fopen(argv[1], "rb"); CHECK(f != NULL);
    memset(bytes, 0, sizeof bytes); memset(seen, 0, sizeof seen);
    sd = ld = end = reloc80 = reloc88 = 0;
    while ((n = fread(card, 1, sizeof card, f)) != 0) {
        CHECK(n == 80 && card[0] == 2 && !end); length = (size_t)be(card + 10, 2);
        if (card[1] == 0xc5 && card[2] == 0xe2 && card[3] == 0xc4) {
            CHECK(length == 16);
            if (card[24] == 4) {
                CHECK(!sd++ && be(card + 14, 2) == 1);
                for (i = 16; i < 24; ++i) CHECK(card[i] == 0x40);
                CHECK(be(card + 25, 3) == 0 && card[28] == 7 && be(card + 29, 3) == 92);
            } else {
                CHECK(card[24] == 1 && !ld++ && !memcmp(card + 16, exported, 8));
                CHECK(be(card + 25, 3) == 0 && be(card + 29, 3) == 1);
            }
        } else if (card[1] == 0xe3 && card[2] == 0xe7 && card[3] == 0xe3) {
            at = (size_t)be(card + 5, 3);
            CHECK(length && length <= 56 && at <= 92 && length <= 92 - at);
            CHECK(be(card + 14, 2) == 1);
            for (i = 0; i < length; ++i) { CHECK(!seen[at+i]); seen[at+i] = 1; bytes[at+i] = card[16+i]; }
        } else if (card[1] == 0xd9 && card[2] == 0xd3 && card[3] == 0xc4) {
            CHECK(length == 8 && be(card + 16, 2) == 1 && be(card + 18, 2) == 1 && card[20] == 0x0c);
            at = (size_t)be(card + 21, 3);
            if (at == 80) CHECK(!reloc80++); else { CHECK(at == 88 && !reloc88++); }
        } else {
            CHECK(card[1] == 0xc5 && card[2] == 0xd5 && card[3] == 0xc4);
            CHECK(card[5] == 0x40 && card[6] == 0x40 && card[7] == 0x40);
            CHECK(card[14] == 0x40 && card[15] == 0x40); ++end;
        }
    }
    CHECK(!ferror(f) && fclose(f) == 0);
    CHECK(sd == 1 && ld == 1 && end == 1 && reloc80 == 1 && reloc88 == 1);
    for (i = 0; i < 92; ++i) {
        CHECK(seen[i] == (i < 74 || i >= 80)); CHECK(bytes[i] == expected[i]);
    }
    puts("PDPROBE: complete 92-byte layout, save/restore, result and two A relocations verified");
    return 0;
}
