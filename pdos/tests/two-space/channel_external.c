/* SPDX-License-Identifier: MIT; independent data-chain and residual vectors. */
#include "twospace_channel.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static void put(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
int main(void)
{
    unsigned char *ram=(unsigned char *)malloc(0x100000U),*ccw,*irb;TSCSTATE s;unsigned int i,got=999U;
    assert(ram);memset(ram,0x5aU,0x100000U);assert(!TSCINIT(&s,ram,0x100000U,0U));
    assert(!TSCBUILDEXT(&s,17U,0x40000U,131072U,0U));ccw=ram+0x200U;irb=TSCIRB(&s);
    for(i=0U;i<4U;++i){
        unsigned char expected[8]={17U,0xa0U,0x80U,0U,0U,4U,0U,0U};
        expected[1U]=(unsigned char)(i==3U?0x20U:0xa0U);put(expected+4U,0x40000U+32768U*i);
        assert(!memcmp(ccw+8U*i,expected,8U));
    }
    assert(ram[0x40000U]==0x5aU&&ram[0x5ffffU]==0x5aU);
    irb[8U]=12U;put(irb+4U,0x220U);
    assert(!TSCCHECKEXT(&s,131072U,0U,&got)&&got==131072U);
    put(irb+4U,0x218U);assert(TSCCHECKEXT(&s,131072U,0U,&got)==TSC_IO);
    assert(!TSCBUILDEXT(&s,6U,0x40000U,65535U,1U));
    assert(ccw[0U]==6U&&ccw[1U]==0x20U&&ccw[2U]==0xffU&&ccw[3U]==0xffU);
    irb[8U]=0x8cU;put(irb+4U,0x208U);irb[10U]=0xffU;irb[11U]=0xefU;
    assert(!TSCCHECKEXT(&s,65535U,1U,&got)&&got==16U);
    s.occupied=1U;assert(TSCBUILDEXT(&s,6U,0x40000U,65535U,1U)==TSC_BAD&&irb[8U]==0x8cU);s.occupied=0U;
    assert(TSCBUILDEXT(&s,6U,0x40000U,65536U,1U)==TSC_BAD);
    assert(TSCBUILDEXT(&s,6U,0x100U,100U,1U)==TSC_BAD);
    assert(TSCBUILDEXT(&s,6U,0xfffffff0U,100U,1U)==TSC_BAD);
    assert(TSCBUILDEXT(&s,0xffU,0x40000U,100U,0U)==TSC_BAD);
    assert(!TSCBUILDEXT(&s,15U,0x40000U,1U,0U));irb[8U]=12U;put(irb+4U,0x208U);irb[11U]=1U;
    assert(!TSCCHECKEXT(&s,1U,0U,&got)&&got==0U);
    free(ram);return 0;
}
