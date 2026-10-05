/* SPDX-License-Identifier: MIT
 * Sparse region-first DAT table construction from a K-accessible real pool.
 * Bootstrap only: complete and protect both tables before loading either
 * ASCE. Live changes require architecture-defined translation invalidation.
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

typedef struct {
    unsigned char *pool;      /* K31 pointer to independently backed pool */
    unsigned int pool_real;   /* real origin, 4 KiB aligned, below 2 GiB */
    unsigned int capacity;    /* multiple of 4 KiB */
    unsigned int used;
    unsigned int asce_lo;    /* region-first ASCE; high word is zero */
} TSDSTATE;

int TSDINIT(TSDSTATE *state, unsigned char *pool,
            unsigned int pool_real, unsigned int capacity);
int TSDMAP(TSDSTATE *state, TSPADDR virtual_page, TSPADDR real_page);

#endif
