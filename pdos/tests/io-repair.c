/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Fault injection against functions extracted from the real PDOS source.
 * Host component evidence only; native WRBLOCK needs guest I/O evidence. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAXBLKSZ 32767
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
typedef struct { int ipldev; } PDOS;
typedef struct { int failed, basecyl, basehead, endcyl, endhead, records; unsigned char first_ttr[3]; } PDOS64PDSWRITE;
typedef struct { char dcbfdad[8]; } DCB;
static int results[8], calls, saved_len, saved_cyl, saved_head, saved_rec;
static unsigned char saved_data[MAXBLKSZ + 8];
static void split_cchhr(char *p, int *c, int *h, int *r) {
    *c = ((unsigned char)p[0] << 8) | (unsigned char)p[1];
    *h = ((unsigned char)p[2] << 8) | (unsigned char)p[3]; *r = (unsigned char)p[4];
}
static void join_cchhr(char *p, int c, int h, int r) {
    p[0] = c >> 8; p[1] = c; p[2] = h >> 8; p[3] = h; p[4] = r;
}
static int wrblock(int dev, int c, int h, int r, void *data, int len, int op) {
    CHECK(dev == 0x1b9 && op == 0x1d && calls < 8);
    saved_len = len; saved_cyl = c; saved_head = h; saved_rec = r;
    memcpy(saved_data, data, len);
    return results[calls++] == 0 ? len : results[calls - 1];
}
#include "record.inc"
static struct { unsigned char before[16]; char bytes[6 + 22 * 80 + 7]; unsigned char after[16]; } storage;
#define intbuf storage.bytes
static int cons_type = 3270, writes;
static void __conswr(size_t len, void *data, int mode) {
    CHECK(len == sizeof intbuf && data == intbuf && mode == 0);
    CHECK(memcmp(intbuf, "\x41\x11\x5d\x7f\x1d\xf0", 6) == 0);
    CHECK(memcmp(intbuf + 6 + 22 * 80, "\x1d\x00\x13\x3c\x5d\x7f\x00", 7) == 0);
    for (int i = 0; i < 16; i++) CHECK(storage.before[i] == 0xa5 && storage.after[i] == 0xa5);
    writes++;
}
#include "console.inc"
static void record_tests(void) {
    PDOS os = {0x1b9}; DCB dcb = {{0}};
    PDOS64PDSWRITE state = {0, 38, 4, 38, 6, 0, {0}};
    unsigned char payload[16]; memset(payload, 0x93, sizeof payload);
    join_cchhr(dcb.dcbfdad + 3, 38, 4, 3);
    results[0] = 8; results[1] = 0; calls = 0;
    CHECK(pdos64PdsRecord(&os, &state, &dcb, payload, 16) == 1);
    CHECK(calls == 2 && saved_cyl == 38 && saved_head == 5 && saved_rec == 0);
    CHECK(saved_len == 24 && memcmp(saved_data + 8, payload, 16) == 0);
    CHECK(state.records == 1 && state.first_ttr[1] == 1 && state.first_ttr[2] == 1);
    results[0] = -1; results[1] = 0; calls = 0;
    CHECK(pdos64PdsRecord(&os, &state, &dcb, payload, 16) == 1);
    CHECK(calls == 2 && saved_head == 6 && saved_rec == 0 && state.records == 2);
    CHECK(memcmp(saved_data + 8, payload, 16) == 0);
    results[0] = 1; results[1] = 0; calls = 0; state.endhead = 7;
    CHECK(pdos64PdsRecord(&os, &state, &dcb, NULL, 0) == 1);
    CHECK(calls == 2 && saved_head == 7 && saved_len == 8 && state.records == 3);
    DCB before = dcb; calls = 0; results[0] = -1;
    CHECK(pdos64PdsRecord(&os, &state, &dcb, payload, 16) == 0);
    CHECK(calls == 1 && state.records == 3 && memcmp(&dcb, &before, sizeof dcb) == 0);
    calls = 0;
    CHECK(pdos64PdsRecord(&os, &state, &dcb, payload, -1) == 0);
    CHECK(pdos64PdsRecord(&os, &state, &dcb, payload, MAXBLKSZ + 1) == 0);
    state.failed = 1;
    CHECK(pdos64PdsRecord(&os, &state, &dcb, payload, 16) == 0 && calls == 0);
    puts("PASS PDS: short/full, unit-check retry, payload preservation, EOF, extent and failure state");
}
static void console_tests(void) {
    char longline[5000]; for (int i = 0; i < 5000; i++) longline[i] = 'A' + i % 26;
    memset(&storage, 0xa5, sizeof storage);
    write3270("HEL", 3, 0);
    write3270("LO", 2, 1);
    CHECK(memcmp(intbuf + 6, "HELLO", 5) == 0);
    write3270("NEXT", 4, 1);
    CHECK(memcmp(intbuf + 6 + 80, "NEXT", 4) == 0);
    write3270(longline, 81, 1);
    CHECK(memcmp(intbuf + 6 + 2 * 80, longline, 80) == 0);
    CHECK(intbuf[6 + 3 * 80] == longline[80]);
    write3270("", 0, 1);
    write3270(longline, 180, 1);
    for (int i = 0; i < 25; i++) write3270("scroll", 6, 1);
    write3270(longline, 5000, 1);
    CHECK(writes >= 30);
    puts("PASS 3270: fragmented echo, protected field, 80/81/180/5000-column output and scrolling");
}
int main(int argc, char **argv) {
    CHECK(argc == 2);
    if (strcmp(argv[1], "record") == 0) record_tests();
    else if (strcmp(argv[1], "console") == 0) console_tests();
    else return 2;
    return 0;
}
