/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Independent scanner for the constants/forms/profile CLI fixtures.
 * No assembler library, value utility or character conversion is used here.
 * Classic OBJSTMT fields follow IBM z/VM 7.4 OBJSTMT documentation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
static unsigned long card_count;
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "CLI deck card %lu: %s\n", card_count, #x); \
    exit(1); } } while (0)

static unsigned long be(const unsigned char *p, unsigned n)
{
    unsigned long result;
    result = 0;
    while (n--) result = result * 256UL + *p++;
    return result;
}

int main(int argc, char **argv)
{
    static const unsigned char constants[40] = {
        0x7f,0,0x80,0x00,0x7f,0xff,0,0,
        0x80,0x00,0x00,0x00,0x7f,0xff,0xff,0xff,
        0x00,0x00,0x00,0x00,0x00,0x0f,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0x00,0x0f,0xff,0xff,0xc1,0xc2,0xc3,0xc4
    };
    static const unsigned char forms[40] = {
        0x18,0x12,0x1a,0x34,0x1b,0x56,0x07,0xfe,
        0x58,0x12,0x3f,0xff,0x50,0x40,0x50,0x00,
        0x89,0x60,0x00,0x1f,0x95,0xff,0x80,0x00,
        0x91,0x80,0x9f,0xff,0xd2,0x00,0xa0,0x00,
        0xb0,0x00,0xd5,0xff,0xcf,0xff,0xdf,0xff
    };
    static const unsigned char profile[2] = {0x0d,0xef};
    static const unsigned char names[3][8] = {
        {0xc3,0xd6,0xd5,0xe2,0xe3,0xe2,0x40,0x40},
        {0xc6,0xd6,0xd9,0xd4,0xe2,0x40,0x40,0x40},
        {0xd7,0xd9,0xd6,0xc6,0xc9,0xd3,0xc5,0x40}
    };
    const unsigned char *expected;
    unsigned char card[80], text[40], present[40];
    unsigned fixture, section_id, esd_seen, ended, body, i, length, size;
    unsigned long offset;
    size_t got;
    FILE *input;

    CHECK(argc == 3);
    if (!strcmp(argv[1], "sentinel")) {
        static const unsigned char sentinel[] = {
            0x6b,0x65,0x65,0x70,0x00,0xff,0x75,0x6e,0x63,0x68,0x61,0x6e,0x67,0x65,0x64
        };
        input = fopen(argv[2], "rb"); CHECK(input != NULL);
        got = fread(card, 1, sizeof card, input);
        CHECK(got == sizeof sentinel && !memcmp(card, sentinel, sizeof sentinel));
        CHECK(!ferror(input)); CHECK(fclose(input) == 0);
        printf("CLI preservation: %u independent checks passed\n", checks);
        return 0;
    }
    if (!strcmp(argv[1], "absent")) {
        input = fopen(argv[2], "rb");
        CHECK(input == NULL);
        printf("CLI absence: %u independent checks passed\n", checks);
        return 0;
    }
    if (!strcmp(argv[1], "constants")) {
        fixture = 0; expected = constants; length = sizeof constants;
    } else if (!strcmp(argv[1], "forms")) {
        fixture = 1; expected = forms; length = sizeof forms;
    } else {
        CHECK(!strcmp(argv[1], "profile"));
        fixture = 2; expected = profile; length = sizeof profile;
    }
    input = fopen(argv[2], "rb"); CHECK(input != NULL);
    memset(text, 0, sizeof text); memset(present, 0, sizeof present);
    esd_seen = ended = body = section_id = 0;
    while ((got = fread(card, 1, sizeof card, input)) != 0) {
        ++card_count;
        CHECK(got == sizeof card && card[0] == 2 && !ended);
        if (card[1] == 0xc5 && card[2] == 0xe2 && card[3] == 0xc4) {
            CHECK(!esd_seen && !body);
            CHECK(be(card + 10, 2) == 16);
            section_id = (unsigned)be(card + 14, 2);
            CHECK(section_id != 0);
            CHECK(!memcmp(card + 16, names[fixture], 8));
            CHECK(card[24] == 0 && be(card + 25, 3) == 0);
            CHECK(card[28] == 1 && be(card + 29, 3) == length);
            esd_seen = 1;
        } else if (card[1] == 0xe3 && card[2] == 0xe7 && card[3] == 0xe3) {
            CHECK(esd_seen);
            body = 1; size = (unsigned)be(card + 10, 2);
            CHECK(size > 0 && size <= 56 && be(card + 14, 2) == section_id);
            offset = be(card + 5, 3);
            CHECK(offset <= length && size <= length - offset);
            for (i = 0; i < size; ++i) {
                CHECK(!present[offset + i]);
                present[offset + i] = 1; text[offset + i] = card[16 + i];
            }
        } else {
            CHECK(card[1] == 0xc5 && card[2] == 0xd5 && card[3] == 0xc4);
            CHECK(esd_seen && be(card + 14, 2) == section_id && be(card + 5, 3) == 0);
            ended = 1;
        }
    }
    CHECK(!ferror(input)); CHECK(fclose(input) == 0);
    CHECK(esd_seen && body && ended);
    for (i = 0; i < length; ++i) {
        if (fixture == 0 && (i == 1 || i == 6 || i == 7)) CHECK(!present[i]);
        else { CHECK(present[i]); CHECK(text[i] == expected[i]); }
    }
    printf("CLI output: %u independent checks passed (%lu cards)\n", checks, card_count);
    return 0;
}
