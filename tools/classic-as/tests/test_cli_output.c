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
static void storage_requests(const char *path)
{
    static const unsigned long expected[4][4] = {
        {120,128,0,0x512}, {120,513,0,0x30},
        {120,513,0x8000,0x503}, {10,0x03000040,0x123450,0}
    };
    unsigned char bytes[512]; unsigned long reg[16], address, value;
    size_t size, pc; unsigned step, calls, op, r, second, base; FILE *input;
    input = fopen(path,"rb"); CHECK(input != NULL);
    size = fread(bytes,1,sizeof bytes,input); CHECK(size && size < sizeof bytes);
    CHECK(!ferror(input) && fclose(input) == 0);
    memset(reg,0,sizeof reg); reg[6] = 513; reg[10] = 0x123450;
    reg[12] = 4096; reg[14] = 0x445566; pc = calls = 0;
    for (step = 0; step < 64; ++step) {
        CHECK(pc+2 <= size); op = bytes[pc]; r = bytes[pc+1] >> 4; second = bytes[pc+1] & 15;
        if (op == 0x07) { CHECK(r == 15 && second == 14); break; }
        if (op == 0x18 || op == 0x1b) {
            reg[r] = op == 0x18 ? reg[second] : (reg[r]-reg[second]) & 0xffffffffUL;
            pc += 2; continue;
        }
        if (op == 0x0a) {
            CHECK(calls < 4 && bytes[pc+1] == expected[calls][0]);
            CHECK(reg[0] == expected[calls][1] && reg[1] == expected[calls][2] && reg[15] == expected[calls][3]);
            CHECK(reg[6] == 513 && reg[10] == 0x123450 && reg[12] == 4096 && reg[14] == 0x445566);
            if (calls < 2) reg[1] = 0x8000;
            reg[15] = 0; ++calls; pc += 2; continue;
        }
        CHECK(pc+4 <= size && second == 0);
        base = bytes[pc+2] >> 4;
        address = (base ? reg[base] : 0) + ((bytes[pc+2] & 15)*256UL+bytes[pc+3]);
        CHECK(address >= 4096 && address-4096 < size);
        if (op == 0x47) { CHECK(r == 15); pc = (size_t)(address-4096); continue; }
        CHECK(address-4096+4 <= size); value = be(bytes+address-4096,4);
        if (op == 0x58) reg[r] = value;
        else if (op == 0x54) reg[r] &= value;
        else { CHECK(op == 0x56); reg[r] |= value; }
        pc += 4;
    }
    CHECK(step < 64 && calls == 4);
    puts("Storage interfaces: four independently expected SVC register requests pass");
}
static void io_templates(const char *path)
{
    static const unsigned char names[3][8] = {
        {0xc9,0xd5,0xd7,0xe4,0xe3,0x40,0x40,0x40},
        {0xd6,0xe4,0xe3,0xd7,0xe4,0xe3,0x40,0x40},
        {0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40}
    };
    unsigned char expected[308], actual[309]; size_t got, at; unsigned i; FILE *input;
    memset(expected,0,sizeof expected); expected[0] = expected[8] = 0x80; expected[4] = 0x8f;
    for (i = 0; i < 3; ++i) {
        at = 12+i*96; expected[at+26] = 0x40; memcpy(expected+at+40,names[i],8);
    }
    expected[46] = expected[50] = 0x11; expected[47] = 0x2c; expected[51] = 0x30;
    expected[62] = 0x48; expected[159] = 0x50; expected[255] = 0x48;
    expected[300] = 7; expected[301] = 0xfe;
    input = fopen(path,"rb"); CHECK(input != NULL); got = fread(actual,1,sizeof actual,input);
    CHECK(got == sizeof expected && !memcmp(actual,expected,sizeof expected));
    CHECK(!ferror(input) && fclose(input) == 0);
    puts("I/O templates: all 308 independent field, reserved and relocation bytes pass");
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
    if (!strcmp(argv[1],"pdos31-io")) { io_templates(argv[2]); return 0; }
    if (!strcmp(argv[1],"pdos31-get")) {
        static const unsigned char linked[20] = {0x18,0x12,0x58,0xf0,0x10,0x30,0x54,0xf0,0xc0,0x10,5,0xef,7,0xfe,0,0,0,0xff,0xff,0xff};
        input = fopen(argv[2],"rb"); CHECK(input != NULL); got = fread(card,1,sizeof card,input);
        CHECK(got == sizeof linked && !memcmp(card,linked,sizeof linked));
        CHECK(!ferror(input) && fclose(input) == 0); puts("GET: all 20 independent linkage bytes pass"); return 0;
    }
    if (!strcmp(argv[1],"pdos31-storage")) { storage_requests(argv[2]); return 0; }
    if (!strcmp(argv[1], "pdos31-maps")) {
        static const unsigned char linked[64] = {0,32,0,64,0,96,0,104,0,112,0,120,1,0,1,128,2,28,2,32,2,36,2,128,0,32,0,16,0,4,0,0,0,8,0,12,0,16,1,8,0,13,0,8,0,8,0,16,0,108,0,20,1,48,0,16,0,0,0,4,0,5,0,6};
        input = fopen(argv[2], "rb"); CHECK(input != NULL);
        got = fread(card,1,sizeof card,input);
        CHECK(got == sizeof linked && !memcmp(card,linked,sizeof linked));
        CHECK(!ferror(input) && fclose(input) == 0);
        puts("PDOS31 mappings: 32 independent field/length values pass"); return 0;
    }
    if (!strcmp(argv[1], "call")) {
        static const unsigned char linked[12] = {0x58,0xf0,0xc0,8,5,0xef,7,0xfe,0,0,0x10,6};
        input = fopen(argv[2], "rb"); CHECK(input != NULL);
        got = fread(card,1,sizeof card,input);
        CHECK(got == sizeof linked && !memcmp(card,linked,sizeof linked));
        CHECK(!ferror(input) && fclose(input) == 0);
        puts("CALL: all 12 linked bytes and V target pass"); return 0;
    }
    if (!strcmp(argv[1], "channel-word")) {
        static const unsigned char linked[30] = {0xaa,0,0,0,0,0,0,0,7,0x40,0,6,0,0,0x10,0x18,0x1d,0,0x7f,0xff,0,0,0,0,0,0,0,0,0,0};
        input = fopen(argv[2], "rb"); CHECK(input != NULL);
        got = fread(card,1,sizeof card,input);
        CHECK(got == sizeof linked && !memcmp(card,linked,sizeof linked));
        CHECK(!ferror(input) && fclose(input) == 0);
        puts("CCW1: all 30 linked bytes, zero alignment and relocated address pass"); return 0;
    }
    if (!strcmp(argv[1], "overlays")) {
        static const unsigned char linked[10] = {0,0x11,0xab,0xcd,0x44,0x55,0x66,0x77,0x18,0x12};
        input = fopen(argv[2], "rb"); CHECK(input != NULL);
        got = fread(card,1,sizeof card,input);
        CHECK(got == sizeof linked && !memcmp(card,linked,sizeof linked));
        CHECK(!ferror(input) && fclose(input) == 0);
        puts("overlays: later TXT bytes and section high water pass independent link check"); return 0;
    }
    if (!strcmp(argv[1], "fullword-bits")) {
        static const unsigned char words[28] = {0,0,0,0,0x7f,0xff,0xff,0xff,
            0x80,0,0,0,0xff,0xff,0xff,0xff,0xa8,0x88,0x5a,0x31,
            0x12,0x34,0x56,0x78,0x87,0x65,0x43,0x21};
        input = fopen(argv[2], "rb"); CHECK(input != NULL);
        got = fread(card, 1, sizeof card, input);
        CHECK(got == sizeof words && !memcmp(card, words, sizeof words));
        CHECK(!ferror(input) && fclose(input) == 0);
        puts("fullword bits: all 28 independently expected bytes passed"); return 0;
    }
    if (!strcmp(argv[1], "local-v")) {
        static const unsigned char linked[14] = {0,0,0x10,0x0c,0,0,0x10,0x0c,0,0,0x10,0,7,0xfe};
        input = fopen(argv[2], "rb"); CHECK(input != NULL);
        got = fread(card, 1, sizeof card, input);
        CHECK(got == sizeof linked && !memcmp(card, linked, sizeof linked));
        CHECK(!ferror(input) && fclose(input) == 0);
        puts("local V: independent entry/section self-reference link bytes passed"); return 0;
    }
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
        CHECK(!strcmp(argv[1], "profile") || !strcmp(argv[1], "registers"));
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
