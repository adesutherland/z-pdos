/* SPDX-License-Identifier: MIT
 * K-owned, two-bank record store in one explicitly allocated FB extent.
 * A bank header is published only after payload write and readback. */
#ifndef PDOS_TWO_SPACE_STORE_H
#define PDOS_TWO_SPACE_STORE_H
#include "twospace_fileprofile.h"

#define TSS_OK 0
#define TSS_ABSENT -1
#define TSS_BAD -2
#define TSS_IO -3
#define TSS_FULL -4
#define TSS_DELETED -5
#define TSS_BLOCK 18452U
#define TSS_LIMIT TSFP_OBJECT_LIMIT
#define TSS_SLOTS 8U
#define TSS_BANK_BLOCKS (1U + (TSS_LIMIT + TSS_BLOCK - 1U) / TSS_BLOCK)
#define TSS_FIRST_BLOCK (TSFP_LAYOUT - 1U)
#define TSS_RECORDS (TSS_FIRST_BLOCK + TSS_SLOTS * 2U * TSS_BANK_BLOCKS)
#define TSS_KEY 56U

typedef int (*TSSIO)(void *context, unsigned int record, unsigned int write,
                     unsigned char *block);
typedef struct {
    TSSIO io;
    void *context;
    unsigned char block[TSS_BLOCK];
} TSSSTATE;

int TSSINIT(TSSSTATE *state, TSSIO io, void *context);
int TSSSIZE(TSSSTATE *state, unsigned int kind, const unsigned char *key,
             unsigned int key_bytes, unsigned int *bytes);
int TSSREAD(TSSSTATE *state, unsigned int kind, const unsigned char *key,
             unsigned int key_bytes, unsigned char *data,
             unsigned int capacity, unsigned int *bytes);
int TSSPUT(TSSSTATE *state, unsigned int kind, const unsigned char *key,
            unsigned int key_bytes, const unsigned char *data,
            unsigned int bytes);
int TSSERASE(TSSSTATE *state, unsigned int kind, const unsigned char *key,
              unsigned int key_bytes);
/* Read-only slot/byte admission for an existing or new bounded object. */
int TSSADMIT(TSSSTATE *,unsigned int,const unsigned char *,unsigned int,unsigned int);
#endif
