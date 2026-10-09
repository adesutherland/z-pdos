/* SPDX-License-Identifier: MIT */
#include "twospace_session.h"
static int geometry(const T27GEOMETRY *g)
{T27GEOMETRY checked;return g&&T27GEOM(&checked,g->rows,g->columns,g->addressing)==T27_OK&&checked.cells==g->cells;}
int TDSINIT(TDSPOOL *p,unsigned char *data,unsigned int bytes)
{
    unsigned int i,j;
    if(!p||!data||bytes<TDS_POOL_BYTES)return TDS_BAD;
    p->data=data;p->bytes=bytes;p->next_handle=0U;
    for(i=0U;i<TDS_SLOTS;++i){
        TDSSESSION *s=&p->session[i];s->handle=s->owner=s->phase=0U;
        s->records=data+i*TDS_SLOT_BYTES;s->transfer=s->records+TDS_EVENTS*TDS_RECORD_BYTES;
        for(j=0U;j<TDS_EVENTS;++j)s->event[j].data=s->records+j*TDS_RECORD_BYTES;
    }
    return TDS_OK;
}
TDSSESSION *TDSFIND(TDSPOOL *p,unsigned int owner,unsigned int handle)
{unsigned int i;if(!p||!owner||!handle)return 0;for(i=0U;i<TDS_SLOTS;++i)if(p->session[i].handle==handle&&p->session[i].owner==owner)return &p->session[i];return 0;}
int TDSOPEN(TDSPOOL *p,unsigned int owner,unsigned int address,unsigned int type,const T27GEOMETRY *g,unsigned int generation,unsigned int *handle)
{
    unsigned int i;TDSSESSION *s;
    if(!p||!p->data||!owner||!address||address>65535U||!generation||!handle||
       (type!=1U&&type!=2U&&type!=3U)||(type==1U&&!geometry(g)))return TDS_BAD;
    if(p->next_handle==0xffffffffU)return TDS_FULL;
    for(i=0U;i<TDS_SLOTS&&p->session[i].handle;++i){}
    if(i==TDS_SLOTS)return TDS_FULL;
    s=&p->session[i];s->handle=++p->next_handle;s->owner=owner;s->address=address;
    s->device_class=type;s->generation=generation;s->phase=TDS_READY;
    if(g)s->geometry=*g;else s->geometry.rows=s->geometry.columns=s->geometry.cells=s->geometry.addressing=0U;
    s->first=s->count=s->sequence_hi=s->sequence_lo=s->transfer_bytes=s->transfer_command=s->transfer_sequence=0U;
    *handle=s->handle;return TDS_OK;
}
int TDSCLOSE(TDSPOOL *p,unsigned int owner,unsigned int handle)
{TDSSESSION *s=TDSFIND(p,owner,handle);if(!s)return TDS_STALE;if(s->phase==TDS_SUBMITTED||s->phase==TDS_QUARANTINED)return TDS_BUSY;s->handle=s->owner=s->phase=s->count=0U;return TDS_OK;}
static int room(const TDSSESSION *s)
{return s&&s->handle&&s->count<TDS_EVENTS&&!(s->sequence_hi==0xffffffffU&&s->sequence_lo==0xffffffffU);}
int TDSPOST(TDSSESSION *s,unsigned int type,const unsigned char *data,unsigned int bytes)
{
    T27INPUTEVENT input;TDSEVENT *e;unsigned char *target;unsigned int i,at;
    if(!s||!s->handle||(bytes&&!data)||bytes>TDS_RECORD_BYTES||
       type<TDS_EVENT_KEY||type>TDS_EVENT_TRUNCATED)return TDS_BAD;
    if(type==TDS_EVENT_TRUNCATED&&(s->device_class!=1U||bytes!=TDS_RECORD_BYTES))return TDS_BAD;
    if(!room(s))return TDS_FULL;
    input.aid=input.cursor_valid=input.cursor=input.fields=0U;
    if(type==TDS_EVENT_KEY||type==TDS_EVENT_STRUCTURED){
        if(s->device_class!=1U||T27EVENT(&s->geometry,data,bytes,&input)||
           (type==TDS_EVENT_STRUCTURED)!=(input.kind==T27_EVENT_STRUCTURED))return TDS_BAD;
    }else if(type==TDS_EVENT_LINE){if(s->device_class!=2U)return TDS_BAD;input.aid=0x7dU;}
    else if(type==TDS_EVENT_GEOMETRY){if(bytes)return TDS_BAD;}
    else if(type==TDS_EVENT_BUFFER){if(s->device_class!=1U||bytes<3U||T27DECD(&s->geometry,data+1U,&input.cursor))return TDS_BAD;input.aid=data[0U];input.cursor_valid=1U;}
    else input.aid=bytes?data[0U]:0U;
    at=(s->first+s->count)%TDS_EVENTS;e=&s->event[at];target=s->records+at*TDS_RECORD_BYTES;
    for(i=0U;i<bytes;++i)target[i]=data[i];
    if(++s->sequence_lo==0U)++s->sequence_hi;
    e->type=type;e->generation=s->generation;e->sequence_hi=s->sequence_hi;e->sequence_lo=s->sequence_lo;
    e->bytes=bytes;e->aid=input.aid;e->cursor_valid=input.cursor_valid;e->cursor=input.cursor;e->fields=input.fields;e->data=target;
    ++s->count;return TDS_OK;
}
int TDSPEEK(const TDSSESSION *s,const TDSEVENT **event)
{if(!s||!s->handle||!event)return TDS_BAD;if(!s->count)return TDS_EMPTY;*event=&s->event[s->first];return TDS_OK;}
int TDSACK(TDSSESSION *s,unsigned int hi,unsigned int lo)
{const TDSEVENT *e;if(TDSPEEK(s,&e))return TDS_STALE;if(e->sequence_hi!=hi||e->sequence_lo!=lo)return TDS_STALE;s->first=(s->first+1U)%TDS_EVENTS;--s->count;return TDS_OK;}
int TDSGEOM(TDSPOOL *p,unsigned int address,const T27GEOMETRY *g,unsigned int generation)
{
    unsigned int i;TDSSESSION *s;
    if(!p||!address||address>65535U||!geometry(g)||!generation)return TDS_BAD;
    for(i=0U;i<TDS_SLOTS;++i){s=&p->session[i];if(!s->handle||s->address!=address)continue;
        if(s->device_class!=1U)return TDS_BAD;
        if(generation<=s->generation)return TDS_STALE;
        if(s->phase!=TDS_READY)return TDS_BUSY;if(!room(s))return TDS_FULL;}
    for(i=0U;i<TDS_SLOTS;++i){s=&p->session[i];if(!s->handle||s->address!=address)continue;
        s->geometry=*g;s->generation=generation;TDSPOST(s,TDS_EVENT_GEOMETRY,0,0U);}
    return TDS_OK;
}
int TDSFRAGMENT(TDSSESSION *s,unsigned int generation,unsigned int command,unsigned int offset,const unsigned char *data,unsigned int bytes,unsigned int end)
{
    unsigned int i;
    if(!s||!s->handle||generation!=s->generation)return TDS_STALE;
    if(s->phase!=TDS_READY&&s->phase!=TDS_ASSEMBLING)return TDS_BUSY;
    if(!command||command>255U||(bytes&&!data)||offset>TDS_TRANSFER_BYTES||bytes>TDS_TRANSFER_BYTES-offset||end>1U)return TDS_BAD;
    if(s->phase==TDS_READY){if(offset)return TDS_BAD;}
    else if(offset!=s->transfer_bytes||command!=s->transfer_command)return TDS_BAD;
    if(!bytes&&!offset)return TDS_BAD;
    if(end&&s->transfer_sequence==0xffffffffU)return TDS_FULL;
    if(data!=s->transfer+offset)for(i=0U;i<bytes;++i)s->transfer[offset+i]=data[i];
    s->phase=TDS_ASSEMBLING;s->transfer_command=command;s->transfer_bytes=offset+bytes;
    if(end){++s->transfer_sequence;s->phase=TDS_SEALED;}
    return TDS_OK;
}
int TDSSUBMIT(TDSSESSION *s)
{if(!s||s->phase!=TDS_SEALED||!s->transfer_bytes)return TDS_BAD;s->phase=TDS_SUBMITTED;return TDS_OK;}
int TDSFINISH(TDSSESSION *s,unsigned int quiesced)
{if(!s||(s->phase!=TDS_SUBMITTED&&s->phase!=TDS_QUARANTINED)||quiesced>1U)return TDS_BAD;if(!quiesced){s->phase=TDS_QUARANTINED;return TDS_BUSY;}s->phase=TDS_READY;s->transfer_bytes=s->transfer_command=0U;return TDS_OK;}
