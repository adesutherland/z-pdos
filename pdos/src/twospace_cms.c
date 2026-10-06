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
