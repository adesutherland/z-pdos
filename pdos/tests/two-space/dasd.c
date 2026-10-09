/* SPDX-License-Identifier: MIT; independent allocation unions and bad extents. */
#include "twospace_dasd.h"
#include <assert.h>
#include <string.h>
static void half(unsigned char *p,unsigned int n){p[0]=(unsigned char)(n>>8);p[1]=(unsigned char)n;}
int main(void)
{
    unsigned char f4[140],f1[140],map[1500],before[1500];TDAMAP s;
    memset(f4,0,sizeof f4);f4[44]=0xf4U;half(f4+62,100U);half(f4+64,15U);
    half(f4+107,1U);half(f4+111,2U);half(f4+113,14U);
    assert(!TDAINIT(&s,map,sizeof map,f4,sizeof f4)&&!TDASTAT(&s));
    assert(s.reserved==45U&&s.free==1455U&&s.largest==1455U);
    memset(f1,0,sizeof f1);f1[44]=0xf1U;f1[59]=1U;f1[105]=0x81U;
    half(f1+107,3U);half(f1+111,3U);half(f1+113,14U);
    assert(!TDAADD(&s,f1,sizeof f1)&&!TDASTAT(&s));
    assert(s.allocated==15U&&s.free==1440U&&s.largest==1440U&&s.datasets==1U);
    half(f1+107,99U);half(f1+111,99U);
    assert(!TDAADD(&s,f1,sizeof f1)&&!TDASTAT(&s));
    assert(s.allocated==30U&&s.free==1425U&&s.largest==1425U);
    assert(!TDAADD(&s,f1,sizeof f1)&&!TDASTAT(&s)&&s.allocated==30U);
    memcpy(before,map,sizeof map);f1[59]=2U;f1[115]=0x81U;half(f1+117,100U);
    assert(TDAADD(&s,f1,sizeof f1)&&!memcmp(map,before,sizeof map));
    f4[63]=99U;assert(TDAINIT(&s,map,sizeof map,f4,sizeof f4)&&!memcmp(map,before,sizeof map));
    return 0;
}
