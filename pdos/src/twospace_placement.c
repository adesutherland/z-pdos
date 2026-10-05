/* SPDX-License-Identifier: MIT
 * Bounded placement ledger for one shared application translation.
 * No host/runtime allocator, native 64-bit C type, or U-pointer dereference.
 */
#include "twospace_placement.h"

static int cmp(TSPADDR a, TSPADDR b)
{
    if (a.hi != b.hi) return a.hi < b.hi ? -1 : 1;
    if (a.lo != b.lo) return a.lo < b.lo ? -1 : 1;
    return 0;
}

static int end_address(TSPADDR first, unsigned int bytes, TSPADDR *last)
{
    unsigned int offset;
    if (!bytes || (bytes & 4095U) || (first.lo & 4095U)) return TSP_BAD;
    offset = bytes - 1U;
    last->lo = first.lo + offset;
    last->hi = first.hi + (last->lo < first.lo ? 1U : 0U);
    if (last->hi < first.hi) return TSP_BAD;
    return TSP_OK;
}

static int mode_ok(unsigned int mode, TSPADDR last)
{
    if (mode == 24U) return last.hi == 0U && last.lo < 0x01000000U;
    if (mode == 31U) return last.hi == 0U && last.lo < 0x80000000U;
    return mode == 64U;
}

void TSPINIT(TSPMAP *map)
{
    unsigned int i;
    if (!map) return;
    map->used = 0U;
    for (i = 0; i < TSP_SLOTS; ++i) map->entry[i].owner = 0U;
}

int TSPRESV(TSPMAP *map, unsigned int owner, unsigned int mode,
            TSPADDR first, unsigned int bytes)
{
    TSPADDR last;
    unsigned int i;
    if (!map || !owner || sizeof(unsigned int) != 4U ||
        end_address(first, bytes, &last) != TSP_OK || !mode_ok(mode, last))
        return TSP_BAD;
    for (i = 0; i < TSP_SLOTS; ++i) {
        const TSPENTRY *e = &map->entry[i];
        if (e->owner && cmp(first, e->last) <= 0 &&
            cmp(e->first, last) <= 0) return TSP_COLLISION;
    }
    for (i = 0; i < TSP_SLOTS; ++i) {
        TSPENTRY *e = &map->entry[i];
        if (!e->owner) {
            e->first = first; e->last = last; e->owner = owner;
            ++map->used;
            return TSP_OK;
        }
    }
    return TSP_FULL;
}

int TSPRELS(TSPMAP *map, unsigned int owner)
{
    unsigned int i, found = 0U;
    if (!map || !owner) return TSP_BAD;
    for (i = 0; i < TSP_SLOTS; ++i) {
        if (map->entry[i].owner == owner) {
            map->entry[i].owner = 0U;
            --map->used;
            found = 1U;
        }
    }
    return found ? TSP_OK : TSP_ABSENT;
}

int TSPFIND(const TSPMAP *map, unsigned int mode, TSPADDR minimum,
            TSPADDR maximum, unsigned int bytes, TSPADDR *result)
{
    TSPADDR at = minimum, last;
    unsigned int i, moved;
    if (!map || !result || sizeof(unsigned int) != 4U ||
        (minimum.lo & 4095U) || cmp(minimum, maximum) > 0)
        return TSP_BAD;
    for (;;) {
        if (end_address(at, bytes, &last) != TSP_OK ||
            !mode_ok(mode, last) || cmp(last, maximum) > 0) return TSP_FULL;
        moved = 0U;
        for (i = 0; i < TSP_SLOTS; ++i) {
            const TSPENTRY *e = &map->entry[i];
            if (!e->owner || cmp(at, e->last) > 0 ||
                cmp(e->first, last) > 0) continue;
            if (e->last.lo > 0xffffefffU &&
                e->last.hi == 0xffffffffU) return TSP_FULL;
            at.lo = (e->last.lo & ~4095U) + 4096U;
            at.hi = e->last.hi + (at.lo == 0U ? 1U : 0U);
            moved = 1U;
            break;
        }
        if (!moved) { *result = at; return TSP_OK; }
    }
}
