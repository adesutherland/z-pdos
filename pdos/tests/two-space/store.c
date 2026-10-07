/* SPDX-License-Identifier: MIT
 * Restart and failed-write controls for the durable K record store. */
#include <stdlib.h>
#include <string.h>
#include "twospace_store.h"
typedef struct { unsigned char *disk;unsigned int writes,fail; } DISK;
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
    for(i=0U;i<TSS_LIMIT;++i){a[i]=(unsigned char)(i%251U);b[i]=(unsigned char)(255U-i%241U);}
    if(TSSINIT(s,io,&d)!=TSS_OK||TSSREAD(s,31U,key,4U,out,TSS_LIMIT,&n)!=TSS_ABSENT)
        return 2;
    if(TSSPUT(s,31U,key,4U,a,TSS_LIMIT)!=TSS_OK)return 3;
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
    free(s);free(d.disk);free(a);free(b);free(out);return 0;
}
