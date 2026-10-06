/* SPDX-License-Identifier: MIT */
#include <string.h>
#include "twospace_dataset.h"

static const unsigned char target[] =
    {0xd2U,0xc3U,0xd6U,0xd9U,0xc5U,0x4bU,0xc2U,0xc9U,0xd5U};
static unsigned char label[80], other[140], match[140], endmark[140];
static unsigned int fail_volume;
static unsigned int fail_vtoc;

static int read_record(void *unused, unsigned int c, unsigned int h,
                       unsigned int r, unsigned int cap,
                       const unsigned char **data)
{
    (void)unused;
    if (c == 0U && h == 0U && r == 3U && cap == 80U) {
        *data=label;
        return fail_volume ? -1 : 80;
    }
    if (c == 1U && h == 0U && cap == 140U) {
        if (fail_vtoc) { *data=0; return -1; }
        if (r == 1U) { *data=other; return 140; }
        if (r == 2U) { *data=match; return 140; }
        if (r == 3U) { *data=endmark; return 140; }
    }
    *data=0;
    return -1;
}

int main(void)
{
    TSKEXTENT extent, unchanged;
    unsigned int i;
    memset(label,0,sizeof label);
    memset(other,0x40,sizeof other);
    memset(match,0x40,sizeof match);
    memset(endmark,0,sizeof endmark);
    label[4]=0xe5U; label[5]=0xd6U; label[6]=0xd3U; label[7]=0xf1U;
    label[15]=0U; label[16]=1U; label[17]=label[18]=0U;
    label[19]=1U;
    other[44]=0xf4U;
    for (i=0U; i<sizeof target; ++i) match[i]=target[i];
    match[44]=0xf1U; match[59]=1U;
    match[84]=0x80U; match[86]=0x48U; match[87]=0x14U;
    match[88]=0x48U; match[89]=0x14U;
    match[107]=0U; match[108]=3U; match[109]=match[110]=0U;
    match[111]=0U; match[112]=4U; match[113]=match[114]=0U;
    memset(&extent,0x5a,sizeof extent);
    if (TSKFIND(read_record,0,target,sizeof target,&extent) != TSK_OK ||
        extent.start_cylinder != 3U || extent.end_cylinder != 4U ||
        extent.block_length != 18452U || extent.record_format != 0x80U ||
        !TSKWITHIN(&extent,3U,0U) || !TSKWITHIN(&extent,4U,0U) ||
        TSKWITHIN(&extent,2U,14U) || TSKWITHIN(&extent,4U,15U)) return 1;
    unchanged=extent;
    match[59]=2U;
    if (TSKFIND(read_record,0,target,sizeof target,&extent) != TSK_CORRUPT ||
        memcmp(&extent,&unchanged,sizeof extent)) return 2;
    match[59]=1U;
    match[112]=100U;
    if (TSKFIND(read_record,0,target,sizeof target,&extent) != TSK_CORRUPT ||
        memcmp(&extent,&unchanged,sizeof extent)) return 3;
    match[112]=4U;
    fail_volume=1U;
    if (TSKFIND(read_record,0,target,sizeof target,&extent) != TSK_CORRUPT ||
        memcmp(&extent,&unchanged,sizeof extent)) return 4;
    fail_volume=0U;
    fail_vtoc=1U;
    if (TSKFIND(read_record,0,target,sizeof target,&extent) != TSK_CORRUPT ||
        memcmp(&extent,&unchanged,sizeof extent)) return 5;
    fail_vtoc=0U;
    match[0]=0xc2U;
    if (TSKFIND(read_record,0,target,sizeof target,&extent) != TSK_ABSENT ||
        memcmp(&extent,&unchanged,sizeof extent)) return 6;
    return 0;
}
