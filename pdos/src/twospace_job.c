/* SPDX-License-Identifier: MIT */
#include "twospace_job.h"
#include "twospace_jobdd.h"
unsigned int TJBWORD(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
void TJBPUT(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static void zero(unsigned char *p,unsigned int n)
{unsigned int i;for(i=0U;i<n;++i)p[i]=0U;}
static void copy(unsigned char *p,const unsigned char *q,unsigned int n)
{unsigned int i;for(i=0U;i<n;++i)p[i]=q[i];}
static int name(const unsigned char *p)
{unsigned int i,blank=0U;if(p[0]==0x40U)return 0;
 for(i=0U;i<8U;++i){unsigned int c=p[i];if(c==0x40U){blank=1U;continue;}
  if(blank||!((c>=0xc1U&&c<=0xc9U)||(c>=0xd1U&&c<=0xd9U)||(c>=0xe2U&&c<=0xe9U)||(i&&c>=0xf0U&&c<=0xf9U)))return 0;}
 return 1;}
static unsigned char *slot(TJBPOOL *p,unsigned int i)
{return p->bytes+TJB_BASE+i*TJB_SLOT_BYTES;}
unsigned char *TJBSTEP(unsigned char *j,unsigned int i)
{return j&&i<TJB_STEPS?j+TJB_STEP_BASE+i*TJB_STEP_BYTES:0;}
unsigned char *TJBFIND(TJBPOOL *p,unsigned int owner,unsigned int id)
{unsigned int i;if(!p||!p->bytes||!owner||!id)return 0;
 for(i=0U;i<TJB_SLOTS;++i){unsigned char *j=slot(p,i);if(TJBWORD(j)==id&&TJBWORD(j+4U)==owner)return j;}return 0;}
unsigned char *TJBOUT(TJBPOOL *p,unsigned int id)
{unsigned int i;if(!p||!p->bytes||!id)return 0;
 for(i=0U;i<TJB_SLOTS;++i)if(TJBWORD(slot(p,i))==id)return p->bytes+TJB_OUTPUT_BASE+i*TJB_PRINT_BYTES;return 0;}
unsigned char *TJBDDS(TJBPOOL *p,unsigned int id)
{unsigned int i;if(!p||!p->bytes||!id)return 0;
 for(i=0U;i<TJB_SLOTS;++i)if(TJBWORD(slot(p,i))==id)return p->bytes+TJB_DD_BASE+i*TJD_BYTES;return 0;}
int TJBBIND(TJBPOOL *p,unsigned int owner,unsigned int id,const unsigned char *d,unsigned int n)
{unsigned char *j=TJBFIND(p,owner,id),*target;int rc;
 if(!j||!d||n!=TJD_BYTES)return TJB_BAD;
 if(TJBWORD(j+8U)!=TJB_QUEUED&&TJBWORD(j+8U)!=TJB_HELD)return TJB_BUSY;
 target=TJBDDS(p,id);if(TJBWORD(target))return TJB_BUSY;
 rc=TJDCHK(d,TJBWORD(j+16U));if(rc)return rc;copy(target,d,n);TJBPUT(p->bytes+4U,2U);return 0;}
int TJBINIT(TJBPOOL *p,unsigned char *b,unsigned int n)
{if(!p||!b||n!=TJB_BYTES)return TJB_BAD;zero(b,n);TJBPUT(b,0x50444a32U);TJBPUT(b+4U,1U);p->bytes=b;return 0;}
int TJBCHECK(const unsigned char *d,unsigned int n)
{unsigned int i,k,count;if(!d||n!=TJB_DESC_BYTES||TJBWORD(d)!=1U||TJBWORD(d+4U)!=n||!name(d+8U))return TJB_BAD;
 count=TJBWORD(d+16U);if(!count||count>TJB_STEPS||TJBWORD(d+20U)<256U||TJBWORD(d+20U)>TJB_PRINT_BYTES)return TJB_FULL;
 for(i=24U;i<32U;++i)if(d[i])return TJB_UNSUPPORTED;
 for(i=0U;i<TJB_STEPS;++i){const unsigned char *s=d+32U+i*TJB_STEP_BYTES;
  if(i>=count){for(k=0U;k<TJB_STEP_BYTES;++k)if(s[k])return TJB_BAD;continue;}
  if(!name(s)||!name(s+8U)||TJBWORD(s+16U)>100U||TJBWORD(s+20U)>TJB_COND_NE)return TJB_BAD;
  for(k=TJBWORD(s+16U);k<100U;++k)if(s[24U+k])return TJB_BAD;
  for(k=124U;k<TJB_STEP_BYTES;++k)if(s[k])return TJB_BAD;
  for(k=0U;k<i;++k){unsigned int x;const unsigned char *old=d+32U+k*TJB_STEP_BYTES;for(x=0U;x<8U&&s[x]==old[x];++x){}if(x==8U)return TJB_BAD;}
 }return 0;}
int TJBSUB(TJBPOOL *p,unsigned int owner,const unsigned char *d,unsigned int n,unsigned int *id)
{unsigned int i,k,waiting=0U;unsigned char *j=0;int rc;
 if(!p||!p->bytes||!owner||!id)return TJB_BAD;rc=TJBCHECK(d,n);if(rc)return rc;
 for(i=0U;i<TJB_SLOTS;++i){unsigned char *a=slot(p,i);unsigned int state=TJBWORD(a+8U);
  if(!state&&!j)j=a;if(state==TJB_QUEUED||state==TJB_HELD)++waiting;}
 if(!j||waiting||TJBWORD(p->bytes+8U)==0xffffffffU)return TJB_FULL;
 *id=TJBWORD(p->bytes+8U)+1U;TJBPUT(p->bytes+8U,*id);zero(j,TJB_SLOT_BYTES);
 TJBPUT(j,*id);TJBPUT(j+4U,owner);TJBPUT(j+8U,TJB_QUEUED);TJBPUT(j+16U,TJBWORD(d+16U));TJBPUT(j+20U,TJBWORD(d+20U));copy(j+64U,d+8U,8U);
 for(k=0U;k<TJBWORD(d+16U);++k)copy(TJBSTEP(j,k),d+32U+k*TJB_STEP_BYTES,TJB_STEP_BYTES);
 zero(TJBOUT(p,*id),TJB_PRINT_BYTES);zero(TJBDDS(p,*id),TJD_BYTES);return 0;}
int TJBACT(TJBPOOL *p,unsigned int owner,unsigned int id,unsigned int action)
{unsigned char *j=TJBFIND(p,owner,id);unsigned int state;if(!j)return TJB_BAD;state=TJBWORD(j+8U);
 if(action==1U&&state==TJB_QUEUED){TJBPUT(j+8U,TJB_HELD);return 0;}
 if(action==2U&&state==TJB_HELD){TJBPUT(j+8U,TJB_QUEUED);return 0;}
 if(action==3U&&(state==TJB_QUEUED||state==TJB_HELD||state==TJB_RUNNING)){
  if(state!=TJB_RUNNING){unsigned int i;for(i=0U;i<TJB_SLOTS;++i){unsigned int other=TJBWORD(slot(p,i)+8U);if(other==TJB_RUNNING||other>=TJB_DONE)return TJB_BUSY;}}
  TJBPUT(j+48U,1U);if(state!=TJB_RUNNING){unsigned int i;TJBPUT(j+8U,TJB_CANCELLED);TJBPUT(j+32U,16U);
   for(i=0U;i<TJBWORD(j+16U);++i){TJBPUT(TJBSTEP(j,i)+148U,TJB_SKIP);TJBPUT(TJBSTEP(j,i)+144U,3U);}}return 0;}
 if(action==4U&&state>=TJB_DONE&&TJBWORD(j+28U)!=TJB_PRINT_ACTIVE){zero(TJBOUT(p,id),TJB_PRINT_BYTES);zero(TJBDDS(p,id),TJD_BYTES);zero(j,TJB_SLOT_BYTES);return 0;}
 return TJB_BUSY;}
int TJBSTART(TJBPOOL *p,unsigned int owner,unsigned int id)
{unsigned char *j=TJBFIND(p,owner,id);unsigned int i,completed=0U;if(!j)return TJB_BAD;
 if(TJBWORD(j+8U)!=TJB_QUEUED||TJBWORD(p->bytes+12U))return TJB_BUSY;
 for(i=0U;i<TJB_SLOTS;++i)if(TJBWORD(slot(p,i)+8U)>=TJB_DONE)++completed;
 if(completed)return TJB_FULL;TJBPUT(p->bytes+12U,id);TJBPUT(j+8U,TJB_RUNNING);return 0;}
int TJBNEXT(TJBPOOL *p,unsigned int owner,unsigned int id)
{unsigned char *j=TJBFIND(p,owner,id);unsigned int i,count,os,valid,rc;
 if(!j||TJBWORD(j+8U)!=TJB_RUNNING||TJBWORD(p->bytes+12U)!=id)return TJB_BAD;
 count=TJBWORD(j+16U);i=TJBWORD(j+12U);os=TJBWORD(j+32U);valid=TJBWORD(j+36U);rc=TJBWORD(j+40U);
 for(;i<count;++i){unsigned char *s=TJBSTEP(j,i);if(TJBWORD(s+148U))return TJB_BUSY;
  if(os||TJBWORD(j+48U)||(TJBWORD(s+20U)==TJB_COND_NE&&valid&&rc)){
   TJBPUT(s+148U,TJB_SKIP);TJBPUT(s+144U,TJBWORD(j+48U)?3U:os?2U:1U);TJBPUT(j+12U,i+1U);continue;}
  TJBPUT(s+148U,TJB_ACTIVE);TJBPUT(j+12U,i);return 0;}
 TJBPUT(j+8U,TJBWORD(j+48U)?TJB_CANCELLED:os?TJB_FAILED:TJB_DONE);TJBPUT(p->bytes+12U,0U);return 1;}
int TJBEND(TJBPOOL *p,unsigned int owner,unsigned int id,unsigned int os,unsigned int valid,unsigned int rc,unsigned int abnormal,unsigned int cleanup)
{unsigned char *j=TJBFIND(p,owner,id),*s;unsigned int i;if(!j||TJBWORD(j+8U)!=TJB_RUNNING||valid>1U)return TJB_BAD;
 i=TJBWORD(j+12U);s=TJBSTEP(j,i);if(i>=TJBWORD(j+16U)||TJBWORD(s+148U)!=TJB_ACTIVE)return TJB_BAD;
 TJBPUT(s+124U,os);TJBPUT(s+128U,valid);TJBPUT(s+132U,valid?rc:0xffffffffU);TJBPUT(s+136U,abnormal);TJBPUT(s+140U,cleanup);
 TJBPUT(s+148U,TJBWORD(j+48U)?TJB_CANCEL:os||abnormal||cleanup?TJB_ERROR:TJB_NORMAL);
 if(os||abnormal||cleanup){TJBPUT(j+32U,os?os:12U);TJBPUT(j+36U,0U);TJBPUT(j+40U,0xffffffffU);}
 else if(!TJBWORD(j+32U)){TJBPUT(j+36U,valid);if(valid&&rc>TJBWORD(j+40U))TJBPUT(j+40U,rc);}
 TJBPUT(j+44U,cleanup);TJBPUT(j+12U,i+1U);return 0;}
int TJBAPP(TJBPOOL *p,unsigned int owner,unsigned int id,const unsigned char dd[8],unsigned int command,const unsigned char *data,unsigned int n)
{unsigned char *j=TJBFIND(p,owner,id),*out;unsigned int used,quota;
 if(!j||TJBWORD(j+8U)<TJB_RUNNING||TJBWORD(j+28U)!=TJB_PRINT_PENDING||!dd||!name(dd)||!data||!n||n>132U||(command!=9U&&command!=0x89U))return TJB_BAD;
 used=TJBWORD(j+24U);quota=TJBWORD(j+20U);if(used>quota||24U+n>quota-used)return TJB_FULL;
 out=TJBOUT(p,id)+used;TJBPUT(out,TJBWORD(j+52U)+1U);TJBPUT(out+4U,TJBWORD(j+12U)<TJBWORD(j+16U)?TJBWORD(j+12U)+1U:TJBWORD(j+16U));TJBPUT(out+8U,n);TJBPUT(out+12U,command);copy(out+16U,dd,8U);copy(out+24U,data,n);
 TJBPUT(j+52U,TJBWORD(j+52U)+1U);TJBPUT(j+24U,used+24U+n);return 0;}
int TJBLOAD(TJBPOOL *p,unsigned char *b,unsigned int n)
{unsigned int i,k,active=0U,wait=0U,completed=0U,max=0U;
 if(!p||!b||n!=TJB_BYTES||TJBWORD(b)!=0x50444a32U||(TJBWORD(b+4U)!=1U&&TJBWORD(b+4U)!=2U))return TJB_BAD;
 for(i=16U;i<TJB_BASE;++i)if(b[i])return TJB_BAD;
 for(i=0U;i<TJB_SLOTS;++i){unsigned char *j=b+TJB_BASE+i*TJB_SLOT_BYTES;unsigned int state=TJBWORD(j+8U),id=TJBWORD(j),count=TJBWORD(j+16U),used=TJBWORD(j+24U),at=0U,seq=0U;
  {const unsigned char *dd=b+TJB_DD_BASE+i*TJD_BYTES;
   if(TJBWORD(dd)){if(TJBWORD(b+4U)!=2U||!state||TJDCHK(dd,count))return TJB_BAD;}
   else for(k=0U;k<TJD_BYTES;++k)if(dd[k])return TJB_BAD;}
  if(!state){for(k=0U;k<TJB_SLOT_BYTES;++k)if(j[k])return TJB_BAD;continue;}
  if(state>TJB_INTERRUPTED||!id||!TJBWORD(j+4U)||!count||count>TJB_STEPS||TJBWORD(j+12U)>count||!name(j+64U)||TJBWORD(j+20U)<256U||TJBWORD(j+20U)>TJB_PRINT_BYTES||used>TJBWORD(j+20U)||TJBWORD(j+28U)>TJB_PRINT_FAILED||TJBWORD(j+36U)>1U||TJBWORD(j+48U)>1U)return TJB_BAD;
  if(id>max)max=id;for(k=0U;k<i;++k)if(TJBWORD(b+TJB_BASE+k*TJB_SLOT_BYTES)==id)return TJB_BAD;
  if(state==TJB_RUNNING){++active;if(TJBWORD(b+12U)!=id)return TJB_BAD;}else if(state==TJB_QUEUED||state==TJB_HELD)++wait;else ++completed;
  for(k=0U;k<count;++k){unsigned char *s=TJBSTEP(j,k);unsigned int x;
   if(!name(s)||!name(s+8U)||TJBWORD(s+16U)>100U||TJBWORD(s+20U)>TJB_COND_NE||TJBWORD(s+128U)>1U||TJBWORD(s+148U)>TJB_INTERRUPT)return TJB_BAD;
   if(TJBWORD(s+148U)==TJB_ACTIVE&&(state!=TJB_RUNNING||k!=TJBWORD(j+12U)))return TJB_BAD;
   for(x=TJBWORD(s+16U);x<100U;++x)if(s[24U+x])return TJB_BAD;
   for(x=152U;x<160U;++x)if(s[x])return TJB_BAD;}
  while(at<used){const unsigned char *r=b+TJB_OUTPUT_BASE+i*TJB_PRINT_BYTES+at;unsigned int len;if(used-at<24U)return TJB_BAD;len=TJBWORD(r+8U);
   if(!len||len>132U||24U+len>used-at||TJBWORD(r)!=++seq||!TJBWORD(r+4U)||TJBWORD(r+4U)>count||!name(r+16U)||(TJBWORD(r+12U)!=9U&&TJBWORD(r+12U)!=0x89U))return TJB_BAD;at+=24U+len;}
  if(seq!=TJBWORD(j+52U))return TJB_BAD;
 }
 for(i=TJB_DD_BASE+TJB_SLOTS*TJD_BYTES;i<TJB_BYTES;++i)if(b[i])return TJB_BAD;
 if(active>1U||wait>1U||completed>1U||(active&&completed)||max>TJBWORD(b+8U)||(!active&&TJBWORD(b+12U)))return TJB_BAD;
 p->bytes=b;
 /* A queued descriptor needs explicit release after restart. Uncertain
    execution and delivery are never automatically replayed. */
 for(i=0U;i<TJB_SLOTS;++i){unsigned char *j=slot(p,i);unsigned int state=TJBWORD(j+8U);
  if(state==TJB_RUNNING){TJBPUT(j+8U,TJB_INTERRUPTED);TJBPUT(j+32U,12U);TJBPUT(j+36U,0U);TJBPUT(j+40U,0xffffffffU);
   for(k=0U;k<TJBWORD(j+16U);++k){unsigned char *s=TJBSTEP(j,k);if(TJBWORD(s+148U)==TJB_ACTIVE){TJBPUT(s+148U,TJB_INTERRUPT);TJBPUT(s+124U,12U);TJBPUT(s+132U,0xffffffffU);}else if(!TJBWORD(s+148U)){TJBPUT(s+148U,TJB_SKIP);TJBPUT(s+144U,4U);}}}
  if(state==TJB_QUEUED)TJBPUT(j+8U,TJB_HELD);
  if(TJBWORD(j+28U)==TJB_PRINT_ACTIVE)TJBPUT(j+28U,TJB_PRINT_FAILED);
 }TJBPUT(b+12U,0U);return 0;}
