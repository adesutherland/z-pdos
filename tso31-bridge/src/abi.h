/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Existing TSO31 bridge C surface, not a new native PDPCLIB ABI.
 * Target: big-endian 31-bit ELF calling convention, 32-bit unsigned long,
 * four-byte pointers/function pointers. Verify producer and link path separately.
 */
#ifndef LAB_TSO31_ABI_H
#define LAB_TSO31_ABI_H

struct lab_tso31_services {
    unsigned long version; /* exactly 4 */
    int (*putline)(const unsigned char *, unsigned long);
    void *(*allocate)(unsigned long);
    int (*release)(void *, unsigned long);
    int (*filecall)(unsigned long, void *);
    void (*finish)(int); /* returns to original native caller, never to C */
    int (*readline)(unsigned char *, unsigned long, unsigned long *);
};

int ELFPOC(const unsigned char *, unsigned long,
    const struct lab_tso31_services *);

#endif
