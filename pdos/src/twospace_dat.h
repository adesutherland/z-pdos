/* SPDX-License-Identifier: MIT
 * Sparse region-first DAT table construction from a K-accessible real pool.
 * Bootstrap construction and bounded live page changes on one CPU. A live
 * state requires a supervisor purge callback after every changed mapping.
 * The physical frame allocator must reserve both table pools before assigning
 * backing frames to K or U; C31 code never dereferences a U virtual address.
 */
#ifndef PDOS_TWO_SPACE_DAT_H
#define PDOS_TWO_SPACE_DAT_H

#include "twospace_placement.h"

#define TSD_OK 0
#define TSD_BAD -1
#define TSD_EXISTS -2
#define TSD_FULL -3
#define TSD_MISSING -4

typedef void (*TSDPURGE)(void *context);

typedef struct {
    unsigned char *pool;      /* K31 pointer to independently backed pool */
    unsigned int pool_real;   /* real origin, 4 KiB aligned, below 2 GiB */
    unsigned int capacity;    /* multiple of 4 KiB */
    unsigned int used;        /* high-water bytes; unlinked holes are reusable */
    unsigned int asce_lo;    /* region-first ASCE; high word is zero */
    TSDPURGE purge;
    void *purge_context;
    unsigned int live;
} TSDSTATE;

int TSDINIT(TSDSTATE *state, unsigned char *pool,
            unsigned int pool_real, unsigned int capacity);
/* Reopen the completed pool through its K virtual alias after bootstrap. */
int TSDATTACH(TSDSTATE *state, unsigned char *pool_alias,
              unsigned int pool_real, unsigned int capacity,
              unsigned int used, unsigned int asce_lo);
int TSDMAP(TSDSTATE *state, TSPADDR virtual_page, TSPADDR real_page);
/* Privileged K-only alias after the caller proves table-frame ownership. */
int TSDMAPTABLE(TSDSTATE *state, TSPADDR virtual_page,
                TSPADDR real_page);
/* Unmap also unlinks empty tables; live translation is purged before their
 * frames can be reused. The ASCE root remains allocated. */
int TSDUNMAP(TSDSTATE *state, TSPADDR virtual_page,
             TSPADDR *old_real_page);
/* Translate a complete U address through a K-accessible table alias. The
 * result is a real byte address, never a directly usable C31 U pointer. */
int TSDLOOKUP(const TSDSTATE *state, TSPADDR virtual_byte,
              TSPADDR *real_byte);
int TSDLIVE(TSDSTATE *state, TSDPURGE purge, void *context);

#endif
