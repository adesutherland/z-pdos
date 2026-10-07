/* SPDX-License-Identifier: MIT */
#include "twospace_store.h"

static unsigned int word(const unsigned char *p)
{ return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|
         ((unsigned int)p[2]<<8)|p[3]; }
static void put(unsigned char *p,unsigned int v)
{ p[0]=(unsigned char)(v>>24);p[1]=(unsigned char)(v>>16);
  p[2]=(unsigned char)(v>>8);p[3]=(unsigned char)v; }
static unsigned int hash(unsigned int h,const unsigned char *p,unsigned int n)
{ unsigned int i;for(i=0U;i<n;++i) h=(h^p[i])*0x01000193U;return h; }
static void zero(unsigned char *p,unsigned int n)
{ unsigned int i;for(i=0U;i<n;++i) p[i]=0U; }
static void copy(unsigned char *p,const unsigned char *q,unsigned int n)
{ unsigned int i;for(i=0U;i<n;++i) p[i]=q[i]; }
static int header(const unsigned char *p)
{
    static const unsigned char magic[8]={0x50U,0x44U,0x53U,0x54U,
                                         0x4fU,0x52U,0x30U,0x31U};
    unsigned int i;
    for(i=0U;i<8U&&p[i]==magic[i];++i) {}
    if(i!=8U||word(p+8U)!=1U||!word(p+12U)||
       !word(p+20U)||word(p+20U)>TSS_KEY||word(p+24U)>TSS_LIMIT||
       word(p+32U)>1U||hash(0x811c9dc5U,p,92U)!=word(p+92U)) return 0;
    return 1;
}
static int same(const unsigned char *p,unsigned int kind,
                 const unsigned char *key,unsigned int n)
{ unsigned int i;if(word(p+16U)!=kind||word(p+20U)!=n)return 0;
  for(i=0U;i<n&&p[36U+i]==key[i];++i){}return i==n; }

/* A damaged/torn inactive header is ignored. A published payload is checked
 * before it can supply bytes. Both banks for one slot name the same key. */
static int find(TSSSTATE *s,unsigned int kind,const unsigned char *key,
                 unsigned int n,unsigned int *slot,unsigned int *bank,
                 unsigned char selected[96],unsigned int *empty)
{
    unsigned char a[96],b[96];unsigned int i,j,valid_a,valid_b;
    if(!s||!s->io||!key||!n||n>TSS_KEY||!slot||!bank||!empty) return TSS_BAD;
    *empty=TSS_SLOTS;
    for(i=0U;i<TSS_SLOTS;++i) {
        if(s->io(s->context,i*2U*TSS_BANK_BLOCKS,0U,s->block)) return TSS_IO;
        copy(a,s->block,96U);valid_a=header(a);
        if(s->io(s->context,(i*2U+1U)*TSS_BANK_BLOCKS,0U,s->block))return TSS_IO;
        copy(b,s->block,96U);valid_b=header(b);
        if(!valid_a&&!valid_b) {if(*empty==TSS_SLOTS)*empty=i;continue;}
        j=valid_b&&(!valid_a||word(b+12U)>word(a+12U))?1U:0U;
        if(valid_a&&valid_b&&!same(a,word(b+16U),b+36U,word(b+20U)))
            return TSS_BAD;
        if(same(j?b:a,kind,key,n)) {
            *slot=i;*bank=j;copy(selected,j?b:a,96U);return TSS_OK;
        }
    }
    return TSS_ABSENT;
}
int TSSINIT(TSSSTATE *s,TSSIO io,void *context)
{ if(!s||!io)return TSS_BAD;s->io=io;s->context=context;return TSS_OK; }

int TSSSIZE(TSSSTATE *s,unsigned int kind,const unsigned char *key,
             unsigned int n,unsigned int *bytes)
{
    unsigned char h[96];unsigned int slot,bank,empty;int rc;
    if(!bytes)return TSS_BAD;*bytes=0U;
    rc=find(s,kind,key,n,&slot,&bank,h,&empty);if(rc)return rc;
    if(word(h+32U))return TSS_DELETED;*bytes=word(h+24U);return TSS_OK;
}

int TSSREAD(TSSSTATE *s,unsigned int kind,const unsigned char *key,
             unsigned int n,unsigned char *data,unsigned int cap,
             unsigned int *bytes)
{
    unsigned char h[96];unsigned int slot,bank,empty,size,at,take,record,sum;
    int rc;if(!data||!bytes)return TSS_BAD;*bytes=0U;
    rc=find(s,kind,key,n,&slot,&bank,h,&empty);if(rc)return rc;
    if(word(h+32U))return TSS_ABSENT;
    size=word(h+24U);if(size>cap)return TSS_FULL;
    sum=0x811c9dc5U;record=(slot*2U+bank)*TSS_BANK_BLOCKS+1U;
    for(at=0U;at<size;at+=take) {
        take=size-at;if(take>TSS_BLOCK)take=TSS_BLOCK;
        if(s->io(s->context,record++,0U,s->block))return TSS_IO;
        sum=hash(sum,s->block,take);copy(data+at,s->block,take);
    }
    if(sum!=word(h+28U))return TSS_BAD;
    *bytes=size;return TSS_OK;
}
static int commit(TSSSTATE *s,unsigned int kind,const unsigned char *key,
                   unsigned int n,const unsigned char *data,unsigned int bytes,
                   unsigned int erased)
{
    static const unsigned char magic[8]={0x50U,0x44U,0x53U,0x54U,
                                         0x4fU,0x52U,0x30U,0x31U};
    unsigned char h[96];unsigned int slot=0U,bank=0U,empty,seq,at,take;
    unsigned int base,record,sum,expected;int rc;
    if(bytes>TSS_LIMIT||(!data&&bytes))return TSS_BAD;
    rc=find(s,kind,key,n,&slot,&bank,h,&empty);
    if(rc!=TSS_OK&&rc!=TSS_ABSENT)return rc;
    if(rc==TSS_ABSENT){
        if(empty==TSS_SLOTS)return TSS_FULL;slot=empty;bank=0U;seq=1U;
    }else{seq=word(h+12U);if(seq==0xffffffffU)return TSS_FULL;++seq;bank^=1U;}
    base=(slot*2U+bank)*TSS_BANK_BLOCKS;record=base+1U;
    expected=hash(0x811c9dc5U,data,bytes);
    /* Invalidate the inactive bank first. Never overwrite the active bank. */
    zero(s->block,TSS_BLOCK);
    if(s->io(s->context,base,1U,s->block))return TSS_IO;
    for(at=0U;at<bytes;at+=take) {
        take=bytes-at;if(take>TSS_BLOCK)take=TSS_BLOCK;
        zero(s->block,TSS_BLOCK);copy(s->block,data+at,take);
        if(s->io(s->context,record++,1U,s->block))return TSS_IO;
    }
    sum=0x811c9dc5U;record=base+1U;
    for(at=0U;at<bytes;at+=take) {
        take=bytes-at;if(take>TSS_BLOCK)take=TSS_BLOCK;
        if(s->io(s->context,record++,0U,s->block))return TSS_IO;
        sum=hash(sum,s->block,take);
    }
    if(sum!=expected)return TSS_IO;
    zero(h,96U);copy(h,magic,8U);put(h+8U,1U);put(h+12U,seq);
    put(h+16U,kind);put(h+20U,n);put(h+24U,bytes);put(h+28U,expected);
    put(h+32U,erased);copy(h+36U,key,n);put(h+92U,hash(0x811c9dc5U,h,92U));
    zero(s->block,TSS_BLOCK);copy(s->block,h,96U);
    if(s->io(s->context,base,1U,s->block)||
       s->io(s->context,base,0U,s->block)||!header(s->block)||
       word(s->block+12U)!=seq||word(s->block+28U)!=expected||
       !same(s->block,kind,key,n))return TSS_IO;
    return TSS_OK;
}
int TSSPUT(TSSSTATE *s,unsigned int kind,const unsigned char *key,
            unsigned int n,const unsigned char *data,unsigned int bytes)
{return commit(s,kind,key,n,data,bytes,0U);}
int TSSERASE(TSSSTATE *s,unsigned int kind,const unsigned char *key,unsigned int n)
{return commit(s,kind,key,n,0,0U,1U);}
