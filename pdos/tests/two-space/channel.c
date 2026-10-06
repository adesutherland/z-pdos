/* SPDX-License-Identifier: MIT
 * Check the real-address CKD wire contract independently of the guest I/O.
 */
#include <stdlib.h>
#include <string.h>
#include "twospace_channel.h"

static unsigned int word(const unsigned char *p)
{
    return ((unsigned int)p[0]<<24) | ((unsigned int)p[1]<<16) |
           ((unsigned int)p[2]<<8) | (unsigned int)p[3];
}

int main(void)
{
    unsigned char *real, *base, *irb;
    unsigned int got=0U;
    TSCSTATE channel;
    real=(unsigned char *)malloc(0x1000000U);
    if (!real) return 1;
    memset(real,0xa5,0x1000000U);
    if (TSCINIT(&channel,real,0x1000000U,0x180000U) != TSC_OK ||
        TSCINIT(&channel,real,0x1000000U,0xff1000U) != TSC_BAD ||
        TSCINIT(&channel,real,0x1000000U,0x180001U) != TSC_BAD)
        return 2;
    if (TSCBUILDREAD(&channel,0U,0U,3U,0x0eU,80U) != TSC_OK)
        return 3;
    base=real+0x180000U;
    if (word(base+4U) != 0x0080ff00U ||
        word(base+8U) != 0x180200U ||
        memcmp(base+0x200U,"\x07\x40\x00\x06\x00\x18\x03\x00",8U) ||
        memcmp(base+0x208U,"\x31\x40\x00\x05\x00\x18\x03\x10",8U) ||
        memcmp(base+0x210U,"\x08\x00\x00\x00\x00\x18\x02\x08",8U) ||
        memcmp(base+0x218U,"\x0e\x20\x00\x50\x00\x18\x10\x00",8U) ||
        base[0x310U] || base[0x311U] || base[0x312U] ||
        base[0x313U] || base[0x314U] != 3U ||
        base[0x1000U] != 0U || real[0x17ffffU] != 0xa5U ||
        real[0x190000U] != 0xa5U)
        return 4;
    if (TSCBUILDREAD(&channel,0U,15U,3U,0x0eU,80U) != TSC_BAD ||
        TSCBUILDREAD(&channel,0U,0U,0U,0x0eU,80U) != TSC_BAD ||
        TSCBUILDREAD(&channel,0U,0U,3U,0x01U,80U) != TSC_BAD ||
        TSCBUILDREAD(&channel,0U,0U,3U,0x0eU,18453U) != TSC_BAD)
        return 5;
    irb=TSCIRB(&channel);
    irb[8]=0x0cU; irb[9]=0U; irb[10]=0U; irb[11]=16U;
    irb[4]=0U; irb[5]=0x18U; irb[6]=0x02U; irb[7]=0x20U;
    if (TSCCHECKREAD(&channel,80U,&got) != TSC_OK || got != 64U)
        return 6;
    irb[9]=1U;
    if (TSCCHECKREAD(&channel,80U,&got) != TSC_IO) return 7;
    irb[9]=0U; irb[11]=81U;
    if (TSCCHECKREAD(&channel,80U,&got) != TSC_IO) return 8;
    memset(TSCDATA(&channel),0xd2,1773U);
    if (TSCBUILDCONSWRITE(&channel,1773U) != TSC_OK ||
        TSCBUILDCONSWRITE(&channel,2049U) != TSC_BAD ||
        word(base+4U) != 0x0080ff00U ||
        word(base+8U) != 0x180200U ||
        memcmp(base+0x200U,"\x01\x20\x06\xed\x00\x18\x10\x00",8U) ||
        TSCDATA(&channel)[0] != 0xd2U ||
        TSCSCHIB(&channel) != base+0x400U)
        return 9;
    irb[8]=0x0cU; irb[9]=0U; irb[10]=0U; irb[11]=0U;
    irb[4]=0U; irb[5]=0x18U; irb[6]=0x02U; irb[7]=0x08U;
    if (TSCCHECKWRITE(&channel) != TSC_OK) return 10;
    irb[11]=1U;
    if (TSCCHECKWRITE(&channel) != TSC_IO) return 11;
    if (TSCBUILDCONSREAD(&channel,252U) != TSC_OK ||
        TSCBUILDCONSREAD(&channel,257U) != TSC_BAD ||
        memcmp(base+0x200U,"\x06\x20\x00\xfc\x00\x18\x10\x00",8U) ||
        TSCDATA(&channel)[0] != 0U)
        return 12;
    irb[8]=0x0cU; irb[9]=0U; irb[10]=0U; irb[11]=244U;
    irb[4]=0U; irb[5]=0x18U; irb[6]=0x02U; irb[7]=0x08U;
    if (TSCCHECKCONSREAD(&channel,252U,&got) != TSC_OK || got != 8U)
        return 13;
    irb[9]=1U;
    if (TSCCHECKCONSREAD(&channel,252U,&got) != TSC_IO) return 14;
    free(real);
    return 0;
}
