/* SPDX-License-Identifier: MIT; portable native block/record validation. */
#include "twospace_editfile.h"
#include "twospace_digest.h"
static void copybytes(unsigned char *to,const unsigned char *from,unsigned int n)
{unsigned int i;for(i=0U;i<n;++i)to[i]=from[i];}
static unsigned int half(const unsigned char *p)
{return ((unsigned int)p[0]<<8)|p[1];}
static void put2(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>8);p[1]=(unsigned char)n;}
static void put4(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
int TEFVALID(const TEFATTR *a)
{return a&&a->blksize&&a->blksize<=32760U&&
 ((a->format==0x90U&&a->lrecl&&a->lrecl<=256U&&a->blksize%a->lrecl==0U)||
  (a->format==0x50U&&a->lrecl>=4U&&a->lrecl<=260U&&a->lrecl+4U<=a->blksize));}
int TEFUNPK(const TEFATTR *a,const unsigned char *p,unsigned int bytes,
            unsigned char *out,unsigned int cap,unsigned int *used,unsigned int *count)
{
 unsigned int at=0U,n,end,k,len,pos,total=0U,records=0U;
 if(!TEFVALID(a)||(!p&&bytes)||!out||!used||!count||bytes>TEF_LIMIT||cap>TEF_LIMIT)return 8;
 while(at<bytes){
  if(bytes-at<2U)return 8;n=half(p+at);at+=2U;
  if(!n||n>a->blksize||n>bytes-at)return 8;end=at+n;
  if(a->format==0x90U){if(n%a->lrecl)return 8;k=at;}
  else{if(n<4U||half(p+at)!=n||p[at+2U]||p[at+3U])return 8;k=at+4U;}
  while(k<end){
   len=a->lrecl;pos=k;
   if(a->format==0x50U){if(end-k<4U)return 8;len=half(p+k);
    if(len<4U||len>a->lrecl||len>end-k||p[k+2U]||p[k+3U])return 8;pos=k+4U;len-=4U;}
   if(++records>4096U||len+2U>cap-total)return 4;
   put2(out+total,len);copybytes(out+total+2U,p+pos,len);total+=len+2U;k=pos+len;
  }at=end;
 }
 *used=total;*count=records;return 0;
}
static void finish(const TEFATTR *a,unsigned char *out,unsigned int start,unsigned int n)
{put2(out+start,n);if(a->format==0x50U){put2(out+start+2U,n);out[start+4U]=out[start+5U]=0U;}}
int TEFPACK(const TEFATTR *a,const unsigned char *p,unsigned int bytes,
            unsigned char *out,unsigned int cap,unsigned int *used,unsigned int *count)
{
 unsigned int at=0U,start=0U,n=0U,part,cost,total=0U,records=0U,base;
 if(!TEFVALID(a)||(!p&&bytes)||!out||!used||!count||bytes>TEF_LIMIT||cap>TEF_LIMIT)return 8;
 base=a->format==0x50U?4U:0U;
 while(at<bytes){
  if(bytes-at<2U)return 8;part=half(p+at);at+=2U;
  if(part>bytes-at||(a->format==0x90U?part!=a->lrecl:part+4U>a->lrecl)||++records>4096U)return 8;
  cost=part+(a->format==0x50U?4U:0U);
  if(n&&n+cost>a->blksize){finish(a,out,start,n);total=start+2U+n;n=0U;}
  if(!n){start=total;n=base;}
  if(cost>a->blksize-n||start+2U+n>cap||cost>cap-(start+2U+n))return 4;
  if(a->format==0x50U){put2(out+start+2U+n,part+4U);out[start+4U+n]=out[start+5U+n]=0U;n+=4U;}
  copybytes(out+start+2U+n,p+at,part);n+=part;at+=part;
 }
 if(records){finish(a,out,start,n);total=start+2U+n;}
 *used=total;*count=records;return 0;
}
int TEFTOK(const unsigned char key[52],const TEFATTR *a,unsigned int exists,
           const unsigned char *p,unsigned int bytes,unsigned char *work,
           unsigned int cap,unsigned char digest[32])
{
 if(!key||!TEFVALID(a)||exists>1U||(!p&&bytes)||!work||!digest||
    bytes>TEF_LIMIT||cap<TEF_ID_BYTES||bytes>cap-TEF_ID_BYTES)return 8;
 copybytes(work,key,52U);put4(work+52U,a->format);put4(work+56U,a->lrecl);
 put4(work+60U,a->blksize);put4(work+64U,exists);if(bytes)copybytes(work+TEF_ID_BYTES,p,bytes);
 return TJJHASH(work,TEF_ID_BYTES+bytes,digest);
}
