/* SPDX-License-Identifier: MIT
 * Independent FB80 check of the selected TSO/E terminal parameter blocks.
 * Expected fields come from IBM TSO/E Programming Services and SVC 6 docs.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "TSO terminal deck: %s\n", #x); exit(1); } } while (0)

static unsigned long be(const unsigned char *p, unsigned n)
{
    unsigned long v;
    v = 0;
    while (n--) v = v * 256UL + *p++;
    return v;
}

static unsigned count(const unsigned char *image, const unsigned char *used,
                      unsigned length, const unsigned char *pattern,
                      unsigned pattern_length)
{
    unsigned i, j, found;
    found = 0;
    for (i = 0; i + pattern_length <= length; ++i) {
        for (j = 0; j < pattern_length; ++j)
            if (!used[i + j] || image[i + j] != pattern[j]) break;
        if (j == pattern_length) ++found;
    }
    return found;
}

int main(int argc, char **argv)
{
    static const unsigned char svc6[] = {0x0a,0x06};
    static const unsigned char svc40[] = {0x0a,0x28};
    static const unsigned char extract_pscb[] = {
        0,0,0,0,0,0,0,0,0,0x40,0,0
    };
    static const unsigned char putblock[] = {
        0x12,0x00,0x00,0x00,0,0,0,0,0,0,0,0
    };
    static const unsigned char getblock[] = {0,0,0x80,0,0,0,0,0};
    static const unsigned char putname[] = {
        0xc9,0xd2,0xd1,0xd7,0xe4,0xe3,0xd3,0x40
    };
    static const unsigned char getname[] = {
        0xc9,0xd2,0xd1,0xc7,0xc5,0xe3,0xd3,0x40
    };
    unsigned char card[80], image[4096], used[4096];
    FILE *f; size_t got; unsigned cards, end_cards, text_cards, i, length;
    CHECK(argc == 2);
    f = fopen(argv[1], "rb"); CHECK(f != NULL);
    memset(image, 0, sizeof image); memset(used, 0, sizeof used);
    cards = end_cards = text_cards = length = 0;
    while ((got = fread(card, 1, sizeof card, f)) != 0) {
        CHECK(got == sizeof card && card[0] == 2);
        if (card[1] == 0xe3 && card[2] == 0xe7 && card[3] == 0xe3) {
            unsigned long offset; unsigned text_length;
            offset = be(card + 5, 3); text_length = (unsigned)be(card + 10, 2);
            CHECK(text_length > 0 && text_length <= 56);
            CHECK(offset + text_length <= sizeof image);
            for (i = 0; i < text_length; ++i) {
                CHECK(!used[offset + i]);
                image[offset + i] = card[16 + i]; used[offset + i] = 1;
            }
            if (length < offset + text_length) length = (unsigned)(offset + text_length);
            ++text_cards;
        } else if (card[1] == 0xc5 && card[2] == 0xd5 && card[3] == 0xc4)
            ++end_cards;
        ++cards;
    }
    CHECK(!ferror(f) && fclose(f) == 0 && cards >= 4);
    CHECK(end_cards == 1 && text_cards >= 1);
    CHECK(count(image, used, length, svc6, sizeof svc6) == 2);
    CHECK(count(image, used, length, svc40, sizeof svc40) == 1);
    CHECK(count(image, used, length, extract_pscb, sizeof extract_pscb) == 1);
    CHECK(count(image, used, length, putblock, sizeof putblock) == 1);
    CHECK(count(image, used, length, getblock, sizeof getblock) == 1);
    CHECK(count(image, used, length, putname, sizeof putname) == 1);
    CHECK(count(image, used, length, getname, sizeof getname) == 1);
    puts("Selected TSO/E terminal and EXTRACT service bytes PASS");
    return 0;
}
