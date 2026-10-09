/* SPDX-License-Identifier: MIT */
#include "twospace_io.h"
static unsigned int word(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
static void put(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static void bytes(unsigned char *d,const unsigned char *s,unsigned int n)
{unsigned int i;for(i=0U;i<n;++i)d[i]=s[i];}
static TIOREQUEST *find(TIOPOOL *p,const TSCSTATE *s)
{unsigned int i;if(!p||!s)return 0;for(i=0U;i<TIO_SLOTS;++i)if(p->request[i].channel==s)return &p->request[i];return 0;}
const TIOREQUEST *TIOSTATE(const TIOPOOL *p,const TSCSTATE *s)
{unsigned int i;if(!p||!s)return 0;for(i=0U;i<TIO_SLOTS;++i)if(p->request[i].channel==s)return &p->request[i];return 0;}
void TIOINIT(TIOPOOL *p)
{unsigned int i;if(!p)return;p->next_ticket=0U;for(i=0U;i<TIO_SLOTS;++i){p->request[i].channel=0;p->request[i].phase=TIO_IDLE;}}
int TIOBIND(TIOPOOL *p,TSCSTATE *s)
{
    unsigned int i,j;if(!p||!s||!s->aperture)return TIO_BAD;
    if(find(p,s))return TIO_OK;
    for(i=0U;i<TIO_SLOTS;++i)if(p->request[i].channel&&p->request[i].channel->region_real==s->region_real)return TIO_BAD;
    for(i=0U;i<TIO_SLOTS;++i)if(!p->request[i].channel){
        TIOREQUEST *r=&p->request[i];r->channel=s;r->ssid=r->owner=r->ticket=r->end_ccw=r->count=r->status_valid=r->notices=0U;r->phase=TIO_IDLE;
        for(j=0U;j<64U;++j)r->status[j]=r->notice[j]=0U;
        return TIO_OK;
    }
    return TIO_BUSY;
}
int TIOREADY(const TIOPOOL *p,const TSCSTATE *s)
{const TIOREQUEST *r=TIOSTATE(p,s);return r&&r->phase==TIO_IDLE&&!s->quarantined?TIO_OK:TIO_BUSY;}
int TIOTAKE(TIOPOOL *p,TSCSTATE *s,unsigned int ssid,unsigned int owner,unsigned int *ticket)
{
    TIOREQUEST *r=find(p,s);unsigned int i,end=0U,count=0U;const unsigned char *base;
    if(!r||!ticket||!ssid||s->quarantined||r->phase!=TIO_IDLE)return TIO_BUSY;
    for(i=0U;i<TIO_SLOTS;++i)if(p->request[i].phase!=TIO_IDLE&&p->request[i].ssid==ssid)return TIO_BUSY;
    if(p->next_ticket==0xffffffffU)return TIO_BAD;
    base=s->aperture+s->region_real;
    /* Every supported chain starts at this workspace's CCW area. Recover the
     * final CCW from command chaining, bounding TIC/search loops by storage. */
    if(word(base+TSC_ORB_OFFSET+8U)!=s->region_real+TSC_CCW_OFFSET)return TIO_BAD;
    for(i=TSC_CCW_OFFSET;i+8U<=TSC_SEEK_OFFSET;i+=8U){
        if(base[i]==8U)continue;
        if(!(base[i+1U]&0xc0U)){end=s->region_real+i+8U;count=((unsigned int)base[i+2U]<<8)|base[i+3U];break;}
    }
    if(!end||!count)return TIO_BAD;
    r->ssid=ssid;r->owner=owner;r->ticket=++p->next_ticket;r->end_ccw=end;r->count=count;r->status_valid=0U;r->phase=TIO_PREPARED;s->occupied=1U;
    put(TSCORB(s),r->ticket);*ticket=r->ticket;return TIO_OK;
}
static int identity(const TIOREQUEST *r,unsigned int owner,unsigned int ticket)
{return r&&r->owner==owner&&r->ticket==ticket&&ticket!=0U;}
static int ops_valid(const TIOOPS *o)
{return o&&o->start&&o->poll&&o->wait&&o->clear;}
static int notify(TIOREQUEST *r,const TIOOPS *o)
{
    bytes(r->notice,TSCIRB(r->channel),64U);
    if(r->notices!=0xffffffffU)++r->notices;
    return o->notice?o->notice(r->channel,r->ssid,r->notice,o->context):TIO_OK;
}
int TIOFREE(TIOPOOL *p,TSCSTATE *s,unsigned int owner,unsigned int ticket)
{
    TIOREQUEST *r=find(p,s);
    if(!identity(r,owner,ticket))return TIO_STALE;
    if(r->phase!=TIO_DONE&&r->phase!=TIO_PREPARED)return TIO_BUSY;
    if(s->quarantined)return TIO_BUSY;
    r->phase=TIO_IDLE;s->occupied=0U;return TIO_OK;
}
int TIOCANCEL(TIOPOOL *p,TSCSTATE *s,unsigned int owner,unsigned int ticket,const TIOOPS *o)
{
    TIOREQUEST *r=find(p,s);int status;
    if(!identity(r,owner,ticket)||!ops_valid(o))return TIO_STALE;
    if(r->phase==TIO_PREPARED||r->phase==TIO_DONE)return TIOFREE(p,s,owner,ticket);
    if(r->phase!=TIO_ACTIVE&&r->phase!=TIO_QUARANTINED)return TIO_BAD;
    s->quarantined=1U;r->phase=TIO_QUARANTINED;
    status=o->clear(r->ssid,TSCIRB(s),o->context);
    if(status||!(TSCIRB(s)[2U]&0x10U)||TSCIRB(s)[9U])return TIO_ERROR;
    /* The provider confirms clear completion. The separate operation status
     * remains available for diagnosis; IRB now contains the clear receipt. */
    s->quarantined=0U;s->occupied=0U;r->phase=TIO_IDLE;return TIO_OK;
}
int TIOQUIESCE(TIOPOOL *p,TSCSTATE *s,unsigned int ssid,unsigned int owner,const TIOOPS *o)
{
    TIOREQUEST *r=find(p,s);unsigned int i;
    if(!r||!ssid||!ops_valid(o))return TIO_BAD;
    if(r->phase!=TIO_IDLE){
        if(r->ssid!=ssid||r->owner!=owner)return TIO_STALE;
        return TIOCANCEL(p,s,owner,r->ticket,o);
    }
    for(i=0U;i<TIO_SLOTS;++i)if(p->request[i].phase!=TIO_IDLE&&p->request[i].ssid==ssid)return TIO_BUSY;
    if(p->next_ticket==0xffffffffU)return TIO_BAD;
    r->ssid=ssid;r->owner=owner;r->ticket=++p->next_ticket;r->phase=TIO_QUARANTINED;s->occupied=s->quarantined=1U;
    return TIOCANCEL(p,s,owner,r->ticket,o);
}
int TIOSUB(TIOPOOL *p,TSCSTATE *s,unsigned int owner,unsigned int ticket,const TIOOPS *o)
{
    TIOREQUEST *r=find(p,s);unsigned int pending;int status,n;
    if(!identity(r,owner,ticket)||!ops_valid(o)||r->phase!=TIO_PREPARED)return TIO_STALE;
    for(pending=0U;pending<8U;++pending){
        status=o->start(r->ssid,TSCORB(s),TSCIRB(s),o->context);
        if(!status){r->phase=TIO_ACTIVE;return TIO_OK;}
        if(status!=-2){TIOFREE(p,s,owner,ticket);return TIO_ERROR;}
        if(o->poll(r->ssid,TSCIRB(s),o->context)){TIOFREE(p,s,owner,ticket);return TIO_ERROR;}
        if(TSCIRB(s)[9U]||(TSCIRB(s)[8U]&2U)){bytes(r->status,TSCIRB(s),64U);r->status_valid=1U;TIOFREE(p,s,owner,ticket);return TIO_ERROR;}
        n=notify(r,o);
        if(n==TIO_DEFER){TIOFREE(p,s,owner,ticket);return TIO_DEFER;}
        if(n){TIOFREE(p,s,owner,ticket);return TIO_ERROR;}
    }
    TIOFREE(p,s,owner,ticket);return TIO_BUSY;
}
int TIOPOLL(TIOPOOL *p,TSCSTATE *s,unsigned int owner,unsigned int ticket,const TIOOPS *o)
{
    TIOREQUEST *r=find(p,s);unsigned char *irb;unsigned int i;int status;
    if(!identity(r,owner,ticket)||!ops_valid(o)||r->phase!=TIO_ACTIVE)return TIO_STALE;
    status=o->poll(r->ssid,TSCIRB(s),o->context);if(status>0)return TIO_PENDING;
    if(status<0){TIOCANCEL(p,s,owner,ticket,o);return TIO_ERROR;}
    irb=TSCIRB(s);
    if(irb[9U]||(irb[8U]&2U)||(irb[2U]&0x10U)){bytes(r->status,irb,64U);r->status_valid=1U;TIOCANCEL(p,s,owner,ticket,o);return TIO_ERROR;}
    if(irb[8U]&0x80U){if(notify(r,o)){TIOCANCEL(p,s,owner,ticket,o);return TIO_ERROR;}}
    /* An attention-only/ready notification cannot replace an earlier primary
     * transfer result while secondary ending status is still outstanding. */
    if(!(irb[8U]&0x0cU))return TIO_PENDING;
    /* Secondary status can finish a separately reported channel end. Its
     * CPA/count are not the original primary transfer result. */
    if(r->status_valid&&(irb[3U]&2U)&&!(irb[3U]&4U)){
        r->status[8U]|=irb[8U];r->status[9U]|=irb[9U];
        for(i=0U;i<4U;++i)r->status[i]=irb[i];
    }else{
        unsigned int old=r->status_valid?r->status[8U]:0U;
        bytes(r->status,irb,64U);r->status[8U]|=(unsigned char)(old&0x0cU);r->status_valid=1U;
    }
    if((r->status[8U]&0x0cU)!=0x0cU)return TIO_PENDING;
    if((irb[2U]&0x0fU)||(irb[3U]&0xe0U)||(irb[3U]&8U))return TIO_PENDING;
    if(word(r->status+4U)!=r->end_ccw||(((unsigned int)r->status[10U]<<8)|r->status[11U])>r->count){TIOCANCEL(p,s,owner,ticket,o);return TIO_ERROR;}
    bytes(irb,r->status,64U);r->phase=TIO_DONE;return TIO_OK;
}
int TIOEXEC(TIOPOOL *p,TSCSTATE *s,unsigned int ssid,unsigned int owner,const TIOOPS *o)
{
    unsigned int ticket;int status;TIOREQUEST *r;
    if(!ops_valid(o))return TIO_BAD;
    status=TIOTAKE(p,s,ssid,owner,&ticket);if(status)return status;
    status=TIOSUB(p,s,owner,ticket,o);if(status)return status;
    r=find(p,s);
    for(;;){
        status=TIOPOLL(p,s,owner,ticket,o);
        if(status==TIO_OK){TIOFREE(p,s,owner,ticket);return TIO_OK;}
        if(status!=TIO_PENDING)return status;
        if(o->wait(r->ssid,TSCIRB(s),0U,o->context)){TIOCANCEL(p,s,owner,ticket,o);return TIO_ERROR;}
    }
}
