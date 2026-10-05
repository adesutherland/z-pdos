/* SPDX-License-Identifier: MIT */
#include "twospace_real.h"

static int aligned(unsigned int n) { return (n & (TSR_PAGE - 1U)) == 0U; }

int TSRINIT(TSRPLAN *p, unsigned int real_bytes)
{
    if (!p || !aligned(real_bytes) || real_bytes < TSR_PAGE) return TSR_BAD;
    p->limit = real_bytes;
    p->count = 0U;
    return TSR_OK;
}

int TSRRESERVE(TSRPLAN *p, unsigned int owner, unsigned int start,
               unsigned int size, unsigned int phases)
{
    unsigned int i;
    if (!p || !owner || !phases || !size || !aligned(start) ||
        !aligned(size) || start > p->limit || size > p->limit - start)
        return TSR_BAD;
    for (i = 0U; i < p->count; ++i) {
        const TSRRANGE *r = &p->ranges[i];
        if ((r->phases & phases) && start < r->start + r->size &&
            r->start < start + size) return TSR_COLLISION;
    }
    if (p->count == TSR_MAX) return TSR_FULL;
    p->ranges[p->count].owner = owner;
    p->ranges[p->count].start = start;
    p->ranges[p->count].size = size;
    p->ranges[p->count].phases = phases;
    ++p->count;
    return TSR_OK;
}

int TSRALLOC(TSRPLAN *p, unsigned int owner, unsigned int size,
             unsigned int first, unsigned int end, unsigned int phases,
             unsigned int *start)
{
    unsigned int at, i, next;
    int blocked;
    if (!p || !start || !size || !aligned(size) || !aligned(first) ||
        !aligned(end) || first > end || end > p->limit) return TSR_BAD;
    at = first;
    while (at <= end && size <= end - at) {
        blocked = 0;
        next = at + TSR_PAGE;
        for (i = 0U; i < p->count; ++i) {
            const TSRRANGE *r = &p->ranges[i];
            if ((r->phases & phases) && at < r->start + r->size &&
                r->start < at + size) {
                blocked = 1;
                if (r->start + r->size > next) next = r->start + r->size;
            }
        }
        if (!blocked) {
            int rc = TSRRESERVE(p, owner, at, size, phases);
            if (rc == TSR_OK) *start = at;
            return rc;
        }
        at = next;
    }
    return TSR_FULL;
}

int TSRRELEASE(TSRPLAN *p, unsigned int owner, unsigned int start)
{
    unsigned int i;
    if (!p || !owner) return TSR_BAD;
    for (i = 0U; i < p->count; ++i) {
        if (p->ranges[i].owner == owner && p->ranges[i].start == start) {
            --p->count;
            p->ranges[i] = p->ranges[p->count];
            return TSR_OK;
        }
    }
    return TSR_MISSING;
}
