/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Independent checker for the original macro fixture and separately supplied
 * LDVAL/LDADD/STVAL consumer; both use the same fixed instruction oracle.
 * No assembler, provider, conversion or writer helper is linked here.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "macro reference deck: %s\n", #x); exit(1); } } while (0)

static unsigned long be(const unsigned char *p, unsigned n)
{
    unsigned long v;
    v = 0;
    while (n--) v = v * 256UL + *p++;
    return v;
}

int main(int argc, char **argv)
{
    static const unsigned char name[8] = {
        0xd4,0xc1,0xc3,0xd9,0xc5,0xc6,0x40,0x40
    };
    static const unsigned char expected[28] = {
        0x58,0x40,0x10,0x10, 0x58,0x40,0x40,0x00,
        0x58,0x50,0x10,0x14, 0x58,0xe0,0x10,0x18,
        0x50,0x60,0xe0,0x00, 0x58,0x70,0x10,0x1c,
        0x50,0x60,0x70,0x00
    };
    unsigned char card[80], text[28], present[28];
    unsigned esd, ended, body, section, n, i, cards;
    unsigned long offset;
    size_t got;
    FILE *f;
    CHECK(argc == 2);
    f = fopen(argv[1], "rb"); CHECK(f != NULL);
    memset(text, 0, sizeof text); memset(present, 0, sizeof present);
    esd = ended = body = section = cards = 0;
    while ((got = fread(card, 1, sizeof card, f)) != 0) {
        ++cards; CHECK(got == sizeof card && card[0] == 2 && !ended);
        if (card[1] == 0xc5 && card[2] == 0xe2 && card[3] == 0xc4) {
            CHECK(!esd && !body && be(card + 10, 2) == 16);
            section = (unsigned)be(card + 14, 2); CHECK(section == 1);
            CHECK(!memcmp(card + 16, name, sizeof name));
            CHECK(card[24] == 0 && be(card + 25, 3) == 0);
            CHECK(card[28] == 1 && be(card + 29, 3) == sizeof expected);
            esd = 1;
        } else if (card[1] == 0xe3 && card[2] == 0xe7 && card[3] == 0xe3) {
            CHECK(esd && be(card + 14, 2) == section);
            body = 1; n = (unsigned)be(card + 10, 2);
            offset = be(card + 5, 3);
            CHECK(n && n <= 56 && offset <= sizeof text && n <= sizeof text - offset);
            for (i = 0; i < n; ++i) {
                CHECK(!present[offset + i]);
                present[offset + i] = 1; text[offset + i] = card[16 + i];
            }
        } else {
            CHECK(card[1] == 0xc5 && card[2] == 0xd5 && card[3] == 0xc4);
            CHECK(esd && body && be(card + 14, 2) == section && be(card + 5, 3) == 0);
            ended = 1;
        }
    }
    CHECK(!ferror(f)); CHECK(fclose(f) == 0); CHECK(esd && body && ended && cards == 3);
    for (i = 0; i < sizeof text; ++i) { CHECK(present[i]); CHECK(text[i] == expected[i]); }
    printf("Macro reference deck: %u independent checks (28 bytes, no relocations)\n", checks);
    return 0;
}
