/* SPDX-License-Identifier: MIT; independent lifetime and atomicity controls. */
#include "twospace_session.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
int main(void)
{
    unsigned char *data=(unsigned char *)malloc(TDS_POOL_BYTES),input[]={0x7dU,0x40U,0xc1U,0x11U,0x40U,0xc1U,0xc1U,0x11U,0x40U,0xc4U,0xc2U};
    unsigned char short_key[]={0x6cU};unsigned char bad[]={0x7dU,0x80U,0U};
    unsigned char colour[]={0x88U,0U,14U,0x81U,0x86U,0U,4U,0U,0xf4U,0xf5U,0xf5U,0xf6U,0xf6U,0xf7U,0xf7U};
    T27CAPABILITIES capabilities,unchanged;
    TDSPOOL pool;TDSSESSION *s,*other;T27GEOMETRY g,large;const TDSEVENT *event;TDSEVENT old;
    unsigned int h,h2,h3,i;
    assert(data&&!TDSINIT(&pool,data,TDS_POOL_BYTES));assert(!T27GEOM(&g,24U,80U,T27_CODED12));
    assert(!T27QRPLY(colour,sizeof colour,&capabilities)&&capabilities.colours_valid==0xf0U&&capabilities.colours[5U]==0xf5U);
    unchanged=capabilities;colour[6U]=5U;
    assert(T27QRPLY(colour,sizeof colour,&capabilities)==T27_BAD&&!memcmp(&capabilities,&unchanged,sizeof capabilities));
    assert(!TDSOPEN(&pool,11U,9U,1U,&g,1U,&h));s=TDSFIND(&pool,11U,h);assert(s&&!TDSFIND(&pool,12U,h));
    assert(TDSCLOSE(&pool,12U,h)==TDS_STALE);
    assert(!TDSPOST(s,TDS_EVENT_KEY,input,sizeof input));assert(!TDSPEEK(s,&event));
    assert(event->aid==0x7dU&&event->cursor_valid&&event->cursor==1U&&event->fields==2U&&event->bytes==sizeof input);
    old=*event;input[6U]=0xc3U;assert(event->data[6U]==0xc1U);
    assert(TDSPOST(s,TDS_EVENT_KEY,bad,sizeof bad)==TDS_BAD&&s->count==1U);
    assert(!TDSPOST(s,TDS_EVENT_KEY,short_key,sizeof short_key));
    assert(TDSACK(s,0U,99U)==TDS_STALE&&s->count==2U);
    assert(!TDSACK(s,old.sequence_hi,old.sequence_lo)&&!TDSPEEK(s,&event)&&!event->cursor_valid&&event->aid==0x6cU);
    assert(!TDSOPEN(&pool,12U,9U,1U,&g,1U,&h2));other=TDSFIND(&pool,12U,h2);
    assert(!T27GEOM(&large,62U,160U,T27_BINARY14));
    for(i=0U;i<TDS_EVENTS;++i)assert(!TDSPOST(other,TDS_EVENT_KEY,short_key,sizeof short_key));
    assert(TDSPOST(other,TDS_EVENT_KEY,short_key,sizeof short_key)==TDS_FULL);
    assert(TDSGEOM(&pool,9U,&large,2U)==TDS_FULL&&s->generation==1U&&other->generation==1U);
    assert(!TDSCLOSE(&pool,12U,h2));assert(!TDSGEOM(&pool,9U,&large,2U)&&s->geometry.cells==9920U&&s->generation==2U);
    assert(TDSGEOM(&pool,9U,&g,2U)==TDS_STALE);
    assert(TDSFRAGMENT(s,1U,0xf3U,0U,input,sizeof input,0U)==TDS_STALE);
    assert(!TDSFRAGMENT(s,2U,0xf3U,0U,input,sizeof input,0U));
    assert(TDSSUBMIT(s)==TDS_BAD);
    assert(TDSFRAGMENT(s,2U,0xf3U,1U,input,sizeof input,1U)==TDS_BAD&&s->transfer_bytes==sizeof input);
    assert(TDSGEOM(&pool,9U,&g,3U)==TDS_BUSY);
    assert(!TDSFRAGMENT(s,2U,0xf3U,sizeof input,input,sizeof input,1U));
    assert(TDSFRAGMENT(s,2U,0xf3U,2U*sizeof input,input,sizeof input,1U)==TDS_BUSY);
    assert(!TDSSUBMIT(s)&&TDSCLOSE(&pool,11U,h)==TDS_BUSY);
    assert(TDSFINISH(s,0U)==TDS_BUSY&&s->phase==TDS_QUARANTINED&&TDSCLOSE(&pool,11U,h)==TDS_BUSY);
    assert(!TDSFINISH(s,1U)&&!TDSCLOSE(&pool,11U,h));
    assert(!TDSOPEN(&pool,11U,9U,1U,&g,1U,&h3)&&h3!=h&&!TDSFIND(&pool,11U,h));
    s=TDSFIND(&pool,11U,h3);s->transfer_sequence=0xffffffffU;s->transfer[0U]=0x5aU;
    assert(TDSFRAGMENT(s,1U,0xf3U,0U,input,sizeof input,1U)==TDS_FULL&&s->transfer[0U]==0x5aU&&s->phase==TDS_READY);
    for(i=0U;i<TDS_RECORD_BYTES;++i)s->transfer[i]=(unsigned char)i;
    assert(TDSPOST(s,TDS_EVENT_TRUNCATED,s->transfer,TDS_RECORD_BYTES-1U)==TDS_BAD&&s->count==0U);
    assert(!TDSPOST(s,TDS_EVENT_TRUNCATED,s->transfer,TDS_RECORD_BYTES)&&!TDSPEEK(s,&event));
    assert(event->type==TDS_EVENT_TRUNCATED&&event->bytes==TDS_RECORD_BYTES&&!event->cursor_valid);
    s->transfer[0U]=0xffU;assert(event->data[0U]==0U&&event->data[65534U]==0xfeU);
    assert(!TDSCLOSE(&pool,11U,h3));
    assert(!TDSOPEN(&pool,11U,11U,3U,0,1U,&h3));s=TDSFIND(&pool,11U,h3);
    assert(s&&s->device_class==3U&&!s->geometry.cells&&TDSPOST(s,TDS_EVENT_KEY,input,sizeof input)==TDS_BAD);
    assert(!TDSCLOSE(&pool,11U,h3));pool.next_handle=0xffffffffU;
    assert(TDSOPEN(&pool,11U,9U,1U,&g,1U,&h3)==TDS_FULL);
    free(data);return 0;
}
