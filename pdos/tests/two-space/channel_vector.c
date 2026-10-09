/* SPDX-License-Identifier: MIT; independent three-record CKD chain vectors. */
#include "twospace_channel.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static void word(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static unsigned int value(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
int main(void)
{
    unsigned char *ram=(unsigned char *)calloc(1,TSC_REGION_BYTES);TSCSTATE s;unsigned char *p,*irb,*data;unsigned int i;
    assert(ram&&!TSCINIT(&s,ram,TSC_REGION_BYTES,0U));
    assert(!TSCBUILDV(&s,1U,2U,3U,18452U,3U));p=ram+TSC_CCW_OFFSET;irb=TSCIRB(&s);data=TSCDATA(&s);
    assert(p[0]==7U&&p[1]==0x40U&&value(p+4U)==0x300U);
    for(i=0U;i<3U;++i){unsigned int offset=8U+24U*i;
        assert(p[offset]==0x31U&&value(p+offset+4U)==0x310U+5U*i);
        assert(ram[0x314U+5U*i]==2U+i);
        assert(p[offset+8U]==8U&&value(p+offset+12U)==0x200U+offset);
        assert(p[offset+16U]==0x1eU&&p[offset+17U]==(i==2U?0x20U:0x60U));
        assert(p[offset+18U]==0x48U&&p[offset+19U]==0x1cU);
        assert(value(p+offset+20U)==0x1000U+18460U*i);
        data[i*18460U+1U]=1U;data[i*18460U+3U]=2U;data[i*18460U+4U]=(unsigned char)(3U+i);
        data[i*18460U+6U]=0x48U;data[i*18460U+7U]=0x14U;
    }
    irb[8U]=12U;word(irb+4U,0x250U);
    assert(!TSCCHECKV(&s,1U,2U,3U,18452U,3U));
    data[18460U+5U]=1U;assert(TSCCHECKV(&s,1U,2U,3U,18452U,3U)==TSC_IO);data[18460U+5U]=0U;
    data[2U*18460U+7U]=0x13U;assert(TSCCHECKV(&s,1U,2U,3U,18452U,3U)==TSC_IO);data[2U*18460U+7U]=0x14U;
    irb[8U]=13U;word(irb+4U,0x220U);assert(TSCCHECKV(&s,1U,2U,3U,18452U,3U)==TSC_IO);
    assert(TSCBUILDV(&s,1U,2U,255U,18452U,3U)==TSC_BAD);
    assert(TSCBUILDV(&s,1U,2U,3U,32767U,3U)==TSC_BAD);
    s.occupied=1U;assert(TSCBUILDV(&s,1U,2U,3U,18452U,3U)==TSC_BAD);
    free(ram);return 0;
}
