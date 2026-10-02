/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Independent consumer object/ABI controls, no assembler library helpers.
 * Offsets are calculated from the maintained source's instruction lengths and
 * natural data alignment; register fields and service values have fixed bytes.
 * OBJSTMT facts: IBM z/VM 7.4 OBJSTMT; service facts: SERVICES.md.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_SIZE 1168
static unsigned checks;
static unsigned long cards;
static unsigned char image[IMAGE_SIZE], present[IMAGE_SIZE];
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "TSO31 deck card %lu: %s\n", cards, #x); exit(1); } } while (0)
struct identity { unsigned id, kind; };
static struct identity ids[7];
static unsigned identity_count;
static unsigned long be(const unsigned char *p, unsigned n)
{ unsigned long v; v = 0; while (n--) v = v * 256UL + *p++; return v; }
static unsigned kind(unsigned id)
{
    unsigned i;
    for (i = 0; i < identity_count; ++i) if (ids[i].id == id) return ids[i].kind;
    return 99;
}
static unsigned name(const unsigned char *p)
{
    static const unsigned char names[7][8] = {
        {0xd3,0xc1,0xc2,0xe3,0xe2,0xd6,0x40,0x40},
        {0xc5,0xd3,0xc6,0xd7,0xd6,0xc3,0x40,0x40},
        {0x7c,0x7c,0xc1,0xd6,0xd7,0xc5,0xd5,0x40},
        {0x7c,0x7c,0xc1,0xd9,0xc5,0xc1,0xc4,0x40},
        {0x7c,0x7c,0xc1,0xe6,0xd9,0xc9,0xe3,0xc5},
        {0x7c,0x7c,0xc1,0xc3,0xd3,0xd6,0xe2,0xc5},
        {0x7c,0x7c,0xc4,0xe8,0xd5,0xc1,0xd3,0x40}
    };
    unsigned i;
    for (i = 0; i < 7; ++i) if (!memcmp(p, names[i], 8)) return i;
    return 99;
}
static void bytes(unsigned offset, const unsigned char *expected, unsigned n)
{
    unsigned i;
    CHECK(offset <= IMAGE_SIZE && n <= IMAGE_SIZE - offset);
    for (i = 0; i < n; ++i) {
        if (!present[offset + i] || image[offset + i] != expected[i])
            fprintf(stderr, "TSO31 byte %u: expected %02x, got %02x (%s)\n",
                offset + i, (unsigned)expected[i], (unsigned)image[offset + i],
                present[offset + i] ? "present" : "absent");
        CHECK(present[offset + i] && image[offset + i] == expected[i]);
    }
}
static int gap(unsigned i)
{
    return (i >= 630 && i < 632) || (i >= 684 && i < 712) ||
        (i >= 726 && i < 752) || (i >= 824 && i < 864);
}
static void abi_and_services(void)
{
    static const unsigned char entry[] = {0x90,0xec,0xd0,0x0c,0x18,0xcf};
    static const unsigned char c_call[] = {
        0x54,0x10,0xc3,0x68,0x48,0x30,0x10,0x00,0x41,0x20,0x10,0x02,
        0x41,0x40,0xc2,0x7c,0x58,0xf0,0xc2,0xe4,0x58,0x10,0xc3,0x6c,0x0d,0xe1
    };
    static const unsigned char native_return[] = {
        0x58,0xf0,0xc2,0xec,0x58,0xd0,0xc2,0xd8,0x58,0xe0,0xd0,0x0c,
        0x98,0x0c,0xd0,0x14,0x07,0xfe
    };
    static const unsigned char get_stack[] = {0x58,0,0xc3,0x60,0x1b,0x11,0x41,0xf0,0,0x30,0x0a,0x78};
    static const unsigned char get_below[] = {0x41,0,1,0,0x1b,0x11,0x41,0xf0,0,0x10,0x0a,0x78};
    static const unsigned char get_callback[] = {0x18,0x02,0x1b,0x11,0x41,0xf0,0,0x30,0x0a,0x78};
    static const unsigned char free_buffer[] = {0x41,0,1,0,0x18,0x19,0x41,0xf0,0,1,0x0a,0x78};
    static const unsigned char free_stack[] = {0x58,0,0xc3,0x60,0x18,0x19,0x41,0xf0,0,1,0x0a,0x78};
    static const unsigned char free_callback[] = {0x18,0x03,0x18,0x12,0x41,0xf0,0,1,0x0a,0x78};
    static const unsigned char put[] = {0x18,0x08,0x18,0x19,0x0a,0x5d};
    static const unsigned char get[] = {0x18,0x07,0x18,0x19,0x56,0x10,0xc3,0x7c,0x0a,0x5d};
    static const unsigned char native_file[] = {
        0x18,0x13,0x89,0x20,0,2,0x41,0x40,0xc2,0x98,
        0x58,0xf2,0x40,0,0x05,0xef,0x18,0x2f
    };
    /* Clear the request, length=20, unallocate verb=2, text-unit list at +8,
     * flag both final pointers, copy eight DD bytes, and enter SVC 99 via R1. */
    static const unsigned char dynalloc[] = {
        0xd7,0x13,0xc2,0xb0,0xc2,0xb0,0x92,0x14,0xc2,0xb0,
        0x92,2,0xc2,0xb1,0x41,0x40,0xc2,0xc8,0x56,0x40,0xc3,0x7c,
        0x50,0x40,0xc2,0xc4,0x41,0x40,0xc2,0xc4,0x50,0x40,0xc2,0xb8,
        0xd2,7,0xc2,0xce,0x30,0,0x41,0x40,0xc2,0xb0,
        0x56,0x40,0xc3,0x7c,0x50,0x40,0xc2,0xac,
        0x41,0x10,0xc2,0xac,0x0a,0x63
    };
    static const unsigned char returned_callback[] = {0x98,0x6f,0xc3,0x38,0x07,0xfe};
    static const unsigned callbacks[5] = {202,286,330,362,484};
    static const unsigned returns[5] = {280,324,356,414,614};
    static const unsigned char finish[] = {0x0d,0x10,0x58,0xc0,0x10,0x0a,0x47,0xf0,0xc0,0x84};
    static const unsigned char branch_nz[] = {0x47,0x70,0xc0,0x80};
    static const unsigned char branch_np[] = {0x47,0xd0,0xc2,0x62};
    static const unsigned char text_unit[] = {0,1,0,1,0,8,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40};
    /* Each selected literal is four bytes. Length grouping therefore keeps
     * their first-use order across F, X and V, rather than grouping by type. */
    static const unsigned char literals[48] = {
        0,0x10,0,0,0,0x0f,0xff,0xa0,0x7f,0xff,0xff,0xff,0,0,0,0,
        0,0,0,0x84,0,0,0,5,0xff,0xff,0xff,0xff,0x80,0,0,0,
        0,0,1,0,0,0,0,0x0c,0,0,0,0x18,0,0,0,0x1c
    };
    /* Exact project-owned reference table, transcribed independently from
     * the retained source. This checks every DC octet, not only printable
     * character anchors or the assembler's own character conversion. */
    static const unsigned char translation[256] = {
        0x00,0x01,0x02,0x03,0x37,0x2d,0x2e,0x2f,0x16,0x05,0x25,0x0b,0x0c,0x0d,0x0e,0x0f,
        0x10,0x11,0x12,0x13,0x3c,0x3d,0x32,0x26,0x18,0x19,0x3f,0x27,0x1c,0x1d,0x1e,0x1f,
        0x40,0x5a,0x7f,0x7b,0x5b,0x6c,0x50,0x7d,0x4d,0x5d,0x5c,0x4e,0x6b,0x60,0x4b,0x61,
        0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0x7a,0x5e,0x4c,0x7e,0x6e,0x6f,
        0x7c,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,
        0xd7,0xd8,0xd9,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xad,0xe0,0xbd,0x5f,0x6d,
        0x79,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x91,0x92,0x93,0x94,0x95,0x96,
        0x97,0x98,0x99,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xc0,0x4f,0xd0,0xa1,0x07,
        0x20,0x21,0x22,0x23,0x24,0x15,0x06,0x17,0x28,0x29,0x2a,0x2b,0x2c,0x09,0x0a,0x1b,
        0x30,0x31,0x1a,0x33,0x34,0x35,0x36,0x08,0x38,0x39,0x3a,0x3b,0x04,0x14,0x3e,0xff,
        0x41,0xaa,0x4a,0xb1,0x9f,0xb2,0x6a,0xb5,0xbb,0xb4,0x9a,0x8a,0xb0,0xca,0xaf,0xbc,
        0x90,0x8f,0xea,0xfa,0xbe,0xa0,0xb6,0xb3,0x9d,0xda,0x9b,0x8b,0xb7,0xb8,0xb9,0xab,
        0x64,0x65,0x62,0x66,0x63,0x67,0x9e,0x68,0x74,0x71,0x72,0x73,0x78,0x75,0x76,0x77,
        0xac,0x69,0xed,0xee,0xeb,0xef,0xec,0xbf,0x80,0xfd,0xfe,0xfb,0xfc,0xba,0xae,0x59,
        0x44,0x45,0x42,0x46,0x43,0x47,0x9c,0x48,0x54,0x51,0x52,0x53,0x58,0x55,0x56,0x57,
        0x8c,0x49,0xcd,0xce,0xcb,0xcf,0xcc,0xe1,0x70,0xdd,0xde,0xdb,0xdc,0x8d,0x8e,0xdf
    };
    unsigned i, base, displacement; unsigned char prologue[14];
    bytes(0, entry, sizeof entry); bytes(98, c_call, sizeof c_call);
    bytes(184, native_return, sizeof native_return);
    bytes(38, get_stack, sizeof get_stack); bytes(300, get_callback, sizeof get_callback);
    bytes(68, get_below, sizeof get_below);
    bytes(150, free_buffer, sizeof free_buffer); bytes(172, free_stack, sizeof free_stack);
    bytes(344, free_callback, sizeof free_callback);
    bytes(264, put, sizeof put); bytes(528, get, sizeof get);
    bytes(388, native_file, sizeof native_file);
    bytes(420, dynalloc, sizeof dynalloc); bytes(620, finish, sizeof finish);
    bytes(52, branch_nz, sizeof branch_nz); bytes(512, branch_np, sizeof branch_np);
    bytes(712, text_unit, sizeof text_unit); bytes(864, literals, sizeof literals);
    bytes(912, translation, sizeof translation);
    for (i = 0; i < 5; ++i) {
        base = callbacks[i] + 2; displacement = 824 - base;
        prologue[0] = 0x0d; prologue[1] = 0x10; prologue[2] = 0x90; prologue[3] = 0x6f;
        prologue[4] = (unsigned char)(0x10 | (displacement >> 8)); prologue[5] = (unsigned char)displacement;
        displacement = 632 - base;
        prologue[6] = 0x58; prologue[7] = 0xc0;
        prologue[8] = (unsigned char)(0x10 | (displacement >> 8)); prologue[9] = (unsigned char)displacement;
        prologue[10] = 0x41; prologue[11] = 0xd0; prologue[12] = 0xc2; prologue[13] = 0xf0;
        bytes(callbacks[i], prologue, sizeof prologue);
        bytes(returns[i], returned_callback, sizeof returned_callback);
    }
    CHECK(be(image + 632, 4) == 0); CHECK(be(image + 636, 4) == 4);
    for (i = 752; i < 824; ++i) CHECK(image[i] == 0 && present[i]);
    /* IBM1047 printable anchors, including the bracket/caret differences from
     * CP037; the preserved source comparison covers every table octet. */
    CHECK(image[912 + 0x20] == 0x40 && image[912 + 0x41] == 0xc1);
    CHECK(image[912 + 0x5b] == 0xad && image[912 + 0x5d] == 0xbd);
    CHECK(image[912 + 0x5e] == 0x5f && image[912 + 0x7e] == 0xa1);
}
int main(int argc, char **argv)
{
    static const unsigned expected_offsets[13] = {632,640,644,648,652,656,660,664,668,672,676,680,876};
    static const unsigned expected_targets[13] = {0,0,0,0,0,0,0,2,3,4,5,6,1};
    static const unsigned expected_addends[13] = {0,202,286,330,362,620,484,0,0,0,0,0,0};
    unsigned char card[80]; FILE *input; size_t got;
    unsigned i, j, id, n, k, names_seen, ended, body, relocations, flag, target, owner;
    unsigned long offset; unsigned reloc_seen;
    CHECK(argc == 2); input = fopen(argv[1], "rb"); CHECK(input != NULL);
    names_seen = ended = body = relocations = reloc_seen = 0;
    while ((got = fread(card, 1, sizeof card, input)) != 0) {
        ++cards; CHECK(got == sizeof card && card[0] == 2 && !ended);
        if (card[1] == 0xc5 && card[2] == 0xe2 && card[3] == 0xc4) {
            CHECK(!body); n = (unsigned)be(card + 10, 2);
            CHECK(n > 0 && n <= 48 && n % 16 == 0); id = (unsigned)be(card + 14, 2);
            for (i = 16; i < 16 + n; i += 16) {
                k = name(card + i); CHECK(k < 7 && !(names_seen & (1U << k)));
                CHECK(id != 0 && kind(id) == 99 && identity_count < 7);
                ids[identity_count].id = id++; ids[identity_count++].kind = k; names_seen |= 1U << k;
                if (!k) CHECK(card[i + 8] == 0 && card[i + 12] == 6 && be(card + i + 9, 3) == 0 && be(card + i + 13, 3) == IMAGE_SIZE);
                else CHECK(card[i + 8] == 2);
            }
        } else if (card[1] == 0xe3 && card[2] == 0xe7 && card[3] == 0xe3) {
            body = 1; n = (unsigned)be(card + 10, 2); offset = be(card + 5, 3);
            CHECK(n > 0 && n <= 56 && kind((unsigned)be(card + 14, 2)) == 0);
            CHECK(offset <= IMAGE_SIZE && n <= IMAGE_SIZE - offset);
            for (i = 0; i < n; ++i) { CHECK(!present[offset + i]); present[offset + i] = 1; image[offset + i] = card[16 + i]; }
        } else if (card[1] == 0xd9 && card[2] == 0xd3 && card[3] == 0xc4) {
            body = 1; n = (unsigned)be(card + 10, 2); CHECK(n > 0 && n <= 56);
            i = 0; target = owner = 99; flag = 0;
            while (i < n) {
                if (!(flag & 1)) { CHECK(n - i >= 8); target = kind((unsigned)be(card + 16 + i, 2)); owner = kind((unsigned)be(card + 18 + i, 2)); i += 4; }
                else CHECK(n - i >= 4);
                flag = card[16 + i]; offset = be(card + 17 + i, 3); i += 4;
                CHECK(owner == 0);
                for (j = 0; j < 13; ++j) if (expected_offsets[j] == offset) break;
                CHECK(j < 13 && !(reloc_seen & (1U << j)) && target == expected_targets[j]);
                CHECK((flag & 0xfe) == (target ? 0x1c : 0x0c));
                reloc_seen |= 1U << j; ++relocations;
            }
            CHECK(!(flag & 1));
        } else {
            CHECK(card[1] == 0xc5 && card[2] == 0xd5 && card[3] == 0xc4);
            CHECK(kind((unsigned)be(card + 14, 2)) == 0 && be(card + 5, 3) == 0); ended = 1;
        }
    }
    CHECK(!ferror(input)); CHECK(fclose(input) == 0);
    CHECK(ended && names_seen == 127 && identity_count == 7 && relocations == 13 && reloc_seen == 0x1fff);
    for (i = 0; i < IMAGE_SIZE; ++i) CHECK(present[i] == !gap(i));
    for (i = 0; i < 13; ++i) CHECK(be(image + expected_offsets[i], 4) == expected_addends[i]);
    abi_and_services();
    printf("TSO31 deck: %u independent checks (%lu cards), complete section/modes/gaps/relocations and ABI/service controls\n", checks, cards);
    return 0;
}
