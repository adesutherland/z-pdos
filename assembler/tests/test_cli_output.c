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
static void pool_release(const char *path)
{
    static const unsigned char expected[66] = {
        0x18,0x1a,0x18,0xf1,0x1b,0x11,0xbf,0x17,0xf0,21,
        0x47,0x80,0xc0,56,0x91,1,0xf0,23,0x47,0x70,0xc0,56,
        0x91,1,0xf0,32,0x47,0x80,0xc0,36,0x41,0x10,0,1,0x0a,13,
        0x1b,0,0x43,0,0x10,5,0x4c,0,0x10,6,0x4a,0,0xc0,64,
        0x92,1,0xf0,23,0x0a,10,7,0xfe,0,0,0,0,0,0,0,8
    };
    unsigned char actual[67], memory[16384]; unsigned long reg[16], address, pool;
    unsigned mode, step, op, r, second, base, cc, calls, svc, i; size_t pc, got;
    FILE *input;
    input = fopen(path,"rb"); CHECK(input != NULL);
    got = fread(actual,1,sizeof actual,input);
    CHECK(got == sizeof expected && !memcmp(actual,expected,sizeof expected));
    CHECK(!ferror(input) && fclose(input) == 0);
    for (mode = 0; mode < 4; ++mode) {
        memset(memory,0,sizeof memory); memcpy(memory+4096,actual,got);
        for (i = 0; i < 16; ++i) reg[i] = 0x8000+i;
        reg[10] = 0x2000; reg[12] = 4096; pc = 4096; cc = calls = svc = 0;
        pool = mode == 1 ? 0 : mode == 2 ? 0x2201 : 0x2200;
        memory[0x2015] = (unsigned char)(pool >> 16);
        memory[0x2016] = (unsigned char)(pool >> 8); memory[0x2017] = (unsigned char)pool;
        memory[0x2205] = 3; memory[0x2207] = 24;
        if (mode == 3) memory[0x2020] = 1;
        for (step = 0; step < 32; ++step) {
            CHECK(pc >= 4096 && pc+2 <= 4096+got);
            op = memory[pc]; r = memory[pc+1] >> 4; second = memory[pc+1] & 15;
            if (op == 7) { CHECK(r == 15 && second == 14); break; }
            if (op == 0x18 || op == 0x1b) {
                reg[r] = op == 0x18 ? reg[second] : (reg[r]-reg[second]) & 0xffffffffUL;
                if (op == 0x1b) cc = reg[r] ? 2 : 0;
                pc += 2; continue;
            }
            if (op == 0x0a) {
                ++calls; svc = memory[pc+1];
                if (mode == 3) { CHECK(svc == 13 && reg[1] == 1); break; }
                CHECK(mode == 0 && svc == 10 && reg[0] == 80 && reg[1] == 0x2200);
                CHECK(memory[0x2017] == 1); pc += 2; continue;
            }
            CHECK(pc+4 <= 4096+got); base = memory[pc+2] >> 4;
            address = (base ? reg[base] : 0)+((memory[pc+2]&15)*256UL+memory[pc+3]);
            if (op == 0x41) reg[r] = address;
            else if (op == 0x47) {
                if (r & (8 >> cc)) { pc = (size_t)address; continue; }
            } else {
                CHECK(address+3 < sizeof memory);
                if (op == 0xbf) { CHECK(second == 7); reg[r] = be(memory+address,3); cc = reg[r] ? 2 : 0; }
                else if (op == 0x91) { i = memory[address] & memory[pc+1]; cc = !i ? 0 : i == memory[pc+1] ? 3 : 1; }
                else if (op == 0x43) reg[r] = (reg[r]&0xffffff00UL) | memory[address];
                else if (op == 0x4c) reg[r] *= be(memory+address,2);
                else if (op == 0x4a) reg[r] += be(memory+address,2);
                else { CHECK(op == 0x92); memory[address] = memory[pc+1]; }
            }
            pc += 4;
        }
        CHECK(step < 32 && calls == (mode == 0 || mode == 3 ? 1 : 0));
        for (i = 2; i < 15; ++i) CHECK(reg[i] == (i == 10 ? 0x2000 : i == 12 ? 4096 : 0x8000+i));
    }
    puts("FREEPOOL: independent bytes and pool/null/invalid/alignment request controls pass");
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

static void runtime_maps(const char *path)
{
    /* Literal public/PDOS coordinates, independent of macro output. */
    static const unsigned expected[] = {
        21, 26, 29, 33, 36, 37, 40, 40, 44, 44, 48, 50,
        52, 62, 82, 16, 68, 3, 8, 2, 44, 60, 82, 84,
        86, 88, 94, 98, 140, 62, 74, 105, 10, 17, 18, 28,
        6, 24, 0, 1, 4, 12, 17, 8, 3, 4, 5, 16,
        25, 32, 14, 11, 0, 44, 52, 86, 98, 98, 99, 102,
        106, 118, 174, 30, 64, 2, 1, 8, 64, 16, 4, 16,
        32, 128, 8, 128, 1, 2, 1, 2, 3, 4, 16, 28,
        85, 1, 2, 192, 32, 32, 1, 16, 8, 128, 64, 192,
        224, 32, 16, 8, 128, 64, 8, 1, 64, 2, 8, 128,
        64, 16, 1};
    unsigned char actual[512]; size_t i, got; FILE *input;
    input = fopen(path,"rb"); CHECK(input != NULL);
    got = fread(actual,1,sizeof actual,input);
    CHECK(got == 4+2*(sizeof expected/sizeof expected[0]));
    CHECK(actual[0] == 0x0a && actual[1] == 99 && actual[2] == 7 && actual[3] == 0xfe);
    for (i = 0; i < sizeof expected/sizeof expected[0]; ++i)
        CHECK(be(actual+4+2*i,2) == expected[i]);
    CHECK(!ferror(input) && fclose(input) == 0);
    puts("runtime maps: independent relative coordinates, lengths, flags and SVC99 pass");
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
    if (!strcmp(argv[1],"pdos31-task")) {
        static const unsigned char code[26] = {
            0x41,0xf0,0xc0,0,0x18,2,0x50,0,0xf0,0,0x18,3,0x50,0,0xf0,8,
            0x0a,0x2a,0x41,0x10,0xc0,0x44,0x0a,0x3e,7,0xfe};
        static const unsigned char name[8] = {0xd7,0xd9,0xd6,0xc7,0x40,0x40,0x40,0x40};
        unsigned char actual[73], expected[72];
        memset(expected,0,sizeof expected);
        expected[2] = expected[10] = 0x10; expected[3] = 0x38; expected[11] = 0x40;
        expected[19] = 78; expected[27] = 8;
        memcpy(expected+28,code,sizeof code); memcpy(expected+56,name,sizeof name);
        input = fopen(argv[2],"rb"); CHECK(input != NULL); got = fread(actual,1,sizeof actual,input);
        CHECK(got == sizeof expected && !memcmp(actual,expected,sizeof expected));
        CHECK(!ferror(input) && fclose(input) == 0);
        puts("PDOS task adapter: independent 28-byte list and SVC linkage pass"); return 0;
    }
    if (!strcmp(argv[1],"pdos31-sync")) {
        static const unsigned char linked[44] = {
            0x18,0x15,0x0a,0,0x18,0x12,0x41,0,0,1,0x0a,1,0x18,0x1a,0x1b,0,0x0a,0x37,
            0x18,0x18,0x58,0xf0,0x10,8,0x54,0xf0,0xc0,0x28,0x58,0xf0,0xf0,0x34,
            0x54,0xf0,0xc0,0x28,5,0xef,7,0xfe,0,0xff,0xff,0xff};
        unsigned char actual[45];
        input = fopen(argv[2],"rb"); CHECK(input != NULL); got = fread(actual,1,sizeof actual,input);
        CHECK(got == sizeof linked && !memcmp(actual,linked,sizeof linked));
        CHECK(!ferror(input) && fclose(input) == 0); puts("classic synchronization: register and entry bytes pass"); return 0;
    }
    if (!strcmp(argv[1],"pdos31-basic-dcb")) {
        static const unsigned char names[32] = {
            0xc2,0xe2,0xc1,0xd4,0x40,0x40,0x40,0x40,
            0xc2,0xd7,0xc1,0xd4,0x40,0x40,0x40,0x40,
            0xc5,0xe7,0xc3,0xd7,0x40,0x40,0x40,0x40,
            0xe2,0xe8,0xe2,0xe3,0xc5,0xd9,0xd4,0x40};
        unsigned char actual[337], expected[336];
        size_t i, j;
        static const size_t sentinel[] = {23,31,55,59,71,75,79,87};
        memset(expected,0,sizeof expected);
        for (i = 0; i < 3; ++i) {
            size_t base = i == 0 ? 0 : i == 1 ? 88 : 248;
            for (j = 0; j < sizeof sentinel / sizeof sentinel[0]; ++j)
                expected[base + sentinel[j]] = 1;
            expected[base + 48] = 2;
        }
        expected[26] = 0x40; expected[50] = expected[51] = 0x24;
        expected[114] = 2; expected[138] = expected[139] = 0x20;
        expected[202] = 0x40; expected[212] = 0xc0; expected[226] = 0xd4; expected[227] = 8;
        memcpy(expected+40,names,8); memcpy(expected+128,names+8,8); memcpy(expected+216,names+16,8);
        expected[274] = 0x40; expected[284] = 0x54; expected[299] = 0x20;
        expected[310] = 6; expected[311] = 0x60; expected[331] = 125;
        memcpy(expected+288,names+24,8);
        input = fopen(argv[2],"rb"); CHECK(input != NULL); got = fread(actual,1,sizeof actual,input);
        CHECK(got == sizeof expected && !memcmp(actual,expected,sizeof expected));
        CHECK(!ferror(input) && fclose(input) == 0); puts("basic DCB templates: sizes, organization, modes and fields pass"); return 0;
    }
    if (!strcmp(argv[1],"pdos31-bsam")) {
        static const unsigned char linked[100] = {
            0,0,0,0,0,0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0x41,0x10,0xc0,0,0x18,0x0a,0x50,0,0x10,8,0x18,8,0x50,0,0x10,12,
            0x18,9,0x40,0,0x10,6,0x92,0x80,0x10,5,0x58,0xf0,0x10,8,
            0x54,0xf0,0xc0,0x60,0x58,0xf0,0xf0,0x30,0x54,0xf0,0xc0,0x60,5,0xef,
            0x41,0x10,0xc0,0,0x92,0x20,0x10,5,0x58,0xf0,0x10,8,
            0x54,0xf0,0xc0,0x60,0x58,0xf0,0xf0,0x30,0x54,0xf0,0xc0,0x60,5,0xef,
            7,0xfe,0,0,0,0,0,0xff,0xff,0xff};
        unsigned char actual[101];
        input = fopen(argv[2],"rb"); CHECK(input != NULL); got = fread(actual,1,sizeof actual,input);
        CHECK(got == sizeof linked && !memcmp(actual,linked,sizeof linked));
        CHECK(!ferror(input) && fclose(input) == 0); puts("BSAM: independent request/list and read/write entry bytes pass"); return 0;
    }
    if (!strcmp(argv[1],"pdos31-trkcalc")) {
        static const unsigned char linked[68] = {
            0,0,0,0,0,0,0,0,0,0,0,0,0x90,0xec,0xd0,0x0c,
            0x41,0x10,0xc0,0,0x18,3,0x50,0,0x10,0,0x18,4,0x50,0,0x10,8,
            0x18,5,0x40,0,0x10,6,0x92,0x94,0x10,4,0x18,0x21,0x58,0xf0,0,0x10,
            0x58,0xf0,0xf0,0xe8,0x41,0xf0,0xf0,0x0c,5,0xef,0x98,0x1c,0xd0,0x18,
            0x58,0xe0,0xd0,0x0c,7,0xfe};
        unsigned char actual[69];
        input = fopen(argv[2],"rb"); CHECK(input != NULL); got = fread(actual,1,sizeof actual,input);
        CHECK(got == sizeof linked && !memcmp(actual,linked,sizeof linked));
        CHECK(!ferror(input) && fclose(input) == 0); puts("TRKCALC: list, register setup and preservation bytes pass"); return 0;
    }
    if (!strcmp(argv[1],"pdos31-diagnostics")) {
        static const unsigned char linked[120] = {
            0,0x53,0,0,0,0,0x24,0,0,0,0,0,0,0,0,0,0,0,0x10,0x60,0,0,0x10,0x68,
            0x18,0x19,0x41,0,0xc0,0x60,0x50,0,0x10,8,0x18,7,0x42,0,0x10,0,0x96,8,0x10,1,0x0a,0x33,
            0x41,0x10,0,0x7b,0x0a,0x0d,0x41,0x10,0,1,0x56,0x10,0xc0,0x70,0x0a,0x0d,
            0x18,0x1a,0x58,0xf0,0x10,0x54,0x54,0xf0,0xc0,0x74,5,0xef,
            0x41,0,0xc0,0x60,0x18,0x1a,0x58,0xf0,0x10,0x54,0x54,0xf0,0xc0,0x74,0x41,0xf0,0xf0,4,5,0xef,7,0xfe,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x80,0,0,0,0,0xff,0xff,0xff};
        unsigned char actual[121];
        input = fopen(argv[2],"rb"); CHECK(input != NULL); got = fread(actual,1,sizeof actual,input);
        CHECK(got == sizeof linked && !memcmp(actual,linked,sizeof linked));
        CHECK(!ferror(input) && fclose(input) == 0); puts("diagnostic/positioning interfaces: all 120 independent bytes pass"); return 0;
    }
    if (!strcmp(argv[1],"pdos31-wto")) {
        static const unsigned char linked[39] = {
            0x4d,0x10,0xc0,0x0e,0,6,0x80,0,0xc1,0xc2,2,0,0,0x20,0x1b,0,0x0a,0x23,
            0x4d,0x10,0xc0,0x20,0,5,0x80,0,0xc3,0,0x10,0x80,0,0,
            0x1b,0,0x0a,0x23,0,5,0x80};
        static const unsigned char tail[10] = {0,0xc4,0x80,0,0,1,0,8,0,2};
        unsigned char actual[50];
        input = fopen(argv[2],"rb"); CHECK(input != NULL); got = fread(actual,1,sizeof actual,input);
        CHECK(got == 49 && !memcmp(actual,linked,39) && !memcmp(actual+39,tail,10));
        CHECK(!ferror(input) && fclose(input) == 0); puts("WTO: inline/list bytes and patched-text aliases pass"); return 0;
    }
    if (!strcmp(argv[1],"pdos31-directory")) {
        static const unsigned char linked[40] = {0x18,0x1a,0x41,0,0xc0,0x14,0x0a,0x12,
            0x18,0x1a,0x13,0x11,0x41,0,0xc0,0x20,0x0a,0x12,7,0xfe,
            0,0,0,0,0,0,0,0,0,0,0,0,0xe3,0xc5,0xe2,0xe3,0x40,0x40,0x40,0x40};
        input = fopen(argv[2],"rb"); CHECK(input != NULL); got = fread(card,1,sizeof card,input);
        CHECK(got == sizeof linked && !memcmp(card,linked,sizeof linked));
        CHECK(!ferror(input) && fclose(input) == 0); puts("directory services: all 40 independent bytes pass"); return 0;
    }
    if (!strcmp(argv[1],"pdos31-io-services")) {
        static const unsigned char linked[120] = {
            0xbe,0xa7,0xc0,0x31,0x41,0x10,0xc0,0x30,0x0a,0x40,
            0x41,0x10,0xc0,0x30,0x0a,0x16,0x41,0x10,0xc0,0x30,0x0a,0x14,
            0x41,0x10,0xc0,0x34,0x41,0,0xc0,0x3c,0x0a,0x18,
            0x41,0x10,0xc0,0x48,0x0a,0x1b,0x41,0x10,0xc0,0x48,0x0a,0x1a,7,0xfe,0,0,
            0x80,0,0x10,0x44,0xc4,0xc1,0xe3,0xc1,0x40,0x40,0x40,0x40,
            0,0,0,0,0,0,0,0,0,0,0,0,
            0xc1,0,0,0,0,0,0x10,0x34,0,0,0x10,0x34,0,0,0x10,0x3c,
            0xc0,0x80,0,0,0,0,0x10,0x34,0,0,0x10,0x34,0,0,0x10,0x3c,
            0,0,0,0,0,0,0x10,0x34,0,0,0,0,0,0,0x10,0x3c};
        unsigned char actual[121];
        input = fopen(argv[2],"rb"); CHECK(input != NULL); got = fread(actual,1,sizeof actual,input);
        CHECK(got == sizeof linked && !memcmp(actual,linked,sizeof linked));
        CHECK(!ferror(input) && fclose(input) == 0); puts("short-list/catalog services: all 120 independent bytes pass"); return 0;
    }
    if (!strcmp(argv[1],"pdos31-io-maps")) {
        static const unsigned char linked[40] = {0,0,0,44,0,100,0,102,0,104,0,176,0,176,0,0,0,1,0,2,0,4,0,9,0,12,0,16,0,20,0,24,0,28,0,32,0,1,0,7};
        input = fopen(argv[2],"rb"); CHECK(input != NULL); got = fread(card,1,sizeof card,input);
        CHECK(got == sizeof linked && !memcmp(card,linked,sizeof linked));
        CHECK(!ferror(input) && fclose(input) == 0); puts("JFCB/IOB: 20 independent field and length values pass"); return 0;
    }
    if (!strcmp(argv[1],"classic-partial-save")) {
        static const unsigned char linked[34] = {
            0x47,0xf0,0xf0,8,2,0xc9,0xc4,0,0x90,0x0b,0xd0,20,
            0x98,0x0b,0xd0,20,7,0xfe,0x90,0xec,0xd0,12,0x1b,0xff,
            0x58,0xe0,0xd0,12,0x98,0x0c,0xd0,20,7,0xfe};
        input = fopen(argv[2],"rb"); CHECK(input != NULL); got = fread(card,1,sizeof card,input);
        CHECK(got == sizeof linked && !memcmp(card,linked,sizeof linked));
        CHECK(!ferror(input) && fclose(input) == 0); puts("partial save: all 34 independent bytes pass"); return 0;
    }
    if (!strcmp(argv[1],"encoded-bytes")) {
        unsigned char actual[258];
        input = fopen(argv[2],"rb"); CHECK(input != NULL); got = fread(actual,1,sizeof actual,input);
        CHECK(got == 257);
        for (i = 0; i < 256; ++i) CHECK(actual[i] == i);
        CHECK(actual[256] == 0 && !ferror(input) && fclose(input) == 0);
        puts("compiler numeric escapes: all 256 byte values and terminator pass"); return 0;
    }
    if (!strcmp(argv[1],"pdos31-runtime-maps")) { runtime_maps(argv[2]); return 0; }
    if (!strcmp(argv[1],"pdos31-get")) {
        static const unsigned char linked[20] = {0x18,0x12,0x58,0xf0,0x10,0x30,0x54,0xf0,0xc0,0x10,5,0xef,7,0xfe,0,0,0,0xff,0xff,0xff};
        input = fopen(argv[2],"rb"); CHECK(input != NULL); got = fread(card,1,sizeof card,input);
        CHECK(got == sizeof linked && !memcmp(card,linked,sizeof linked));
        CHECK(!ferror(input) && fclose(input) == 0); puts("GET: all 20 independent linkage bytes pass"); return 0;
    }
    if (!strcmp(argv[1],"pdos31-storage")) { storage_requests(argv[2]); return 0; }
    if (!strcmp(argv[1],"pdos31-freepool")) { pool_release(argv[2]); return 0; }
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
