/* SPDX-License-Identifier: MIT; immutable source, anchoring and eviction cases. */
#include "twospace_history.h"
#include <assert.h>
#include <string.h>
int main(void)
{
    unsigned char data[512],out[64],text[256];TPHRECORD record[6];TPHSTATE s;TPHVIEW v;unsigned int i,n,hi,lo;
    assert(!TPHINIT(&s,data,sizeof data,record,6U));TPHFOLLOW(&v,7U);
    for(i=0U;i<5U;++i){text[0]=(unsigned char)(0xc1U+i);assert(!TPHAPPEND(&s,7U,11U,TPH_TEXT,text,1U));}
    assert(!TPHRENDER(&s,&v,4U,3U,out,sizeof out,&n)&&n==3U&&out[0U]==0xc3U&&out[4U]==0xc4U&&out[8U]==0xc5U);
    assert(!TPHPAGE(&s,&v,4U,3U,-1)&&!v.follow);hi=v.sequence_hi;lo=v.sequence_lo;
    assert(!TPHRENDER(&s,&v,4U,3U,out,sizeof out,&n)&&out[0U]==0xc1U);
    assert(!TPHAPPEND(&s,9U,12U,TPH_TEXT,text,1U));
    assert(!TPHRENDER(&s,&v,4U,3U,out,sizeof out,&n)&&v.sequence_hi==hi&&v.sequence_lo==lo&&out[0U]==0xc1U);
    assert(!TPHAPPEND(&s,7U,11U,TPH_TEXT,text,1U));
    assert(!TPHRENDER(&s,&v,4U,3U,out,sizeof out,&n)&&v.evicted&&out[0U]==0xc2U);
    TPHFOLLOW(&v,7U);assert(!TPHRENDER(&s,&v,4U,2U,out,sizeof out,&n)&&n==2U);
    memset(text,0xc6U,sizeof text);assert(!TPHAPPEND(&s,7U,11U,TPH_TEXT,text,sizeof text));
    memset(text,0xc7U,sizeof text);assert(!TPHAPPEND(&s,7U,11U,TPH_TEXT,text,sizeof text));
    assert(s.used<=sizeof data&&s.evicted>1U);
    assert(!TPHRENDER(&s,&v,8U,2U,out,sizeof out,&n)&&out[0U]==0xc7U&&out[15U]==0xc7U);
    assert(!TPHCOMPACT(&s,&v,8U,2U,out,sizeof out,&n)&&n==2U&&out[0U]==0xc6U&&out[8U]==0xc7U);
    assert(out[5U]==0x4bU&&out[6U]==0x4bU&&out[7U]==0x4bU&&s.used==512U);
    assert(!TPHINIT(&s,data,sizeof data,record,6U));text[0]=0xc1U;text[1]=0x15U;text[2]=0xc2U;text[3]=0xc3U;text[4]=0xc4U;
    assert(!TPHAPPEND(&s,7U,11U,TPH_TEXT,text,5U));TPHFOLLOW(&v,7U);
    assert(!TPHRENDER(&s,&v,2U,3U,out,sizeof out,&n)&&n==3U&&out[0U]==0xc1U&&out[2U]==0xc2U&&out[3U]==0xc3U&&out[4U]==0xc4U);
    s.sequence_hi=s.sequence_lo=0xffffffffU;n=s.count;
    assert(TPHAPPEND(&s,7U,11U,TPH_TEXT,text,1U)==TPH_BAD&&s.count==n);
    return 0;
}
