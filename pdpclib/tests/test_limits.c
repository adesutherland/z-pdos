/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Check the retained LP64 limit against host-width arithmetic.
 */
#define __LONG64__
#include "../limits.h"

int main(void)
{
    if (sizeof(unsigned long) != 8) return 1;
    if (ULONG_MAX != ~0UL) return 1;
    if (LONG_MAX != (long)(~0UL >> 1)) return 1;
    return 0;
}
