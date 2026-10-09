/* SPDX-License-Identifier: MIT
 * Small, supplied-storage real-frame ledger for the successor bootstrap.
 * Phase masks make the PLOAD-to-core handover explicit: a frame may be reused
 * only when the two owners are never live in the same phase.
 */
#ifndef PDOS_TWO_SPACE_REAL_H
#define PDOS_TWO_SPACE_REAL_H

#define TSR_OK 0
#define TSR_BAD -1
#define TSR_COLLISION -2
#define TSR_FULL -3
#define TSR_MISSING -4
#define TSR_PAGE 4096U
#define TSR_LOAD 1U
#define TSR_COPY 2U
#define TSR_RUN 4U
#define TSR_MAX 128U

typedef struct {
    unsigned int start;
    unsigned int size;
    unsigned int phases;
    unsigned int owner;
} TSRRANGE;

typedef struct {
    unsigned int limit;
    unsigned int count;
    unsigned int generation;
    TSRRANGE ranges[TSR_MAX];
} TSRPLAN;

int TSRINIT(TSRPLAN *plan, unsigned int real_bytes);
int TSRRESERVE(TSRPLAN *plan, unsigned int owner, unsigned int start,
               unsigned int size, unsigned int phases);
int TSRALLOC(TSRPLAN *plan, unsigned int owner, unsigned int size,
             unsigned int first, unsigned int end, unsigned int phases,
             unsigned int *start);
int TSRRELEASE(TSRPLAN *plan, unsigned int owner, unsigned int start);

#endif
