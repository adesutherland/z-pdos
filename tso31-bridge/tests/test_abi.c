/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Independent host control for the small C caller's callback contract.
 * It does not establish a mainframe register ABI or guest service behavior.
 */
#include "../src/abi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "caller contract: %s\n", #x); exit(1); } } while (0)
static unsigned char storage[64];
static unsigned alloc_calls, put_calls, free_calls;
static int alloc_failure, put_failure, free_failure;

static void *allocate(unsigned long n)
{
    CHECK(n == 64); ++alloc_calls;
    memset(storage, 0xa5, sizeof storage);
    return alloc_failure ? NULL : storage;
}
static int put(const unsigned char *p, unsigned long n)
{
    static const unsigned char expected[] = {
        0x54,0x53,0x4f,0x33,0x31,0x20,0x43,0x41,0x4c,
        0x4c,0x45,0x52,0x20,0x50,0x41,0x53,0x53
    };
    CHECK(p == storage && n == sizeof expected);
    CHECK(!memcmp(p, expected, sizeof expected));
    CHECK(storage[sizeof expected] == 0xa5); ++put_calls;
    return put_failure;
}
static int release(void *p, unsigned long n)
{ CHECK(p == storage && n == 64); ++free_calls; return free_failure; }
static void reset(void)
{
    alloc_calls = put_calls = free_calls = 0;
    alloc_failure = put_failure = free_failure = 0;
}
int main(void)
{
    struct lab_tso31_services services;
    memset(&services, 0, sizeof services);
    services.version = 4; services.allocate = allocate;
    services.putline = put; services.release = release;
    reset(); CHECK(ELFPOC(NULL, 0, &services) == 0);
    CHECK(alloc_calls == 1 && put_calls == 1 && free_calls == 1);
    reset(); alloc_failure = 1; CHECK(ELFPOC(NULL, 0, &services) == 20);
    CHECK(alloc_calls == 1 && !put_calls && !free_calls);
    reset(); put_failure = 1; CHECK(ELFPOC(NULL, 0, &services) == 12);
    CHECK(put_calls == 1 && free_calls == 1);
    reset(); free_failure = 4; CHECK(ELFPOC(NULL, 0, &services) == 16);
    CHECK(put_calls == 1 && free_calls == 1);
    reset(); CHECK(ELFPOC(NULL, 1, &services) == 16);
    CHECK(!alloc_calls && !put_calls && !free_calls);
    CHECK(ELFPOC(NULL, 0, NULL) == 16);
    services.version = 3; CHECK(ELFPOC(NULL, 0, &services) == 16);
    services.version = 4; services.release = NULL;
    CHECK(ELFPOC(NULL, 0, &services) == 16);
    printf("caller: %u independent behavior checks passed\n", checks);
    return 0;
}
