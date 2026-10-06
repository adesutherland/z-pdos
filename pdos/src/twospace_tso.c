/* SPDX-License-Identifier: MIT
 * Native load-module subset derived from Paul Edwards's public-domain
 * pdosutil.c. Keep the released loader separate while qualifying K/U loading.
 */
#include "twospace_tso.h"

#define TST_SECTIONS 256U
typedef struct { unsigned int id, base; } TSTSECTION;

static unsigned int half(const unsigned char *p)
{ return ((unsigned int)p[0]<<8) | p[1]; }
static unsigned int triple(const unsigned char *p)
{ return ((unsigned int)p[0]<<16) | ((unsigned int)p[1]<<8) | p[2]; }
static unsigned int word(const unsigned char *p)
{
    return ((unsigned int)p[0]<<24) | ((unsigned int)p[1]<<16) |
           ((unsigned int)p[2]<<8) | p[3];
}
static void put_word(unsigned char *p, unsigned int value)
{
    p[0]=(unsigned char)(value>>24); p[1]=(unsigned char)(value>>16);
    p[2]=(unsigned char)(value>>8); p[3]=(unsigned char)value;
}

static int stage_header(const unsigned char *block, unsigned int length,
                        unsigned int mode, unsigned int *raw_bytes,
                        unsigned int *blocks)
{
    static const unsigned char prefix[5]={
        0x50U,0x44U,0x54U,0x53U,0x4fU};
    unsigned int i, bytes, count;
    if (!block || !raw_bytes || !blocks || length<TST_BLOCK)
        return TST_BAD;
    if (mode!=24U && mode!=31U && mode!=64U) return TST_BAD;
    for (i=0U; i<5U; ++i)
        if (block[i]!=prefix[i]) return TST_BAD;
    if (block[5U]!=(mode==24U ? 0x32U : mode==31U ? 0x33U : 0x36U) ||
        block[6U]!=(mode==24U ? 0x34U : mode==31U ? 0x31U : 0x34U) ||
        block[7U]!=0x01U) return TST_BAD;
    bytes=word(block+8U); count=word(block+12U);
    if (bytes<56U+280U+292U+16U || bytes>TST_MAX_RAW ||
        count!=(bytes+64U+TST_BLOCK-1U)/TST_BLOCK || !count ||
        word(block+20U)!=0U) return TST_BAD;
    for (i=24U; i<64U; ++i) if (block[i]) return TST_BAD;
    *raw_bytes=bytes; *blocks=count;
    return TST_OK;
}

int TSTSTAGEHEADER(const unsigned char *block, unsigned int length,
                   unsigned int *raw_bytes, unsigned int *blocks)
{ return stage_header(block,length,31U,raw_bytes,blocks); }

int TSTSTAGEHEADER24(const unsigned char *block, unsigned int length,
                     unsigned int *raw_bytes, unsigned int *blocks)
{ return stage_header(block,length,24U,raw_bytes,blocks); }

int TSTSTAGEHEADER64(const unsigned char *block, unsigned int length,
                     unsigned int *raw_bytes, unsigned int *blocks)
{ return stage_header(block,length,64U,raw_bytes,blocks); }

static int stage_validate(const unsigned char *stage, unsigned int length,
                          unsigned int expected_mode, TSTINFO *info)
{
    unsigned int bytes, blocks, i, fnv=0x811c9dc5U;
    if (!stage || !info ||
        stage_header(stage,length,expected_mode,&bytes,&blocks)!=TST_OK ||
        length!=blocks*TST_BLOCK) return TST_BAD;
    for (i=0U; i<bytes; ++i)
        fnv=(fnv^stage[64U+i])*0x01000193U;
    if (fnv!=word(stage+16U)) return TST_BAD;
    for (i=64U+bytes; i<length; ++i) if (stage[i]) return TST_BAD;
    return TSTHEADER(stage+64U,bytes,expected_mode,info);
}

int TSTSTAGEVALIDATE(const unsigned char *stage, unsigned int length,
                     unsigned int expected_mode, TSTINFO *info)
{ return stage_validate(stage,length,expected_mode,info); }

int TSTHEADER(const unsigned char *raw, unsigned int bytes,
              unsigned int expected_mode, TSTINFO *info)
{
    unsigned int at=0U, rec=0U, size, max_record=0U;
    unsigned int flags=0U, entry=0U, fnv=0x811c9dc5U, i;
    if (!raw || !info || sizeof(unsigned int)!=4U ||
        bytes<56U+280U+292U+16U ||
        bytes>TST_MAX_RAW ||
        (expected_mode!=24U && expected_mode!=31U && expected_mode!=64U))
        return TST_BAD;
    for (i=0U; i<bytes; ++i) fnv=(fnv^raw[i])*0x01000193U;
    while (at<bytes) {
        if (bytes-at<2U) return TST_BAD;
        size=half(raw+at);
        if (size<2U || size>bytes-at || size>18452U) return TST_BAD;
        ++rec;
        if (size>max_record) max_record=size;
        if (rec==1U) {
            if (size!=56U || raw[at+4U]!=0U ||
                raw[at+5U]!=0xcaU || raw[at+6U]!=0x6dU ||
                raw[at+7U]!=0x0fU) return TST_BAD;
        } else if (rec==3U) {
            unsigned int q=at+24U, directory_size;
            if (size<292U) return TST_BAD;
            directory_size=half(raw+q);
            if (directory_size<33U || directory_size>size-26U)
                return TST_BAD;
            entry=triple(raw+q+29U);
            flags=raw[q+33U];
            if ((expected_mode==24U && (flags&3U)!=0U) ||
                (expected_mode==31U && (flags&3U)!=2U) ||
                (expected_mode==64U && (flags&3U)!=1U) ||
                (expected_mode!=24U && !(flags&0x10U)) ||
                (flags&0x20U))
                return TST_BAD;
        }
        at+=size;
    }
    if (rec<4U || half(raw+bytes-16U)!=16U)
        return TST_BAD;
    info->raw_bytes=bytes;
    info->records=rec;
    info->max_record=max_record;
    info->mode=expected_mode;
    info->rmode_any=(flags&0x10U)!=0U;
    info->flags=flags;
    info->entry_offset=entry;
    info->image_bytes=0U;
    info->input_fnv=fnv;
    return TST_OK;
}

static int rld31(unsigned char *image, unsigned int text_bytes,
                 unsigned int base, const unsigned char *rld,
                 unsigned int length, const TSTSECTION *sections,
                 unsigned int section_count)
{
    unsigned int at=16U, source=0U, i, size, address, old;
    int continuing=0, found;
    if (length<16U) return TST_BAD;
    while (at<length) {
        if (!continuing) {
            if (length-at<4U) return TST_BAD;
            source=half(rld+at+2U);
            found=0;
            for (i=0U; i<section_count; ++i)
                if (sections[i].id==source) { found=1; break; }
            if (!found) return TST_BAD;
            at+=4U;
        }
        if (length-at<4U || (rld[at]&2U)) return TST_BAD;
        size=((rld[at]&0x0cU)>>2)+1U;
        if (rld[at]&0x40U) size+=4U;
        if (size!=3U && size!=4U) return TST_BAD;
        if (size==3U && base>0xffffffU) return TST_BAD;
        continuing=(rld[at]&1U)!=0U;
        ++at;
        address=triple(rld+at);
        at+=3U;
        if (address>text_bytes || size>text_bytes-address ||
            (size==3U && !address)) return TST_BAD;
        old=word(image+address-(size==3U ? 1U : 0U));
        put_word(image+address-(size==3U ? 1U : 0U),old+base);
    }
    return at==length ? TST_OK : TST_BAD;
}

static int image_low(const unsigned char *raw, unsigned int bytes,
                     unsigned int base, unsigned char *image,
                     unsigned int capacity, unsigned int mode, TSTINFO *info)
{
    TSTSECTION sections[TST_SECTIONS];
    TSTINFO parsed;
    unsigned int at=0U, rec=0U, size, q, remaining, sublength;
    unsigned int section_count=0U, first, count, i, offset;
    unsigned int nexttext=0U, lastend=0U, imageend=0U;
    int last_type=-1, type, done=0;
    if (!image || !info || !capacity || capacity>TST_MAX_IMAGE ||
        (mode!=24U && mode!=31U && mode!=64U) ||
        (base&4095U) || base<(mode==24U ? 0x10000U : 0x1000000U) ||
        TSTHEADER(raw,bytes,mode,&parsed)!=TST_OK) return TST_BAD;
    for (i=0U; i<capacity; ++i) image[i]=0U;
    while (at<bytes && !done) {
        size=half(raw+at);
        ++rec;
        if (rec>3U) {
            if (size<16U) return TST_BAD;
            q=at+14U;
            remaining=size-14U;
            while (remaining>=2U) {
                sublength=half(raw+q);
                q+=2U; remaining-=2U;
                if (sublength>remaining) return TST_BAD;
                if (!sublength) break;
                remaining-=sublength;
                type=raw[q];
                if (last_type==1 || last_type==3 ||
                    last_type==13 || last_type==15) {
                    if (nexttext<lastend || nexttext>capacity ||
                        sublength>capacity-nexttext)
                        return TST_BAD;
                    for (i=0U; i<sublength; ++i)
                        image[nexttext+i]=raw[q+i];
                    lastend=nexttext+sublength;
                    if (lastend>imageend) imageend=lastend;
                    type=-1;
                    if (last_type==13 || last_type==15) done=1;
                } else if (type==0x20) {
                    if (sublength<8U || half(raw+q+6U)>sublength-8U ||
                        (half(raw+q+6U)&15U)) return TST_BAD;
                    first=half(raw+q+4U);
                    count=half(raw+q+6U)/16U;
                    for (i=0U; i<count; ++i) {
                        const unsigned char *item=raw+q+8U+i*16U;
                        offset=triple(item+9U);
                        if (item[8U]!=0U && item[8U]!=4U) continue;
                        if (section_count==TST_SECTIONS ||
                            offset>capacity ||
                            triple(item+13U)>capacity-offset)
                            return TST_BAD;
                        sections[section_count].id=first+i;
                        sections[section_count].base=offset;
                        ++section_count;
                        if (offset+triple(item+13U)>imageend)
                            imageend=offset+triple(item+13U);
                    }
                } else if (type==1 || type==13) {
                    if (sublength<16U || raw[q+8U]!=6U)
                        return TST_BAD;
                    nexttext=triple(raw+q+9U);
                } else if (type==2 || type==14) {
                    if (rld31(image,lastend,base,raw+q,sublength,
                              sections,section_count)!=TST_OK)
                        return TST_BAD;
                    if (type==14) done=1;
                } else if (type==3 || type==15) {
                    unsigned int rld_length;
                    if (sublength<16U) return TST_BAD;
                    rld_length=half(raw+q+6U)+16U;
                    if (rld_length>sublength ||
                        rld31(image,lastend,base,raw+q,rld_length,
                              sections,section_count)!=TST_OK ||
                        raw[q+8U]!=6U) return TST_BAD;
                    nexttext=triple(raw+q+9U);
                } else if (type!=0x80) {
                    /* The selected native subset has no undefined controls. */
                    return TST_BAD;
                }
                last_type=type;
                if (done) break;
                q+=sublength;
                if (remaining==0U) break;
                if (remaining<12U) return TST_BAD;
                q+=10U; remaining-=10U;
            }
        }
        at+=size;
    }
    if (!done || at!=bytes-16U || !imageend || imageend>capacity ||
        parsed.entry_offset>=imageend ||
        base>(mode==24U ? 0x1000000U-imageend :
                             0x7fffffffU-imageend)) return TST_BAD;
    parsed.image_bytes=imageend;
    *info=parsed;
    return TST_OK;
}

int TSTIMAGE31(const unsigned char *raw, unsigned int bytes,
               unsigned int base, unsigned char *image,
               unsigned int capacity, TSTINFO *info)
{
    return image_low(raw,bytes,base,image,capacity,31U,info);
}

int TSTIMAGE24(const unsigned char *raw, unsigned int bytes,
               unsigned int base, unsigned char *image,
               unsigned int capacity, TSTINFO *info)
{
    /* The selected AMODE24/RMODE24 member has low-only AL3/AL4 records.
     * Its whole image, including save areas, must end below 16 MiB. */
    return image_low(raw,bytes,base,image,capacity,24U,info);
}

int TSTIMAGE64ANY(const unsigned char *raw, unsigned int bytes,
                  unsigned int base, unsigned char *image,
                  unsigned int capacity, TSTINFO *info)
{
    /* AMODE64/RMODE ANY still carries AL4 relocations in a low-resident
     * classic member. The high-resident module has a distinct AL8 format. */
    return image_low(raw,bytes,base,image,capacity,64U,info);
}
