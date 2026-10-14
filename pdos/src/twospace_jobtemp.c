/* SPDX-License-Identifier: MIT */
#include "twospace_jobtemp.h"
static void clear(TJTSLOT *s)
{unsigned int i;s->live=s->ready=s->format=s->lrecl=s->block=s->limit=s->bytes=s->writer=0U;
 for(i=0U;i<8U;++i)s->name[i]=0U;s->data=s->pending=0;}
static unsigned int index_of(TJTPOOL *p,TJTSLOT *s)
{unsigned int i;for(i=0U;i<TJT_SLOTS;++i)if(s==&p->slot[i])return i;return TJT_SLOTS;}
void TJTINIT(TJTPOOL *p,const TJTOPS *ops)
{unsigned int i;p->ops=*ops;for(i=0U;i<TJT_SLOTS;++i)clear(&p->slot[i]);}
TJTSLOT *TJTFIND(TJTPOOL *p,const unsigned char *name)
{unsigned int i,k;if(!p||!name)return 0;
 for(i=0U;i<TJT_SLOTS;++i)if(p->slot[i].live){for(k=0U;k<8U&&p->slot[i].name[k]==name[k];++k){}if(k==8U)return &p->slot[i];}return 0;}
int TJTCLAIM(TJTPOOL *p,const unsigned char *e,TJTSLOT **answer)
{unsigned int i,k,status;TJTSLOT *s;
 if(!p||!e||!answer||!p->ops.acquire||!p->ops.release||TJDWORD(e+4U)!=TJD_TEMP||!TJDNAME(e+40U))return 8;
 if(!TJDWORD(e+24U)||TJDWORD(e+24U)>TJD_TEMP_LIMIT||
    (TJDWORD(e+12U)!=TJD_FB&&TJDWORD(e+12U)!=TJD_VB)||!TJDWORD(e+16U)||
    TJDWORD(e+20U)>32760U||!TJDWORD(e+20U)||
    (TJDWORD(e+12U)==TJD_FB?(TJDWORD(e+16U)>TJDWORD(e+20U)||TJDWORD(e+20U)%TJDWORD(e+16U)):
     (TJDWORD(e+20U)<8U||TJDWORD(e+16U)<4U||TJDWORD(e+16U)>TJDWORD(e+20U)-4U)))return 8;
 status=(TJDWORD(e+92U)>>16)&3U;s=TJTFIND(p,e+40U);
 if(status==TJD_NEW){
  if(TJDWORD(e+8U)!=TJD_OUTPUT)return 8;
  if(s)return 12;for(i=0U;i<TJT_SLOTS&&p->slot[i].live;++i){}if(i==TJT_SLOTS)return 4;
  s=&p->slot[i];clear(s);s->live=1U;
  s->format=TJDWORD(e+12U);s->lrecl=TJDWORD(e+16U);s->block=TJDWORD(e+20U);s->limit=TJDWORD(e+24U);
  for(k=0U;k<8U;++k)s->name[k]=e[40U+k];
 }else if(status!=TJD_OLD||!s||!s->ready)return 12;
 if(!s->limit||s->limit>TJD_TEMP_LIMIT||s->format!=TJDWORD(e+12U)||s->lrecl!=TJDWORD(e+16U)||
    s->block!=TJDWORD(e+20U)||s->limit!=TJDWORD(e+24U)||s->pending)return 12;
 *answer=s;return 0;
}
int TJTSTORE(TJTPOOL *p,TJTSLOT *s,const unsigned char *data,unsigned int bytes)
{
 unsigned int at=0U,i,index,cost;unsigned char e[96],*fresh=0;int rc;
 if(!p||!s)return 8;index=index_of(p,s);
 if(index==TJT_SLOTS||!s->live||s->pending||(!data&&bytes))return 8;
 if(bytes>s->limit)return 4;
 for(i=0U;i<96U;++i)e[i]=0U;
 /* Validate the complete framed BSAM block inventory before acquisition. */
 e[12U]=(unsigned char)(s->format>>24);e[13U]=(unsigned char)(s->format>>16);e[14U]=(unsigned char)(s->format>>8);e[15U]=(unsigned char)s->format;
 e[16U]=(unsigned char)(s->lrecl>>24);e[17U]=(unsigned char)(s->lrecl>>16);e[18U]=(unsigned char)(s->lrecl>>8);e[19U]=(unsigned char)s->lrecl;
 e[20U]=(unsigned char)(s->block>>24);e[21U]=(unsigned char)(s->block>>16);e[22U]=(unsigned char)(s->block>>8);e[23U]=(unsigned char)s->block;
 while(at<bytes){unsigned int n;if(bytes-at<2U)return 8;n=((unsigned int)data[at]<<8)|data[at+1U];at+=2U;
  if(n>bytes-at||TJDOUT(e,data+at,n,&cost,0,0))return 8;at+=n;}
 if(bytes){rc=p->ops.acquire(p->ops.context,index,bytes,&fresh);if(rc)return rc;
  if(!fresh)return 12;for(i=0U;i<bytes;++i)fresh[i]=data[i];}
 if(s->data&&p->ops.release(p->ops.context,index,s->data)){s->pending=fresh;return 12;}
 s->data=fresh;s->bytes=bytes;s->ready=1U;return 0;
}
int TJTDROP(TJTPOOL *p,TJTSLOT *s)
{unsigned int index;if(!p||!s)return 8;index=index_of(p,s);if(index==TJT_SLOTS||!s->live)return 8;
 if(s->writer)return 16;
 if(s->pending){if(p->ops.release(p->ops.context,index,s->pending))return 12;s->pending=0;}
 if(s->data){if(p->ops.release(p->ops.context,index,s->data))return 12;s->data=0;}
 clear(s);return 0;}
int TJTEND(TJTPOOL *p)
{unsigned int i;int rc=0;if(!p)return 8;for(i=0U;i<TJT_SLOTS;++i)if(p->slot[i].live&&TJTDROP(p,&p->slot[i]))rc=12;return rc;}
