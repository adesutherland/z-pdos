/* SPDX-License-Identifier: MIT
 * z/Architecture full-width five-level DAT, page mappings only.
 * Entry formats: IBM SA22-7832-14, Dynamic Address Translation.
 * A 31-bit supervisor can prepare tables; the assembler nucleus loads the
 * 64-bit ASCE with LCTLG and can execute anywhere in K's 64-bit VA range.
 */
#include "twospace_dat.h"

static unsigned int read32(const unsigned char *p)
{
    return ((unsigned int)p[0] << 24) | ((unsigned int)p[1] << 16) |
           ((unsigned int)p[2] << 8) | (unsigned int)p[3];
}

static void write32(unsigned char *p, unsigned int value)
{
    p[0] = (unsigned char)(value >> 24);
    p[1] = (unsigned char)(value >> 16);
    p[2] = (unsigned char)(value >> 8);
    p[3] = (unsigned char)value;
}

static void write64(unsigned char *p, TSPADDR value)
{ write32(p, value.hi); write32(p + 4, value.lo); }

static unsigned char *entry(TSDSTATE *s, unsigned int origin,
                            unsigned int index)
{
    unsigned int offset = origin - s->pool_real + index * 8U;
    return s->pool + offset;
}

/* An unlinked frame has an impossible table/PTE first entry: table high
 * words are zero, and a mapped page's low word is page-aligned. Keep this
 * ledger in the pool so bootstrap attachment needs no new state fields. */
#define FREE_HI 0x46524545U
#define FREE_LO 0x54534401U

static int free_frame(const unsigned char *p)
{ return read32(p) == FREE_HI && read32(p + 4) == FREE_LO; }

static void initialize(TSDSTATE *s, unsigned int offset,
                       unsigned int bytes, unsigned int invalid)
{
    unsigned int i;
    for (i = 0; i < bytes; i += 8U) {
        write32(s->pool + offset + i, 0U);
        write32(s->pool + offset + i + 4U, invalid);
    }
}

/* Plan at most four disjoint tables without changing the pool or state.
 * A failed map must not consume holes or publish a partial table path. */
static int reserve_table(TSDSTATE *s, unsigned int bytes,
                         unsigned int offsets[4], unsigned int sizes[4],
                         unsigned int count, unsigned int *used)
{
    unsigned int offset, i, j;
    for (offset = 16384U; offset <= s->used &&
         bytes <= s->used - offset; offset += 4096U) {
        for (i = 0U; i < count; ++i)
            if (offset < offsets[i] + sizes[i] &&
                offsets[i] < offset + bytes) break;
        if (i != count) continue;
        for (j = 0U; j < bytes; j += 4096U)
            if (!free_frame(s->pool + offset + j)) break;
        if (j == bytes) {
            offsets[count] = offset; sizes[count] = bytes;
            return TSD_OK;
        }
    }
    if (bytes > s->capacity - *used) return TSD_FULL;
    offsets[count] = *used; sizes[count] = bytes;
    *used += bytes;
    return TSD_OK;
}

static int empty_table(TSDSTATE *s, unsigned int origin,
                       unsigned int bytes, unsigned int invalid)
{
    unsigned int i;
    const unsigned char *p = s->pool + origin - s->pool_real;
    for (i = 0U; i < bytes; i += 8U)
        if (read32(p + i) != 0U || read32(p + i + 4U) != invalid)
            return 0;
    return 1;
}

static void release_table(TSDSTATE *s, unsigned int origin,
                          unsigned int bytes)
{
    unsigned int i;
    unsigned char *p = s->pool + origin - s->pool_real;
    for (i = 0U; i < bytes; i += 4096U) {
        write32(p + i, FREE_HI); write32(p + i + 4U, FREE_LO);
    }
}

static unsigned int child(const unsigned char *p, unsigned int invalid)
{
    if (read32(p) != 0U || read32(p+4) == invalid) return 0U;
    return read32(p+4) & ~4095U;
}

int TSDINIT(TSDSTATE *s, unsigned char *pool,
            unsigned int pool_real, unsigned int capacity)
{
    if (!s || !pool || sizeof(unsigned int) != 4U ||
        (pool_real & 4095U) || (capacity & 4095U) ||
        pool_real >= 0x80000000U ||
        capacity < 16384U || capacity > 0x80000000U - pool_real)
        return TSD_BAD;
    s->pool = pool; s->pool_real = pool_real;
    s->capacity = capacity; s->used = 0U;
    s->purge = 0; s->purge_context = 0; s->live = 0U;
    initialize(s, 0U, 16384U, 0x20U);
    s->used = 16384U;
    s->asce_lo = s->pool_real | 0x0fU;
    return TSD_OK;
}

int TSDATTACH(TSDSTATE *s, unsigned char *pool_alias,
              unsigned int pool_real, unsigned int capacity,
              unsigned int used, unsigned int asce_lo)
{
    if (!s || !pool_alias || sizeof(unsigned int) != 4U ||
        (pool_real & 4095U) || (capacity & 4095U) ||
        (used & 4095U) || used < 16384U || used > capacity ||
        pool_real >= 0x80000000U ||
        capacity > 0x80000000U - pool_real ||
        asce_lo != (pool_real | 0x0fU)) return TSD_BAD;
    s->pool = pool_alias; s->pool_real = pool_real;
    s->capacity = capacity; s->used = used; s->asce_lo = asce_lo;
    s->purge = 0; s->purge_context = 0; s->live = 0U;
    return TSD_OK;
}

static int map_page(TSDSTATE *s, TSPADDR va, TSPADDR pa,
                    int allow_table_frame)
{
    unsigned int indexes[5], origins[5], absent[4], need, i, next;
    unsigned int offsets[4], sizes[4], count, planned_used;
    unsigned char *p;
    TSPADDR value;
    if (!s || !s->pool || !s->asce_lo ||
        (va.lo & 4095U) || (pa.lo & 4095U) ||
        (!allow_table_frame && pa.hi == 0U && pa.lo >= s->pool_real &&
         pa.lo - s->pool_real < s->capacity)) return TSD_BAD;
    indexes[0] = va.hi >> 21;
    indexes[1] = (va.hi >> 10) & 2047U;
    indexes[2] = ((va.hi & 1023U) << 1) | (va.lo >> 31);
    indexes[3] = (va.lo >> 20) & 2047U;
    indexes[4] = (va.lo >> 12) & 255U;
    origins[0] = s->asce_lo & ~4095U;
    need = 0U;
    for (i = 0; i < 4U; ++i) {
        if (need) { absent[i] = 1U; need += i == 3U ? 4096U : 16384U; continue; }
        p = entry(s, origins[i], indexes[i]);
        next = child(p, 0x20U);
        if (!next) {
            absent[i] = 1U;
            need += i == 3U ? 4096U : 16384U;
        } else {
            absent[i] = 0U;
            if (next < s->pool_real ||
                (next & 4095U) ||
                s->used < (i == 3U ? 4096U : 16384U) ||
                next - s->pool_real >
                    s->used - (i == 3U ? 4096U : 16384U)) return TSD_BAD;
            origins[i+1] = next;
        }
    }
    if (!need) {
        p = entry(s, origins[4], indexes[4]);
        if (read32(p) != 0U || read32(p+4) != 0x400U)
            return TSD_EXISTS;
    }
    count = 0U; planned_used = s->used;
    for (i = 0U; i < 4U; ++i) {
        if (absent[i]) {
            if (reserve_table(s, i == 3U ? 4096U : 16384U,
                              offsets, sizes, count, &planned_used) != TSD_OK)
                return TSD_FULL;
            origins[i+1] = s->pool_real + offsets[count];
            ++count;
        }
    }
    for (i = 0U; i < count; ++i)
        initialize(s, offsets[i], sizes[i],
                   sizes[i] == 4096U ? 0x400U : 0x20U);
    s->used = planned_used;
    for (i = 0; i < 4U; ++i) {
        if (absent[i]) {
            next = origins[i+1];
            value.hi = 0U;
            value.lo = next | (i == 0U ? 0x0fU :
                               i == 1U ? 0x0bU : i == 2U ? 0x07U : 0U);
            write64(entry(s, origins[i], indexes[i]), value);
        }
    }
    write64(entry(s, origins[4], indexes[4]), pa);
    if (s->live) s->purge(s->purge_context);
    return TSD_OK;
}

int TSDMAP(TSDSTATE *s, TSPADDR va, TSPADDR pa)
{ return map_page(s,va,pa,0); }

int TSDMAPTABLE(TSDSTATE *s, TSPADDR va, TSPADDR pa)
{ return map_page(s,va,pa,1); }

int TSDLIVE(TSDSTATE *s, TSDPURGE purge, void *context)
{
    if (!s || !s->pool || !purge || s->live) return TSD_BAD;
    s->purge = purge;
    s->purge_context = context;
    s->live = 1U;
    return TSD_OK;
}

int TSDUNMAP(TSDSTATE *s, TSPADDR va, TSPADDR *old_pa)
{
    unsigned int indexes[5], origins[5], next, i, first_free;
    unsigned char *p;
    if (!s || !s->pool || !s->asce_lo || !old_pa ||
        (va.lo & 4095U)) return TSD_BAD;
    indexes[0] = va.hi >> 21;
    indexes[1] = (va.hi >> 10) & 2047U;
    indexes[2] = ((va.hi & 1023U) << 1) | (va.lo >> 31);
    indexes[3] = (va.lo >> 20) & 2047U;
    indexes[4] = (va.lo >> 12) & 255U;
    origins[0] = s->asce_lo & ~4095U;
    for (i = 0U; i < 4U; ++i) {
        p = entry(s,origins[i],indexes[i]);
        next = child(p,0x20U);
        if (!next) return TSD_MISSING;
        if (next < s->pool_real || (next & 4095U) ||
            s->used < (i == 3U ? 4096U : 16384U) ||
            next - s->pool_real >
                s->used - (i == 3U ? 4096U : 16384U))
            return TSD_BAD;
        origins[i+1] = next;
    }
    p = entry(s,origins[4],indexes[4]);
    if (read32(p) == 0U && read32(p+4) == 0x400U)
        return TSD_MISSING;
    if (read32(p+4) & 4095U) return TSD_BAD;
    old_pa->hi = read32(p);
    old_pa->lo = read32(p+4) & ~4095U;
    write32(p,0U); write32(p+4,0x400U);
    first_free = 5U;
    for (i = 4U; i != 0U; --i) {
        if (!empty_table(s, origins[i], i == 4U ? 4096U : 16384U,
                         i == 4U ? 0x400U : 0x20U)) break;
        p = entry(s,origins[i-1],indexes[i-1]);
        write32(p,0U); write32(p+4,0x20U);
        first_free = i;
    }
    /* Unlink, purge the single CPU, then make the old storage reusable. */
    if (s->live) s->purge(s->purge_context);
    for (i = first_free; i < 5U; ++i)
        release_table(s,origins[i],i == 4U ? 4096U : 16384U);
    while (s->used > 16384U && free_frame(s->pool + s->used - 4096U))
        s->used -= 4096U;
    return TSD_OK;
}

int TSDLOOKUP(const TSDSTATE *s, TSPADDR va, TSPADDR *real)
{
    unsigned int indexes[5], origin, next, i, bytes, low;
    const unsigned char *p;
    if (!s || !s->pool || !s->asce_lo || !real) return TSD_BAD;
    indexes[0] = va.hi >> 21;
    indexes[1] = (va.hi >> 10) & 2047U;
    indexes[2] = ((va.hi & 1023U) << 1) | (va.lo >> 31);
    indexes[3] = (va.lo >> 20) & 2047U;
    indexes[4] = (va.lo >> 12) & 255U;
    origin = s->asce_lo & ~4095U;
    for (i = 0U; i < 5U; ++i) {
        bytes = i == 4U ? 4096U : 16384U;
        if (origin < s->pool_real || (origin & 4095U) ||
            s->used < bytes || origin - s->pool_real > s->used - bytes)
            return TSD_BAD;
        p = s->pool + origin - s->pool_real + indexes[i] * 8U;
        if (read32(p) != 0U) return TSD_BAD;
        low = read32(p + 4);
        if (i == 4U) {
            if (low == 0x400U) return TSD_MISSING;
            if (low & 4095U) return TSD_BAD;
            real->hi = 0U;
            real->lo = low + (va.lo & 4095U);
            return real->lo < low ? TSD_BAD : TSD_OK;
        }
        if (low == 0x20U) return TSD_MISSING;
        if ((low & 4095U) != (i == 0U ? 0x0fU :
                               i == 1U ? 0x0bU : i == 2U ? 0x07U : 0U))
            return TSD_BAD;
        next = low & ~4095U;
        origin = next;
    }
    return TSD_BAD;
}
