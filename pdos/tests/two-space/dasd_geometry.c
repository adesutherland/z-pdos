/* SPDX-License-Identifier: MIT; independently expected track coordinates. */
#include "twospace_dasd.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
static void half(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>8);p[1]=(unsigned char)n;}
static void format4(unsigned char *p,unsigned int cylinders)
{memset(p,0,140U);p[44U]=0xf4U;half(p+62U,cylinders);half(p+64U,15U);
 half(p+107U,1U);half(p+111U,2U);half(p+113U,14U);}
static void extent(unsigned char *p,unsigned int c,unsigned int h,unsigned int ec,unsigned int eh)
{memset(p,0,10U);p[0]=0x81U;half(p+2U,c);half(p+4U,h);half(p+6U,ec);half(p+8U,eh);}
int main(void)
{
    unsigned char f4[140],f1[140],*map,*before;TDAMAP s,prior;
    unsigned int cylinders=7U,tracks=8U,first=1U,last=2U;
    map=(unsigned char *)malloc(TDA_MAX_TRACKS+2U);
    before=(unsigned char *)malloc(TDA_MAX_TRACKS+2U);assert(map&&before);
    memset(map,0xa5,TDA_MAX_TRACKS+2U);format4(f4,1024U);
    assert(!TDAGEOM(f4,140U,&cylinders,&tracks)&&cylinders==1024U&&tracks==15360U);
    assert(!TDAINIT(&s,map,15360U,f4,140U)&&!TDASTAT(&s));
    assert(s.reserved==45U&&s.free==15315U&&s.largest==15315U&&map[15360U]==0xa5U);
    assert(!TDARANGE(&s,99U,14U,100U,0U,&first,&last)&&first==1499U&&last==1500U);
    memset(f1,0,140U);f1[44]=0xf1U;f1[59]=1U;extent(f1+105U,99U,14U,100U,0U);
    assert(!TDAADD(&s,f1,140U)&&!TDASTAT(&s));
    assert(s.allocated==2U&&s.free==15313U&&s.largest==13859U);
    extent(f1+105U,1023U,0U,1023U,14U);
    assert(!TDAADD(&s,f1,140U)&&!TDASTAT(&s));
    assert(s.allocated==17U&&s.free==15298U&&s.largest==13844U);
    assert(!TDAADD(&s,f1,140U)&&!TDASTAT(&s)&&s.allocated==17U);
    memcpy(before,map,15362U);memcpy(&prior,&s,sizeof s);
    f1[59]=2U;extent(f1+105U,101U,0U,101U,14U);extent(f1+115U,1024U,0U,1024U,14U);
    assert(TDAADD(&s,f1,140U)&&!memcmp(map,before,15362U)&&!memcmp(&s,&prior,sizeof s));
    first=1U;last=2U;
    assert(TDARANGE(&s,100U,0U,99U,14U,&first,&last)&&first==1U&&last==2U);
    assert(TDARANGE(&s,0U,15U,0U,15U,&first,&last)&&first==1U&&last==2U);
    assert(TDAINIT(&s,map,15359U,f4,140U)&&!memcmp(map,before,15362U)&&!memcmp(&s,&prior,sizeof s));
    format4(f4,0U);cylinders=7U;tracks=8U;
    assert(TDAGEOM(f4,140U,&cylinders,&tracks)&&cylinders==7U&&tracks==8U);
    format4(f4,1024U);half(f4+64U,14U);
    assert(TDAGEOM(f4,140U,&cylinders,&tracks)&&cylinders==7U&&tracks==8U);
    format4(f4,65535U);memset(map,0xa5,TDA_MAX_TRACKS+2U);
    assert(!TDAINIT(&s,map,TDA_MAX_TRACKS,f4,140U)&&!TDASTAT(&s));
    assert(s.tracks==983025U&&s.free==982980U&&s.largest==982980U);
    assert(map[983025U]==0xa5U&&map[983026U]==0xa5U);
    assert(!TDARANGE(&s,65534U,14U,65534U,14U,&first,&last)&&first==983024U&&last==983024U);
    assert(TDARANGE(&s,65535U,0U,65535U,0U,&first,&last));
    free(before);free(map);
    puts("PASS: 1024/65535-cylinder declarations, cross-cylinder extents and atomic refusals");
    return 0;
}
