/* SPDX-License-Identifier: MIT
 * Independent FB80 check for the maintained FUNHEAD macro's local path.
 * No assembler, macro provider or object writer code is linked here.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FUNHEAD deck: %s\n", #x); exit(1); } } while (0)

static unsigned long be(const unsigned char *p, unsigned n)
{
    unsigned long v;
    v = 0;
    while (n--) v = v * 256UL + *p++;
    return v;
}

int main(int argc, char **argv)
{
    static const unsigned char name[8] = {0xd4,0xe8,0xe2,0xe5,0xc3,0x40,0x40,0x40};
    static const unsigned char code[18] = {
        0x47,0xf0,0xf0,0x0a, 0x05,0xd4,0xe8,0xe2,0xe5,0xc3,
        0x90,0xec,0xd0,0x0c, 0x18,0xcf, 0x07,0xfe
    };
    unsigned char card[80];
    FILE *f; size_t got; unsigned cards;
    CHECK(argc == 2);
    f = fopen(argv[1], "rb"); CHECK(f != NULL);
    cards = 0;
    while ((got = fread(card, 1, sizeof card, f)) != 0) {
        CHECK(got == sizeof card && card[0] == 2);
        if (cards == 0) {
            CHECK(card[1] == 0xc5 && card[2] == 0xe2 && card[3] == 0xc4);
            CHECK(be(card + 10, 2) == 16 && be(card + 14, 2) == 1);
            CHECK(card[24] == 4 && card[28] == 1 && be(card + 29, 3) == sizeof code);
        } else if (cards == 1) {
            CHECK(card[1] == 0xc5 && card[2] == 0xe2 && card[3] == 0xc4);
            CHECK(be(card + 10, 2) == 16 && !memcmp(card + 16, name, sizeof name));
            CHECK(card[24] == 1 && be(card + 25, 3) == 0 && be(card + 30, 2) == 1);
        } else if (cards == 2) {
            CHECK(card[1] == 0xe3 && card[2] == 0xe7 && card[3] == 0xe3);
            CHECK(be(card + 5, 3) == 0 && be(card + 10, 2) == sizeof code);
            CHECK(be(card + 14, 2) == 1 && !memcmp(card + 16, code, sizeof code));
        } else if (cards == 3) {
            CHECK(card[1] == 0xc5 && card[2] == 0xd5 && card[3] == 0xc4);
            CHECK(be(card + 5, 3) == 0 && be(card + 14, 2) == 1);
        } else CHECK(0);
        ++cards;
    }
    CHECK(!ferror(f) && fclose(f) == 0 && cards == 4);
    puts("Maintained FUNHEAD deck: entry, branch, identifier and linkage bytes PASS");
    return 0;
}
