/* SPDX-License-Identifier: MIT
 * Independently decode actual XMIT segment output at payload boundaries. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ld.h"
#include "xmit.h"

const char *program_name = "test_pdld_xmit_segments";
static struct ld_state test_state;
struct ld_state *ld_state = &test_state;

static void check(int condition, size_t bytes, const char *message)
{
    if (!condition) {
        fprintf(stderr, "XMIT %lu-byte record: %s\n", (unsigned long)bytes, message);
        exit(1);
    }
}
int main(void)
{
    static const size_t lengths[] = {1, 252, 253, 254, 505, 506, 507, 759, 760, 4096, 6112};
    size_t test;
    for (test = 0; test < sizeof lengths / sizeof lengths[0]; ++test) {
        size_t bytes = lengths[test], framed = bytes + 2 * ((bytes + 252) / 253);
        size_t at = 0, decoded = 0, segments = 0, i;
        unsigned char *buffer = malloc(framed + 32), *start, *end;
        check(buffer != NULL, bytes, "allocation");
        memset(buffer, 0xa5, framed + 32); start = buffer + 16;
        for (i = 0; i < bytes; ++i) start[i] = (unsigned char)i;
        check(xmit_calculate_segments(bytes, 0) == framed, bytes, "size prediction");
        end = xmit_write_segments(start + bytes, start);
        check((size_t)(end - start) == framed, bytes, "written size");
        while (at < framed) {
            unsigned length = start[at], flags = start[at + 1];
            check(length >= 3 && length <= 255 && length <= framed - at, bytes, "segment length");
            check(flags == ((at == 0 ? 0x80 : 0) | (at + length == framed ? 0x40 : 0)),
                  bytes, "first/last flags");
            for (i = 2; i < length; ++i)
                check(start[at + i] == (unsigned char)decoded++, bytes, "payload changed");
            at += length; ++segments;
        }
        check(decoded == bytes && segments == (bytes + 252) / 253, bytes, "record reconstruction");
        for (i = 0; i < 16; ++i)
            check(buffer[i] == 0xa5 && end[i] == 0xa5, bytes, "guard changed");
        free(buffer);
    }
    puts("XMIT boundary records decoded exactly; guards intact");
    return 0;
}
