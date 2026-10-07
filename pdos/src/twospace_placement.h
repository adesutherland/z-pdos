/* SPDX-License-Identifier: MIT
 * Shared-U page reservations. Addresses are two 32-bit words so Classic C31
 * can administer AMODE64 placements without truncating them to a pointer.
 */
#ifndef PDOS_TWO_SPACE_PLACEMENT_H
#define PDOS_TWO_SPACE_PLACEMENT_H

#define TSP_SLOTS 128
#define TSP_OK 0
#define TSP_BAD -1
#define TSP_COLLISION -2
#define TSP_FULL -3
#define TSP_ABSENT -4

typedef struct {
    unsigned int hi;
    unsigned int lo;
} TSPADDR;

typedef struct {
    TSPADDR first;
    TSPADDR last;             /* inclusive, page-rounded */
    unsigned int owner;       /* nonzero module/lifetime identity */
} TSPENTRY;

typedef struct {
    TSPENTRY entry[TSP_SLOTS];
    unsigned int used;
} TSPMAP;

void TSPINIT(TSPMAP *map);
int TSPRESV(TSPMAP *map, unsigned int owner, unsigned int mode,
            TSPADDR first, unsigned int bytes);
int TSPRELS(TSPMAP *map, unsigned int owner);
int TSPFIND(const TSPMAP *map, unsigned int mode, TSPADDR minimum,
            TSPADDR maximum, unsigned int bytes, TSPADDR *result);

#endif
