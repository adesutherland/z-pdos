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

static unsigned int allocate(TSDSTATE *s, unsigned int bytes,
                             unsigned int invalid)
{
    unsigned int real = s->pool_real + s->used, i;
    for (i = 0; i < bytes; i += 8U) {
        write32(s->pool + s->used + i, 0U);
        write32(s->pool + s->used + i + 4U, invalid);
    }
    s->used += bytes;
    return real;
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
    s->asce_lo = allocate(s, 16384U, 0x20U) | 0x0fU;
    return TSD_OK;
}

int TSDMAP(TSDSTATE *s, TSPADDR va, TSPADDR pa)
{
    unsigned int indexes[5], origins[5], absent[4], need, i, next;
    unsigned char *p;
    TSPADDR value;
    if (!s || !s->pool || !s->asce_lo ||
        (va.lo & 4095U) || (pa.lo & 4095U) ||
        (pa.hi == 0U && pa.lo >= s->pool_real &&
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
        next = child(p, i == 3U ? 0x400U : 0x20U);
        if (!next) {
            absent[i] = 1U;
            need += i == 3U ? 4096U : 16384U;
        } else {
            absent[i] = 0U;
            if (next < s->pool_real ||
                next - s->pool_real >= s->used) return TSD_BAD;
            origins[i+1] = next;
        }
    }
    if (!need) {
        p = entry(s, origins[4], indexes[4]);
        if (read32(p) != 0U || read32(p+4) != 0x400U)
            return TSD_EXISTS;
    }
    if (need > s->capacity - s->used) return TSD_FULL;
    for (i = 0; i < 4U; ++i) {
        if (absent[i]) {
            next = allocate(s, i == 3U ? 4096U : 16384U,
                            i == 3U ? 0x400U : 0x20U);
            value.hi = 0U;
            value.lo = next | (i == 0U ? 0x0fU :
                               i == 1U ? 0x0bU : i == 2U ? 0x07U : 0U);
            write64(entry(s, origins[i], indexes[i]), value);
            origins[i+1] = next;
        }
    }
    write64(entry(s, origins[4], indexes[4]), pa);
    return TSD_OK;
}
