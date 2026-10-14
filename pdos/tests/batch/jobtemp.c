/* SPDX-License-Identifier: MIT; independent replacement, failure and cleanup. */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "twospace_job.h"
#include "twospace_jobtemp.h"
static unsigned int active,fail_alloc,fail_free;
static int acquire(void *u,unsigned int i,unsigned int n,unsigned char **p)
{(void)u;(void)i;if(fail_alloc)return 4;*p=(unsigned char *)malloc(n);assert(*p);++active;return 0;}
static int release(void *u,unsigned int i,unsigned char *p)
{(void)u;(void)i;if(fail_free)return 12;free(p);--active;return 0;}
static void entry(unsigned char *e,unsigned int status)
{memset(e,0,96U);TJBPUT(e,1U);TJBPUT(e+4U,TJD_TEMP);TJBPUT(e+8U,TJD_OUTPUT);TJBPUT(e+12U,TJD_FB);
 TJBPUT(e+16U,4U);TJBPUT(e+20U,8U);TJBPUT(e+24U,64U);memset(e+40U,0x40,44U);e[40U]=0xe3U;
 TJBPUT(e+92U,status<<16|TJD_PASS<<8);}
int main(void)
{
 TJTPOOL p;TJTOPS ops;TJTSLOT *s,*same,foreign;unsigned char e[96],old[]={0,4,0,1,2,3},next[]={0,8,4,5,6,7,8,9,10,11},bad[]={0,5,1,2,3,4,5};
 ops.acquire=acquire;ops.release=release;ops.context=0;TJTINIT(&p,&ops);
 entry(e,TJD_OLD);assert(TJTCLAIM(&p,e,&s)==12&&!TJTFIND(&p,e+40U));
 entry(e,TJD_NEW);assert(!TJTCLAIM(&p,e,&s)&&s->live&&!s->ready);assert(TJTCLAIM(&p,e,&same)==12);
 assert(!TJTSTORE(&p,s,old,sizeof old)&&s->ready&&active==1U);
 entry(e,TJD_OLD);TJBPUT(e+8U,TJD_INPUT);assert(!TJTCLAIM(&p,e,&same)&&same==s);
 assert(TJTSTORE(&p,s,bad,sizeof bad)==8&&s->bytes==sizeof old&&!memcmp(s->data,old,sizeof old));
 fail_alloc=1U;assert(TJTSTORE(&p,s,next,sizeof next)==4&&active==1U&&!memcmp(s->data,old,sizeof old));fail_alloc=0U;
 fail_free=1U;assert(TJTSTORE(&p,s,next,sizeof next)==12&&active==2U&&s->pending&&!memcmp(s->data,old,sizeof old));
 assert(TJTEND(&p)==12&&s->live&&s->pending);fail_free=0U;
 assert(!TJTEND(&p)&&active==0U&&!s->live);
 entry(e,TJD_NEW);assert(!TJTCLAIM(&p,e,&s)&&!TJTSTORE(&p,s,0,0U)&&s->ready&&!s->data&&!s->bytes);
 assert(TJTSTORE(&p,&foreign,old,sizeof old)==8&&TJTDROP(&p,&foreign)==8);
 s->writer=19U;assert(TJTDROP(&p,s)==16&&TJTEND(&p)==12&&s->live);s->writer=0U;
 assert(!TJTDROP(&p,s)&&active==0U);
 puts("temporary NEW/OLD, empty, atomic replace, failed allocation/release and retained cleanup controls passed");return 0;
}
