/* SPDX-License-Identifier: MIT */
#include "twospace_dataset.h"

static unsigned int word16(const unsigned char *p)
{ return ((unsigned int)p[0]<<8) | (unsigned int)p[1]; }

static int name_is(const unsigned char *record, const unsigned char *name,
                   unsigned int length)
{
    unsigned int i;
    for (i=0U; i<44U; ++i)
        if (record[i] != (i<length ? name[i] : 0x40U)) return 0;
    return 1;
}

int TSKWITHIN(const TSKEXTENT *e, unsigned int cylinder,
              unsigned int head)
{
    if (!e || head >= 15U) return 0;
    if (cylinder < e->start_cylinder || cylinder > e->end_cylinder)
        return 0;
    if (cylinder == e->start_cylinder && head < e->start_head) return 0;
    if (cylinder == e->end_cylinder && head > e->end_head) return 0;
    return 1;
}

int TSKDSCB(TSKREAD read_record, void *context,
            const unsigned char *name, unsigned int name_length,
            TSKEXTENT *extent, unsigned char data[96])
{
    const unsigned char *record;
    unsigned int cylinder, head, number, i, visits=0U;
    TSKEXTENT found;
    int count;
    if (!read_record || !name || !extent || !name_length ||
        name_length > 44U) return TSK_BAD;
    for (i=0U; i<name_length; ++i)
        if (name[i] == 0U || name[i] == 0x40U) return TSK_BAD;
    count=read_record(context,0U,0U,3U,80U,&record);
    if (count < 24 || !record || record[4] != 0xe5U ||
        record[5] != 0xd6U || record[6] != 0xd3U ||
        record[7] != 0xf1U) return TSK_CORRUPT;
    cylinder=word16(record+15U);
    head=word16(record+17U);
    number=record[19U];
    if (cylinder != 1U || head >= 15U || !number) return TSK_CORRUPT;
    while (cylinder <= 2U && visits < 1500U) {
        ++visits;
        count=read_record(context,cylinder,head,number,140U,&record);
        if (count < 0) {
            number=1U;
            ++head;
            if (head == 15U) { head=0U; ++cylinder; }
            continue;
        }
        if (count < 115 || !record) return TSK_CORRUPT;
        if (record[0] == 0U) return TSK_ABSENT;
        if (record[44] == 0xf1U && name_is(record,name,name_length)) {
            if (record[59] != 1U) return TSK_CORRUPT;
            found.start_cylinder=word16(record+107U);
            found.start_head=word16(record+109U);
            found.end_cylinder=word16(record+111U);
            found.end_head=word16(record+113U);
            found.record_format=record[84U];
            found.block_length=word16(record+86U);
            found.logical_length=word16(record+88U);
            found.organisation=word16(record+82U);
            if (found.start_cylinder < 3U ||
                found.end_cylinder >= 100U ||
                found.start_head >= 15U || found.end_head >= 15U ||
                found.start_cylinder > found.end_cylinder ||
                (found.start_cylinder == found.end_cylinder &&
                 found.start_head > found.end_head) ||
                !found.block_length || found.block_length > 18452U)
                return TSK_CORRUPT;
            *extent=found;
            if(data)for(i=0U;i<96U;++i)data[i]=record[44U+i];
            return TSK_OK;
        }
        if (number == 255U) {
            number=1U; ++head;
            if (head == 15U) { head=0U; ++cylinder; }
        } else ++number;
    }
    /* Exhausting the bounded VTOC without its zero terminator is not a
       trustworthy negative lookup; a failed channel read may be corruption. */
    return TSK_CORRUPT;
}

int TSKFIND(TSKREAD read_record,void *context,const unsigned char *name,
             unsigned int bytes,TSKEXTENT *extent)
{return TSKDSCB(read_record,context,name,bytes,extent,0);}
