/* SPDX-License-Identifier: MIT */
#include "twospace_cmsfile.h"

static unsigned int word(const unsigned char *p)
{
    return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|
           ((unsigned int)p[2]<<8)|(unsigned int)p[3];
}

int TSIFHEADER(const unsigned char *first, unsigned int length,
               unsigned int expected_profile, TSIFINFO *info)
{
    static const unsigned char magic[8] =
        {0x50U,0x44U,0x43U,0x4dU,0x53U,0x46U,0x30U,0x31U};
    unsigned int i, payload, source, records;
    if (!first || !info || length<TSIF_BLOCK ||
        (expected_profile!=24U && expected_profile!=31U))
        return TSIF_BAD;
    for (i=0U; i<8U && first[i]==magic[i]; ++i) {}
    if (i!=8U || word(first+8U)!=expected_profile) return TSIF_BAD;
    payload=word(first+12U); source=word(first+16U);
    records=word(first+20U);
    if (!payload || payload>TSIF_LIMIT-64U ||
        !source || source>payload || !records || records>65535U ||
        first[60U] || first[61U] || first[62U] || first[63U])
        return TSIF_BAD;
    info->profile=expected_profile;
    info->payload_bytes=payload;
    info->source_bytes=source;
    info->records=records;
    info->blocks=(64U+payload+TSIF_BLOCK-1U)/TSIF_BLOCK;
    return TSIF_OK;
}

int TSIFVALIDATE(const unsigned char *stage, unsigned int length,
                 unsigned int expected_profile, TSIFINFO *info)
{
    TSIFINFO parsed;
    unsigned int i, cursor, used, size, sum=0U, hash;
    if (!stage || !info ||
        TSIFHEADER(stage,length,expected_profile,&parsed)!=TSIF_OK ||
        length!=parsed.blocks*TSIF_BLOCK) return TSIF_BAD;
    used=64U+parsed.payload_bytes;
    for (i=used; i<length; ++i) if (stage[i]) return TSIF_BAD;
    hash=0x811c9dc5U;
    for (i=64U; i<used; ++i)
        hash=(hash^(unsigned int)stage[i])*0x01000193U;
    if (hash!=word(stage+56U)) return TSIF_BAD;
    cursor=64U;
    for (i=0U; i<parsed.records; ++i) {
        if (cursor>used-2U) return TSIF_BAD;
        size=((unsigned int)stage[cursor]<<8)|stage[cursor+1U];
        if (!size || size>256U || size>used-cursor-2U)
            return TSIF_BAD;
        sum+=size;
        cursor+=size+2U;
    }
    if (cursor!=used || sum!=parsed.source_bytes) return TSIF_BAD;
    *info=parsed;
    return TSIF_OK;
}
