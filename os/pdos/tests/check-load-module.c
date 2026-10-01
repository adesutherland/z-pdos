/* SPDX-License-Identifier: MIT
 * Check the actual PDOS loader against a separately linked flat image.
 * The loader retains its target's 32-bit entry ABI; compare its low bits.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pdosutil.h"

#define CAPACITY 4194304
#define REQUIRE(x) do { if (!(x)) { \
    fprintf(stderr,"load-module:%d: %s\n",__LINE__,#x); exit(1); } } while (0)

int int_rdblock(int a, int b, int c, int d, void *e, int f, int g)
{
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g;
    abort(); return 0;
}
static int read_image(const char *path, char *bytes)
{
    FILE *input; size_t n;
    input = fopen(path,"rb"); REQUIRE(input != NULL);
    n = fread(bytes,1,CAPACITY+1,input);
    REQUIRE(n > 0 && n <= CAPACITY && !ferror(input));
    REQUIRE(fclose(input) == 0); return (int)n;
}
int main(int argc, char **argv)
{
    char *original, *image, *expected; int raw, want, length, entry, amode, rmode;
    unsigned long base, offset, expected_entry; unsigned control; int i;
    if (argc != 5) return 2;
    base = strtoul(argv[3],NULL,0); expected_entry = strtoul(argv[4],NULL,0);
    REQUIRE(base <= 0x7fffffffUL && expected_entry < CAPACITY);
    original = (char *)malloc(CAPACITY+1);
    image = (char *)calloc(CAPACITY+1,1);
    expected = (char *)malloc(CAPACITY+1);
    REQUIRE(original && image && expected);
    raw = read_image(argv[1],original); want = read_image(argv[2],expected);
    memcpy(image,original,(size_t)raw); length = raw;
    REQUIRE(fixPEMode(image,&length,&entry,(int)base,CAPACITY,&amode,&rmode) == 0);
    offset = ((unsigned long)(unsigned int)entry-(unsigned long)image) & 0xffffffffUL;
    /* MVS CESD lengths round the final section to eight bytes. */
    REQUIRE(length == (want+7)/8*8 && amode == 2 && rmode == 0);
    for (i = want; i < length; ++i) REQUIRE(image[i] == 0);
    REQUIRE(offset == expected_entry && offset < (unsigned long)length);
    REQUIRE(!memcmp(image,expected,(size_t)want));
    for (control = 0; control < 3; ++control) {
        memcpy(image,original,(size_t)raw); length = raw;
        if (control == 0) { image[0] = 0; image[1] = 1; }
        if (control == 1) --length;
        REQUIRE(fixPEMode(image,&length,&entry,(int)base,
            control == 2 ? raw-1 : CAPACITY,&amode,&rmode) != 0);
    }
    printf("PDOS loader: %d payload bytes, entry 0x%lx, AMODE31/RMODE24; malformed controls rejected\n",want,offset);
    free(original); free(image); free(expected); return 0;
}
