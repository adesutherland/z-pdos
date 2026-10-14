/* SPDX-License-Identifier: MIT
 * Restart and failed-write controls for the durable K record store. */
#include <stdlib.h>
#include <string.h>
#include "twospace_store.h"
typedef struct { unsigned char *disk;unsigned int writes,fail; } DISK;
#if TSFP_LAYOUT == 2U
static void put(unsigned char *p,unsigned int n)
{ p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);
  p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n; }
static void format(unsigned char *p)
{
    unsigned int h=0x811c9dc5U,i;
    memcpy(p,"PDSTLAY2",8U);put(p+8U,2U);put(p+12U,18452U);
    put(p+16U,911U);put(p+20U,8U);put(p+24U,0x1000000U);
    for(i=0U;i<28U;++i)h=(h^p[i])*0x01000193U;
    put(p+28U,h);
}
#endif
static int io(void *opaque,unsigned int record,unsigned int write,
               unsigned char *block)
{
    DISK *d=(DISK *)opaque;
    if(record>=TSS_RECORDS)return 1;
    if(write&&++d->writes==d->fail)return 1;
    if(write)memcpy(d->disk+record*TSS_BLOCK,block,TSS_BLOCK);
    else memcpy(block,d->disk+record*TSS_BLOCK,TSS_BLOCK);
    return 0;
}
int main(void)
{
    static const unsigned char key[4]={0xc6U,0xc9U,0xd3U,0xc5U};
    TSSSTATE *s;DISK d;unsigned char *a,*b,*out;unsigned int i,n;
    s=(TSSSTATE *)malloc(sizeof *s);d.disk=(unsigned char *)calloc(TSS_RECORDS,TSS_BLOCK);
    a=(unsigned char *)malloc(TSS_LIMIT);b=(unsigned char *)malloc(TSS_LIMIT);
    out=(unsigned char *)malloc(TSS_LIMIT);
    if(!s||!d.disk||!a||!b||!out)return 1;
    d.writes=d.fail=0U;
#if TSFP_LAYOUT == 2U
    if(TSSINIT(s,io,&d)!=TSS_BAD||d.writes)return 10;
    format(d.disk);d.disk[16U]^=1U;
    if(TSSINIT(s,io,&d)!=TSS_BAD||d.writes)return 11;
    format(d.disk);d.disk[32U]=1U;
    if(TSSINIT(s,io,&d)!=TSS_BAD||d.writes)return 12;
    d.disk[32U]=0U;
#endif
    for(i=0U;i<TSS_LIMIT;++i){a[i]=(unsigned char)(i%251U);b[i]=(unsigned char)(255U-i%241U);}
    if(TSSINIT(s,io,&d)!=TSS_OK||TSSREAD(s,31U,key,4U,out,TSS_LIMIT,&n)!=TSS_ABSENT)
        return 2;
    if(TSSADMIT(s,31U,key,4U,TSS_LIMIT)!=TSS_OK||
       TSSADMIT(s,31U,key,4U,TSS_LIMIT+1U)!=TSS_FULL||d.writes)return 14;
    if(TSSPUT(s,31U,key,4U,a,TSS_LIMIT)!=TSS_OK)return 3;
    if(TSSPUT(s,31U,key,4U,a,TSS_LIMIT+1U)!=TSS_BAD||
       TSSREAD(s,31U,key,4U,out,TSS_LIMIT-1U,&n)!=TSS_FULL)return 13;
    memset(s,0,sizeof *s);
    if(TSSINIT(s,io,&d)!=TSS_OK||TSSREAD(s,31U,key,4U,out,TSS_LIMIT,&n)!=TSS_OK||
       n!=TSS_LIMIT||memcmp(a,out,n))return 4;
    d.fail=d.writes+3U;
    if(TSSPUT(s,31U,key,4U,b,TSS_LIMIT)!=TSS_IO)return 5;
    d.fail=0U;memset(s,0,sizeof *s);TSSINIT(s,io,&d);
    if(TSSREAD(s,31U,key,4U,out,TSS_LIMIT,&n)!=TSS_OK||memcmp(a,out,n))return 6;
    if(TSSPUT(s,31U,key,4U,b,TSS_BLOCK+7U)!=TSS_OK||
       TSSREAD(s,31U,key,4U,out,TSS_LIMIT,&n)!=TSS_OK||
       n!=TSS_BLOCK+7U||memcmp(b,out,n))return 7;
    if(TSSPUT(s,24U,key,4U,a,17U)!=TSS_OK||
       TSSREAD(s,24U,key,4U,out,TSS_LIMIT,&n)!=TSS_OK||n!=17U||memcmp(a,out,n))return 8;
    if(TSSERASE(s,31U,key,4U)!=TSS_OK||
       TSSREAD(s,31U,key,4U,out,TSS_LIMIT,&n)!=TSS_ABSENT||
       TSSREAD(s,24U,key,4U,out,TSS_LIMIT,&n)!=TSS_OK)return 9;
    for(i=0U;i<6U;++i)if(TSSPUT(s,100U+i,key,4U,a,1U))return 15;
    n=d.writes;
    if(TSSADMIT(s,500U,key,4U,262144U)!=TSS_FULL||d.writes!=n||
       TSSADMIT(s,24U,key,4U,262144U)!=TSS_OK||d.writes!=n)return 16;
    free(s);free(d.disk);free(a);free(b);free(out);return 0;
}
