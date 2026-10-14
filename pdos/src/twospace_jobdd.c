/* SPDX-License-Identifier: MIT */
#include "twospace_jobdd.h"
unsigned int TJDWORD(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
static unsigned int half(const unsigned char *p)
{return ((unsigned int)p[0]<<8)|p[1];}
static void put_half(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>8);p[1]=(unsigned char)n;}
static int empty(const unsigned char *p,unsigned int n)
{unsigned int i;for(i=0U;i<n;++i)if(p[i])return 0;return 1;}
static int same(const unsigned char *p,const unsigned char *q,unsigned int n)
{unsigned int i;for(i=0U;i<n;++i)if(p[i]!=q[i])return 0;return 1;}
static int letter(unsigned int c)
{return (c>=0xc1U&&c<=0xc9U)||(c>=0xd1U&&c<=0xd9U)||(c>=0xe2U&&c<=0xe9U)||c==0x5bU||c==0x7bU||c==0x7cU;}
int TJDNAME(const unsigned char *p)
{unsigned int i,blank=0U;if(!p||!letter(p[0]))return 0;
 for(i=0U;i<8U;++i){unsigned int c=p[i];if(c==0x40U){blank=1U;continue;}
  if(blank||!(letter(c)||(i&&c>=0xf0U&&c<=0xf9U)))return 0;}return 1;}
static int dataset(const unsigned char *p)
{unsigned int i,part=0U,blank=0U;
 for(i=0U;i<44U;++i){unsigned int c=p[i];
  if(c==0x40U){if(!part)return 0;blank=1U;continue;}
  if(blank)return 0;
  if(c==0x4bU){if(!part)return 0;part=0U;continue;}
  if(part==8U||!(letter(c)||(part&&c>=0xf0U&&c<=0xf9U)))return 0;++part;
 }return part!=0U;}
static int attributes(const unsigned char *e)
{unsigned int fmt=TJDWORD(e+12U),lr=TJDWORD(e+16U),bl=TJDWORD(e+20U);
 if(!lr||!bl||bl>32760U)return 20;
 if(fmt==TJD_FB)return lr<=bl&&bl%lr==0U?0:20;
 if(fmt==TJD_VB)return bl>=8U&&lr>=4U&&lr<=bl-4U?0:20;
 return 20;}
static int records(const unsigned char *e,const unsigned char *p,unsigned int n)
{unsigned int at=0U,lr=TJDWORD(e+16U),fmt=TJDWORD(e+12U);
 while(at<n){unsigned int len;if(n-at<2U)return 8;len=half(p+at);at+=2U;
  if(len>n-at||(fmt==TJD_FB?len!=lr:len>lr-4U))return 8;at+=len;
 }return 0;}
int TJDCHK(const unsigned char *d,unsigned int steps)
{
 unsigned int i,k,count,data=TJD_DATA,per[8];
 if(!d||!steps||steps>8U||(TJDWORD(d)!=1U&&TJDWORD(d)!=2U)||TJDWORD(d+4U)!=TJD_BYTES||
    TJDWORD(d+12U)!=steps||TJDWORD(d+16U)!=1U)return 8;
 count=TJDWORD(d+8U);if(count>TJD_COUNT||!empty(d+20U,12U))return 8;
 for(i=0U;i<8U;++i)per[i]=0U;
 for(i=0U;i<TJD_COUNT;++i){const unsigned char *e=d+32U+i*TJD_ENTRY;
  unsigned int step,kind,mode,off,n,flags,ordinal;int rc;
  if(i>=count){if(!empty(e,TJD_ENTRY))return 8;continue;}
  step=TJDWORD(e);kind=TJDWORD(e+4U);mode=TJDWORD(e+8U);
  off=TJDWORD(e+24U);n=TJDWORD(e+28U);
  flags=TJDWORD(e+92U);ordinal=flags&TJD_CONCAT;
  if((TJDWORD(d)==1U&&flags)||flags&~0x00030303U)return 8;
  if(!step||step>steps||(!ordinal&&++per[step-1U]>8U)||!TJDNAME(e+32U)||
     (mode!=TJD_INPUT&&mode!=TJD_OUTPUT))return 8;
  rc=attributes(e);if(rc)return rc;
  if(ordinal){const unsigned char *old=i?e-TJD_ENTRY:0;
   if(!old||mode!=TJD_INPUT||TJDWORD(old)!=step||TJDWORD(old+8U)!=TJD_INPUT||
      !same(old+32U,e+32U,8U)||(TJDWORD(old+92U)&TJD_CONCAT)+1U!=ordinal||
      TJDWORD(old+12U)!=TJDWORD(e+12U)||TJDWORD(old+16U)!=TJDWORD(e+16U))return 8;}
  for(k=0U;k<i;++k){const unsigned char *old=d+32U+k*TJD_ENTRY;
   if(!ordinal&&TJDWORD(old)==step&&same(old+32U,e+32U,8U))return 8;}
  if(kind!=TJD_TEMP&&(flags&~TJD_CONCAT))return 8;
  if(kind==TJD_DATASET){
   if(off||n||!dataset(e+40U)||(!empty(e+84U,8U)&&!TJDNAME(e+84U)))return 8;
  }else if(kind==TJD_TEMP){
   unsigned int status=(flags>>16)&3U,disp=(flags>>8)&3U;
   if(TJDWORD(d)!=2U||!off||off>TJD_TEMP_LIMIT||n||!TJDNAME(e+40U)||
      !empty(e+84U,8U)||!disp||!status||status>2U||
      (mode==TJD_INPUT&&status!=TJD_OLD))return 8;
   for(k=48U;k<84U;++k)if(e[k]!=0x40U)return 8;
   for(k=0U;k<i;++k){const unsigned char *old=d+32U+k*TJD_ENTRY;
    if(TJDWORD(old)==step&&TJDWORD(old+4U)==TJD_TEMP&&same(old+40U,e+40U,8U)&&
       (TJDWORD(old+92U)&~TJD_CONCAT)!=(flags&~TJD_CONCAT))return 8;}
  }else{
   if(!empty(e+40U,52U))return 8;
   if(kind==TJD_INLINE){
    if(mode!=TJD_INPUT||off!=data||n>TJD_BYTES-data)return 8;
    rc=records(e,d+off,n);if(rc)return rc;data+=n;
   }else if(kind==TJD_DUMMY){if(off||n)return 8;
   }else if(kind==TJD_SYSOUT){
    if(mode!=TJD_OUTPUT||off||n||TJDWORD(e+16U)>(TJDWORD(e+12U)==TJD_FB?132U:136U))return 8;
   }else return 20;
  }
 }return empty(d+data,TJD_BYTES-data)?0:8;
}
int TJDBLOCK(const unsigned char *e,const unsigned char *p,unsigned int n,
             unsigned int *cursor,unsigned char *block,unsigned int cap,unsigned int *actual)
{
 unsigned int at,used,bl,fmt;int rc;
 if(!e||!p||!cursor||!block||!actual)return 8;
 rc=attributes(e);if(rc)return rc;rc=records(e,p,n);if(rc)return rc;
 at=*cursor;bl=TJDWORD(e+20U);fmt=TJDWORD(e+12U);
 if(at>n||cap<bl)return 8;
 /* Reject a cursor into the middle of a record, even if those bytes parse. */
 {unsigned int x=0U;while(x<at)x+=2U+half(p+x);if(x!=at)return 8;}
 if(at==n){*actual=0U;return 1;}
 used=fmt==TJD_VB?4U:0U;
 while(at<n){unsigned int len=half(p+at),i,add=len+(fmt==TJD_VB?4U:0U);
  if(add>bl-used)break;
  if(fmt==TJD_VB){put_half(block+used,len+4U);block[used+2U]=block[used+3U]=0U;used+=4U;}
  for(i=0U;i<len;++i)block[used++]=p[at+2U+i];at+=len+2U;
 }
 if(fmt==TJD_VB){put_half(block,used);block[2U]=block[3U]=0U;}
 *cursor=at;*actual=used;return 0;
}
int TJDOUT(const unsigned char *e,const unsigned char *p,unsigned int n,
           unsigned int *cost,TJDVISIT visit,void *context)
{
 unsigned int fmt,lr,at,total=0U;int rc;
 if(!e||!p||!cost)return 8;rc=attributes(e);if(rc)return rc;
 fmt=TJDWORD(e+12U);lr=TJDWORD(e+16U);
 if(!n||n>TJDWORD(e+20U))return 8;
 if(fmt==TJD_FB){if(n%lr)return 8;at=0U;}
 else {if(n<8U||half(p)!=n||p[2U]||p[3U])return 8;at=4U;}
 while(at<n){unsigned int len=lr;
  if(fmt==TJD_VB){if(n-at<4U||p[at+2U]||p[at+3U])return 8;
   len=half(p+at);if(len<4U||len>lr||len>n-at)return 8;at+=4U;len-=4U;}
  total+=24U+(len?len:1U);at+=len;
 }
 *cost=total;if(!visit)return 0;at=fmt==TJD_VB?4U:0U;
 while(at<n){unsigned int len=lr;
  if(fmt==TJD_VB){len=half(p+at)-4U;at+=4U;}
  rc=visit(context,p+at,len);if(rc)return rc;at+=len;
 }return 0;
}
