/* SPDX-License-Identifier: MIT; independent default reply-mode buffer vectors. */
#include "twospace_3270.h"
#include <assert.h>
#include <string.h>
int main(void)
{
    T27GEOMETRY g;T27INPUTEVENT event,old;unsigned char record[24]={0x60U,0x40U,0xc6U},answer[8],saved[8];unsigned int at=3U,n=99U;
    assert(!T27GEOM(&g,2U,8U,T27_CODED12));
    record[at++]=0x1dU;record[at++]=0xf0U;
    record[at++]=0xc8U;record[at++]=0xc9U;
    record[at++]=0x1dU;record[at++]=0x41U;
    record[at++]=0xc1U;record[at++]=0U;record[at++]=0xc2U;record[at++]=0x40U;
    record[at++]=0U;record[at++]=0U;record[at++]=0U;record[at++]=0U;record[at++]=0U;record[at++]=0U;
    record[at++]=0x1dU;record[at++]=0xf0U;record[at++]=0x40U;
    assert(!T27BUFFER(&g,record,at,4U,answer,sizeof answer,&n,&event)&&n==4U);
    assert(answer[0U]==0xc1U&&answer[1U]==0U&&answer[2U]==0xc2U&&answer[3U]==0x40U&&event.cursor==6U&&event.aid==0x60U);
    assert(!T27BUFFER(&g,record,at,4U,answer,4U,&n,&event)&&n==4U);
    memcpy(saved,answer,sizeof answer);old=event;n=99U;record[at-3U]=0xf0U;
    assert(T27BUFFER(&g,record,at-1U,4U,answer,sizeof answer,&n,&event)==T27_BAD&&n==99U&&!memcmp(answer,saved,sizeof answer)&&!memcmp(&event,&old,sizeof event));
    return 0;
}
