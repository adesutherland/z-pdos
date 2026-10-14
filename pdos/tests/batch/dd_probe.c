/* SPDX-License-Identifier: MIT; prebound DD controller, outside batch. */
#include <stdio.h>
#include <string.h>
#include "twospace_ui.h"
#include "twospace_job.h"
#include "twospace_jobdd.h"
static void init(unsigned char *h,unsigned int n)
{memset(h,0,n);TJBPUT(h,1U);TJBPUT(h+4U,n);}
static void pointer(unsigned char *h,const void *p)
{TJBPUT(h,0U);TJBPUT(h+4U,(unsigned int)(unsigned long)p);}
static void name(unsigned char *p,const char *s)
{memset(p,' ',8U);memcpy(p,s,strlen(s));}
static void dd(unsigned char *e,const char *label,unsigned int kind,unsigned int mode,unsigned int fmt,unsigned int lr,unsigned int bl)
{TJBPUT(e,1U);TJBPUT(e+4U,kind);TJBPUT(e+8U,mode);TJBPUT(e+12U,fmt);TJBPUT(e+16U,lr);TJBPUT(e+20U,bl);name(e+32U,label);}
#define CHECK(x) do{if(!(x)){printf("PD033 DD FAILURE LINE %u\n",(unsigned int)__LINE__);return __LINE__;}}while(0)
int main(void)
{
    unsigned char h[128],d[TJB_DESC_BYTES],a[TJD_BYTES],reply[8];unsigned int id,p,i,n;
    CHECK(!TUIOPEN((const unsigned char *)"PD033 native batch DDs",sizeof "PD033 native batch DDs"-1U));
    init(d,sizeof d);name(d+8U,"DDPROOF");TJBPUT(d+16U,1U);TJBPUT(d+20U,8192U);
    name(d+32U,"FIRST");name(d+40U,"D4STEP");
    init(h,sizeof h);TJBPUT(h+16U,sizeof d);pointer(h+24U,d);
    CHECK(!TUIIO(sizeof h,h,TSA_IO_JOB_SUBMIT));id=TJBWORD(h+8U);
    init(a,sizeof a);TJBPUT(a+8U,7U);TJBPUT(a+12U,1U);TJBPUT(a+16U,1U);
    dd(a+32U,"SYSIN",TJD_INLINE,TJD_INPUT,TJD_FB,128U,256U);
    TJBPUT(a+56U,TJD_DATA);TJBPUT(a+60U,260U);
    a[TJD_DATA+1U]=128U;a[TJD_DATA+131U]=128U;
    for(i=0U;i<128U;++i){a[TJD_DATA+2U+i]=(unsigned char)i;a[TJD_DATA+132U+i]=(unsigned char)(128U+i);}
    dd(a+32U+TJD_ENTRY,"FBOUT",TJD_SYSOUT,TJD_OUTPUT,TJD_FB,80U,80U);
    dd(a+32U+2U*TJD_ENTRY,"VBOUT",TJD_SYSOUT,TJD_OUTPUT,TJD_VB,84U,88U);
    dd(a+32U+3U*TJD_ENTRY,"EMPTY",TJD_DUMMY,TJD_INPUT,TJD_FB,80U,80U);
    dd(a+32U+4U*TJD_ENTRY,"SINK",TJD_DUMMY,TJD_OUTPUT,TJD_FB,80U,80U);
    dd(a+32U+5U*TJD_ENTRY,"SYSPRINT",TJD_SYSOUT,TJD_OUTPUT,TJD_FB,80U,80U);
    dd(a+32U+6U*TJD_ENTRY,"SYSTERM",TJD_SYSOUT,TJD_OUTPUT,TJD_FB,80U,80U);
    init(h,sizeof h);TJBPUT(h+8U,id);TJBPUT(h+16U,sizeof a);pointer(h+24U,a);
    CHECK(!TUIIO(sizeof h,h,TSA_IO_JOB_BIND));CHECK(TUIIO(sizeof h,h,TSA_IO_JOB_BIND)==16U);
    /* Later changes in U cannot change the admitted allocation/data. */
    memset(a,0xee,sizeof a);
    init(h,sizeof h);TJBPUT(h+8U,id);CHECK(!TUIIO(sizeof h,h,TSA_IO_JOB_RUN));
    CHECK(TJBWORD(h+48U)==TJB_DONE&&!TJBWORD(h+72U)&&TJBWORD(h+76U)==1U&&!TJBWORD(h+80U));
    init(h,64U);TJBPUT(h+12U,2U);TJBPUT(h+16U,14U);
    CHECK(!TUIIO(64U,h,TSA_IO_BATCH_OPEN));p=TJBWORD(h+8U);
    init(h,sizeof h);TJBPUT(h+8U,id);TJBPUT(h+20U,p);CHECK(!TUIIO(sizeof h,h,TSA_IO_JOB_PRINT));
    CHECK(!TUILINE((const unsigned char *)"PD033 DD ISOLATION READY",sizeof "PD033 DD ISOLATION READY"-1U));
    CHECK(!TUIREAD(reply,sizeof reply,&n)&&n==2U&&!memcmp(reply,"GO",2U));
    CHECK(!TUILINE((const unsigned char *)"PD033 DD PASS",13U));return 0;
}
