/* SPDX-License-Identifier: MIT; command semantics and exact host boundary. */
#include "twospace_command_router.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static unsigned int writes,missing,ops,dated_clock;static char last[256];
static unsigned int host(unsigned int op,unsigned char *p,unsigned int n)
{
    ++ops;
    if(op==TCM_WRITE){assert(n<sizeof last);memcpy(last,p,n);last[n]=0;++writes;return 0;}
    if(op==TCM_CLOCK){
        /* IBM TOD: 2026-01-01 12:34:56 UTC, relative to 1900. */
        static const unsigned char tod[8]={0xe2,0x06,0x31,0xab,0x6f,0xc0,0,0};
        assert(n==8);if(dated_clock)memcpy(p,tod,8);else memset(p,0,8);return 0;
    }
    if(op==TCM_STAT_CMS){assert(n==18);assert(!memcmp(p,"ENVINPUTDATA    A1",18));return missing?28U:0U;}
    if(op==TCM_STAT_TSO){assert(n==19);assert(!memcmp(p,"LABA01.ENVCMD.INPUT",19));return missing?28U:0U;}
    if(op==TCM_MEDIA){assert(n==11);assert(!memcmp(p,"DIR  HELLO*",11));return 4U;}
    assert(0);return 8U;
}
unsigned int TCMIO(unsigned int op,unsigned char *p,unsigned int n){return host(op,p,n);}
static void run(unsigned int env,const char *p,unsigned int status,int expected)
{int rc=-99;assert(TCMDISP(env,(const unsigned char *)p,(unsigned int)strlen(p),host,&rc)==status);if(!status)assert(rc==expected);}
int main(void)
{
    unsigned int before;int rc;
    run(TCM_CMS,"QUERY TIME",0U,0);assert(!strcmp(last,"TIME 00:00:00 UTC"));
    run(TCM_CMS,"STATE  ENVINPUT DATA A",0U,0);
    missing=1;run(TCM_CMS,"STATE ENVINPUT DATA A",0U,28);missing=0;
    run(TCM_CMS,"ZZNOEXST",0U,-3);
    run(TCM_CMS,"STATE ENVINPUTTOOLONG DATA A",0U,24);
    run(TCM_CMS,"STATE ENVINPUT DATA A extra",0U,24);
    run(TCM_TSO,"TIME",0U,0);
    dated_clock=1;run(TCM_TSO,"TIME",0U,0);assert(!strcmp(last,"TIME 12:34:56 UTC"));
    run(TCM_TSO,"LISTDS  'LABA01.ENVCMD.INPUT'",0U,0);
    missing=1;run(TCM_TSO,"LISTDS 'LABA01.ENVCMD.INPUT'",0U,8);missing=0;
    run(TCM_TSO,"LISTDS 'missing",0U,12);
    run(TCM_PDOS,"echo  mixed Case and 'quotes'",0U,0);assert(!strcmp(last," mixed Case and 'quotes'"));
    run(TCM_PDOS,"VERSION",0U,0);run(TCM_PDOS,"DIR  HELLO*",0U,4);
    run(TCM_PDOS,"echo off",TCM_UNHANDLED,0);run(TCM_PDOS,"RXVM -v",TCM_UNHANDLED,0);
    before=ops;
    assert(TCMDISP(TCM_CMS,(const unsigned char *)"x\0x",3,host,&rc)==8U);
    assert(TCMDISP(TCM_CMS,(const unsigned char *)"x\nx",3,host,&rc)==8U);
    assert(TCMDISP(0,(const unsigned char *)"x",1,host,&rc)==8U);
    assert(TCMDISP(TCM_CMS,(const unsigned char *)"x",TCM_MAX+1U,host,&rc)==8U);
    assert(ops==before);assert(writes>10);
    puts("COMMAND ROUTER PASS");return 0;
}
