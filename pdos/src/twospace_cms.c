/* SPDX-License-Identifier: MIT */
#include "twospace_cms.h"

static unsigned int word(const unsigned char *p)
{
    return ((unsigned int)p[0]<<24) | ((unsigned int)p[1]<<16) |
           ((unsigned int)p[2]<<8) | (unsigned int)p[3];
}
static unsigned int half(const unsigned char *p)
{ return ((unsigned int)p[0]<<8) | (unsigned int)p[1]; }

int TSHHEADER(const unsigned char *block, unsigned int length,
              unsigned int expected_profile, TSHINFO *info)
{
    static const unsigned char magic[8] =
        {0x50U,0x44U,0x43U,0x4dU,0x53U,0x4dU,0x30U,0x31U};
    const unsigned char *h;
    unsigned int i, module_bytes, image_bytes, profile, entry, origin, end;
    unsigned int records, relocations;
    if (!block || !info || length < 146U || length > 18452U ||
        (expected_profile != 24U && expected_profile != 31U)) return TSH_BAD;
    for (i=0U; i<8U; ++i) if (block[i] != magic[i]) return TSH_BAD;
    profile=word(block+8U); module_bytes=word(block+12U);
    image_bytes=word(block+16U); records=word(block+20U);
    if (profile != expected_profile || module_bytes < 82U ||
        module_bytes > 9U*1024U*1024U-64U ||
        !image_bytes || image_bytes > 8U*1024U*1024U ||
        !records || records > module_bytes/3U ||
        word(block+60U) != 0U || half(block+64U) != 80U)
        return TSH_BAD;
    h=block+66U;
    entry=word(h); origin=word(h+4U); end=word(h+8U);
    if (origin < 0x20000U || (origin & 7U) || end != word(h+12U) ||
        end <= origin || end-origin != image_bytes ||
        entry < origin || entry >= end || (end & 7U)) return TSH_BAD;
    relocations=0U;
    if (profile == 24U) {
        if (origin != 0x20000U || end > 0x200000U ||
            h[40U] || h[41U] || h[43U] != 0x80U ||
            records != 1U+(image_bytes+65534U)/65535U) return TSH_BAD;
    } else {
        relocations=half(h+48U);
        if (end >= 0x80000000U || h[32U] != 0x48U ||
            h[34U] != 2U || h[40U] != 0U || h[41U] != 3U ||
            h[43U] != 3U || h[44U] != 0xa0U || h[46U] != 0x80U ||
            !relocations || h[45U] != (relocations > 1U ? 0xf0U : 0xd0U) ||
            records != 2U+(image_bytes+65534U)/65535U+relocations)
            return TSH_BAD;
    }
    info->profile=profile;
    info->module_bytes=module_bytes;
    info->image_bytes=image_bytes;
    info->records=records;
    info->entry=entry;
    info->origin=origin;
    info->end=end;
    info->relocation_records=relocations;
    return TSH_OK;
}

static int record(const unsigned char *module, unsigned int bytes,
                  unsigned int *cursor, unsigned int *data,
                  unsigned int *size)
{
    unsigned int at=*cursor, n;
    if (at > bytes || bytes-at < 2U) return TSH_BAD;
    n=half(module+at);
    if (!n || n > bytes-at-2U) return TSH_BAD;
    *data=at+2U; *size=n; *cursor=at+2U+n;
    return TSH_OK;
}

static unsigned int image_word(const unsigned char *module,
                               const unsigned int *offsets,
                               unsigned int where)
{
    unsigned int i, n, value=0U;
    for (i=0U; i<4U; ++i) {
        n=where+i;
        value=(value<<8) | module[offsets[n/65535U]+n%65535U];
    }
    return value;
}

int TSHVALIDATE(const unsigned char *staged, unsigned int length,
                unsigned int expected_profile, TSHINFO *info)
{
    const unsigned char *module;
    unsigned int offsets[129], cursor=0U, at, size, i, j, count=0U;
    unsigned int image_count, remaining, fnv=0x811c9dc5U, previous, address;
    unsigned int old, expected, module_end;
    TSHINFO parsed;
    if (!staged || !info || length < 146U ||
        TSHHEADER(staged,length < 18452U ? length : 18452U,
                  expected_profile,&parsed) != TSH_OK ||
        parsed.module_bytes > length-64U) return TSH_BAD;
    module_end=64U+parsed.module_bytes;
    for (i=module_end; i<length; ++i)
        if (staged[i]) return TSH_BAD;
    module=staged+64U;
    for (i=0U; i<parsed.module_bytes; ++i)
        fnv=(fnv ^ module[i])*0x01000193U;
    if (fnv != word(staged+56U) ||
        record(module,parsed.module_bytes,&cursor,&at,&size) != TSH_OK ||
        size != 80U) return TSH_BAD;
    ++count;
    image_count=(parsed.image_bytes+65534U)/65535U;
    if (image_count > 129U) return TSH_BAD;
    remaining=parsed.image_bytes;
    for (i=0U; i<image_count; ++i) {
        expected=remaining < 65535U ? remaining : 65535U;
        if (record(module,parsed.module_bytes,&cursor,&at,&size) != TSH_OK ||
            size != expected) return TSH_BAD;
        offsets[i]=at;
        remaining-=size;
        ++count;
    }
    if (remaining) return TSH_BAD;
    if (expected_profile == 31U) {
        static const unsigned char map_magic[8] =
            {0xc5U,0xd3U,0xc6U,0xd7U,0xd6U,0xc3U,0x40U,0x40U};
        if (record(module,parsed.module_bytes,&cursor,&at,&size) != TSH_OK ||
            size != 72U ||
            word(module+at+12U) != (parsed.entry | 0x80000000U) ||
            word(module+at+20U) != parsed.origin) return TSH_BAD;
        for (i=0U; i<8U; ++i)
            if (module[at+i] != map_magic[i]) return TSH_BAD;
        ++count;
        previous=parsed.end;
        for (i=0U; i<parsed.relocation_records; ++i) {
            if (record(module,parsed.module_bytes,&cursor,&at,&size) != TSH_OK ||
                size%5U || (i+1U<parsed.relocation_records && size!=65535U))
                return TSH_BAD;
            for (j=0U; j<size; j+=5U) {
                address=word(module+at+j+1U);
                if (module[at+j] != 3U || address < parsed.origin ||
                    address > parsed.end-4U || address >= previous)
                    return TSH_BAD;
                previous=address;
                old=image_word(module,offsets,address-parsed.origin);
                if (old < parsed.origin || old > parsed.end) return TSH_BAD;
            }
            ++count;
        }
    }
    if (count != parsed.records || cursor != parsed.module_bytes)
        return TSH_BAD;
    *info=parsed;
    return TSH_OK;
}
