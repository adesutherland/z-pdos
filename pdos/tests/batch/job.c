/* SPDX-License-Identifier: MIT; independent state, bounds and recovery cases. */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "twospace_job.h"
static void name(unsigned char *p,unsigned char first)
{memset(p,0x40,8U);p[0]=first;}
static void desc(unsigned char *d,unsigned int count,unsigned int quota)
{unsigned int i;memset(d,0,TJB_DESC_BYTES);TJBPUT(d,1U);TJBPUT(d+4U,TJB_DESC_BYTES);name(d+8U,0xd1U);TJBPUT(d+16U,count);TJBPUT(d+20U,quota);
 for(i=0U;i<count;++i){unsigned char *s=d+32U+i*TJB_STEP_BYTES;name(s,0xe2U);s[1U]=(unsigned char)(0xf0U+i);name(s+8U,0xd7U);}}
int main(void)
{
    unsigned char *b=malloc(TJB_BYTES),*old=malloc(TJB_BYTES),*j,*s;
    unsigned char d[TJB_DESC_BYTES],line[132],dd[8];TJBPOOL p;unsigned int a,q,last;int rc;
    assert(b&&old);assert(!TJBINIT(&p,b,TJB_BYTES));memset(line,0xc1,sizeof line);name(dd,0xe2U);
    desc(d,2U,4096U);assert(!TJBCHECK(d,sizeof d));d[32U+16U+3U]=101U;assert(TJBCHECK(d,sizeof d)==TJB_BAD);d[51U]=0U;
    d[sizeof d-1U]=1U;assert(TJBCHECK(d,sizeof d)==TJB_BAD);d[sizeof d-1U]=0U;
    assert(!TJBSUB(&p,7U,d,sizeof d,&a)&&a==1U);assert(TJBSUB(&p,7U,d,sizeof d,&q)==TJB_FULL);
    assert(!TJBACT(&p,7U,a,1U)&&TJBSTART(&p,7U,a)==TJB_BUSY);assert(!TJBACT(&p,7U,a,2U));
    assert(TJBACT(&p,8U,a,4U)==TJB_BAD);assert(!TJBSTART(&p,7U,a));assert(!TJBNEXT(&p,7U,a));
    assert(!TJBSUB(&p,7U,d,sizeof d,&q)&&q==2U);assert(TJBSTART(&p,7U,q)==TJB_BUSY);assert(TJBACT(&p,7U,a,4U)==TJB_BUSY);
    assert(!TJBAPP(&p,7U,a,dd,9U,line,132U));assert(TJBAPP(&p,8U,a,dd,9U,line,1U)==TJB_BAD);
    assert(TJBWORD(TJBOUT(&p,a))==1U&&TJBWORD(TJBOUT(&p,a)+4U)==1U&&TJBWORD(TJBOUT(&p,a)+8U)==132U);
    assert(!TJBEND(&p,7U,a,0U,1U,8U,0U,0U));j=TJBFIND(&p,7U,a);s=TJBSTEP(j,1U);TJBPUT(s+20U,TJB_COND_NE);
    assert(TJBNEXT(&p,7U,a)==1&&TJBWORD(j+8U)==TJB_DONE&&TJBWORD(j+40U)==8U&&TJBWORD(s+144U)==1U);
    assert(!TJBAPP(&p,7U,a,dd,9U,line,1U));memcpy(old,b,TJB_BYTES);
    {TJBPOOL recovered;assert(!TJBLOAD(&recovered,old,TJB_BYTES));
     assert(TJBWORD(TJBFIND(&recovered,7U,a)+8U)==TJB_DONE&&TJBWORD(TJBFIND(&recovered,7U,q)+8U)==TJB_HELD);}
    assert(TJBSTART(&p,7U,q)==TJB_FULL);assert(!TJBACT(&p,7U,a,4U));assert(!TJBSTART(&p,7U,q));assert(!TJBNEXT(&p,7U,q));
    assert(!TJBEND(&p,7U,q,28U,0U,0U,0U,0U));assert(TJBNEXT(&p,7U,q)==1);j=TJBFIND(&p,7U,q);
    assert(TJBWORD(j+8U)==TJB_FAILED&&TJBWORD(TJBSTEP(j,1U)+144U)==2U&&!TJBWORD(j+36U));
    assert(!TJBACT(&p,7U,q,4U));desc(d,1U,256U);assert(!TJBSUB(&p,7U,d,sizeof d,&a));assert(!TJBSTART(&p,7U,a));assert(!TJBNEXT(&p,7U,a));
    assert(!TJBAPP(&p,7U,a,dd,9U,line,132U));memcpy(old,b,TJB_BYTES);assert(TJBAPP(&p,7U,a,dd,9U,line,132U)==TJB_FULL&&!memcmp(b,old,TJB_BYTES));
    assert(!TJBACT(&p,7U,a,3U));assert(!TJBEND(&p,7U,a,16U,0U,0U,3U,0U));assert(TJBNEXT(&p,7U,a)==1&&TJBWORD(TJBFIND(&p,7U,a)+8U)==TJB_CANCELLED);
    assert(!TJBACT(&p,7U,a,4U));desc(d,2U,4096U);assert(!TJBSUB(&p,7U,d,sizeof d,&a));assert(!TJBSTART(&p,7U,a));assert(!TJBNEXT(&p,7U,a));
    assert(!TJBSUB(&p,7U,d,sizeof d,&q));assert(!TJBAPP(&p,7U,a,dd,9U,line,1U));
    j=TJBFIND(&p,7U,a);TJBPUT(j+28U,TJB_PRINT_ACTIVE);memcpy(old,b,TJB_BYTES);
    TJBPUT(TJBOUT(&p,a)+8U,133U);memcpy(old,b,TJB_BYTES);rc=TJBLOAD(&p,b,TJB_BYTES);assert(rc==TJB_BAD&&!memcmp(old,b,TJB_BYTES));TJBPUT(TJBOUT(&p,a)+8U,1U);
    assert(!TJBLOAD(&p,b,TJB_BYTES));j=TJBFIND(&p,7U,a);
    assert(TJBWORD(j+8U)==TJB_INTERRUPTED&&TJBWORD(j+28U)==TJB_PRINT_FAILED&&TJBWORD(TJBSTEP(j,0U)+148U)==TJB_INTERRUPT);
    assert(TJBWORD(TJBSTEP(j,1U)+144U)==4U&&TJBWORD(TJBFIND(&p,7U,q)+8U)==TJB_HELD&&!TJBWORD(b+12U));
    assert(!TJBACT(&p,7U,a,4U));assert(!TJBACT(&p,7U,q,3U));assert(!TJBACT(&p,7U,q,4U));last=TJBWORD(b+8U);
    assert(!TJBSUB(&p,7U,d,sizeof d,&a)&&a>last);assert(!TJBACT(&p,7U,a,3U));assert(!TJBACT(&p,7U,a,4U));
    TJBPUT(b+8U,0xffffffffU);assert(TJBSUB(&p,7U,d,sizeof d,&a)==TJB_FULL);
    free(old);free(b);puts("job state, record, ownership, quota and interrupted-recovery controls passed");return 0;
}
