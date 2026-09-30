/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Independent source-variant control: every original executable statement is
 * unchanged except the enumerated, source-owned service replacements.
 * Comments/insignificant whitespace are not part of this semantic comparison.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "source variant: %s\n", #x); exit(1); } } while (0)
struct replacement { const char *source; const char *lines[4]; unsigned count; };
static const struct replacement replacements[] = {
    {" GETMAIN RC,LV=1048576,LOC=ANY", {" L 0,=F'1048576'", " SR 1,1", " LA 15,GMANY", " SVC SVCMEM"}, 4},
    {" GETMAIN RC,LV=256,LOC=BELOW", {" LA 0,256", " SR 1,1", " LA 15,GMBELOW", " SVC SVCMEM"}, 4},
    {" FREEMAIN RC,LV=256,A=(9)", {" LA 0,256", " LR 1,9", " LA 15,FMCOND", " SVC SVCMEM"}, 4},
    {" FREEMAIN RC,LV=1048576,A=(9)", {" L 0,=F'1048576'", " LR 1,9", " LA 15,FMCOND", " SVC SVCMEM"}, 4},
    {" TPUT (9),(8)", {" LR 0,8", " LR 1,9", " SVC SVCTERM", NULL}, 3},
    {" GETMAIN RC,LV=(0),LOC=ANY", {" SR 1,1", " LA 15,GMANY", " SVC SVCMEM", NULL}, 3},
    {" FREEMAIN RC,LV=(0),A=(1)", {" LA 15,FMCOND", " SVC SVCMEM", NULL, NULL}, 2},
    {" DYNALLOC", {" SVC SVCDYN", NULL, NULL, NULL}, 1},
    {" TGET (9),(7)", {" LR 0,7", " LR 1,9", " O 1,=X'80000000'", " SVC SVCTERM"}, 4}
};
static int next(FILE *file, char *out)
{
    char row[256]; size_t i, n; int quoted, pending;
    while (fgets(row, sizeof row, file)) {
        CHECK(strchr(row, '\n') != NULL || feof(file));
        if ((unsigned char)row[0] == 0x2a) continue;
        n = 0; quoted = pending = 0;
        for (i = 0; row[i] && (unsigned char)row[i] != 0x0a && (unsigned char)row[i] != 0x0d; ++i) {
            if (!quoted && (unsigned char)row[i] == 0x20) pending = 1;
            else {
                if (pending) out[n++] = 0x20;
                pending = 0; out[n++] = row[i];
                if ((unsigned char)row[i] == 0x27) quoted = !quoted;
            }
        }
        CHECK(!quoted); out[n] = 0;
        if (n) return 1;
    }
    CHECK(!ferror(file)); return 0;
}
int main(int argc, char **argv)
{
    static const char *definitions[] = {
        "GMANY EQU 48", "GMBELOW EQU 16", "FMCOND EQU 1",
        "SVCMEM EQU 120", "SVCTERM EQU 93", "SVCDYN EQU 99"
    };
    FILE *reference, *variant; char original[256], actual[256];
    unsigned i, j, seen, rows;
    CHECK(argc == 3);
    reference = fopen(argv[1], "rb"); variant = fopen(argv[2], "rb");
    CHECK(reference != NULL && variant != NULL);
    seen = rows = 0;
    while (next(reference, original)) {
        ++rows;
        for (i = 0; i < sizeof replacements / sizeof replacements[0]; ++i)
            if (!strcmp(original, replacements[i].source)) break;
        if (i == sizeof replacements / sizeof replacements[0]) {
            CHECK(next(variant, actual)); CHECK(!strcmp(actual, original));
        } else {
            CHECK(!(seen & (1U << i))); seen |= 1U << i;
            for (j = 0; j < replacements[i].count; ++j) {
                CHECK(next(variant, actual)); CHECK(!strcmp(actual, replacements[i].lines[j]));
            }
        }
        if (rows == 3) {
            CHECK(!strcmp(original, "LABTSO RMODE ANY"));
            for (i = 0; i < sizeof definitions / sizeof definitions[0]; ++i) {
                CHECK(next(variant, actual)); CHECK(!strcmp(actual, definitions[i]));
            }
        }
    }
    CHECK(seen == 0x1ff); CHECK(!next(variant, actual));
    CHECK(fclose(reference) == 0); CHECK(fclose(variant) == 0);
    printf("source variant: %u checks, %u original statements, nine explicit replacements\n", checks, rows);
    return 0;
}
