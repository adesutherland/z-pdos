/* SPDX-License-Identifier: MIT; explicit descriptor lifecycle guest oracle. */
#include <stdio.h>
#include <string.h>
#include "twospace_ui.h"
#include "twospace_job.h"
static void init(unsigned char *h,unsigned int n)
{memset(h,0,n);TJBPUT(h,1U);TJBPUT(h+4U,n);}
static void pointer(unsigned char *h,const void *p)
{TJBPUT(h,0U);TJBPUT(h+4U,(unsigned int)(unsigned long)p);}
static void name(unsigned char *p,const char *s)
{unsigned int n=(unsigned int)strlen(s);memset(p,' ',8U);memcpy(p,s,n);}
static void descriptor(unsigned char *d,const char *mode,unsigned int count,unsigned int quota)
{unsigned int i;init(d,TJB_DESC_BYTES);name(d+8U,"PROOF");TJBPUT(d+16U,count);TJBPUT(d+20U,quota);
 for(i=0U;i<count;++i){unsigned char *s=d+32U+i*TJB_STEP_BYTES;name(s,i?"SECOND":"FIRST");name(s+8U,!strcmp(mode,"missing")?"NOJOBPGM":"D4STEP");
  TJBPUT(s+16U,(unsigned int)strlen(mode));memcpy(s+24U,mode,strlen(mode));if(i)TJBPUT(s+20U,1U);}}
static unsigned int action(unsigned char *h,unsigned int id,unsigned int op)
{init(h,128U);TJBPUT(h+8U,id);return TUIIO(128U,h,op);}
static unsigned int submit(unsigned char *h,unsigned char *d,unsigned int *id)
{init(h,128U);TJBPUT(h+16U,TJB_DESC_BYTES);pointer(h+24U,d);
 if(TUIIO(128U,h,TSA_IO_JOB_SUBMIT))return 1U;*id=TJBWORD(h+8U);return 0U;}
static unsigned int printer(unsigned char *h,unsigned int *handle)
{init(h,64U);TJBPUT(h+12U,2U);TJBPUT(h+16U,14U);if(TUIIO(64U,h,TSA_IO_BATCH_OPEN))return 1U;*handle=TJBWORD(h+8U);return 0U;}
static unsigned int print(unsigned char *h,unsigned int id,unsigned int handle)
{init(h,128U);TJBPUT(h+8U,id);TJBPUT(h+20U,handle);return TUIIO(128U,h,TSA_IO_JOB_PRINT);}
static unsigned int wait(const char *prompt)
{unsigned char reply[256];unsigned int n=0U;return TUILINE((const unsigned char *)prompt,(unsigned int)strlen(prompt))||TUIREAD(reply,sizeof reply,&n)||n!=2U||memcmp(reply,"GO",2U);}
#define CHECK(x) do{if(!(x)){printf("PD033 JOB FAILURE LINE %u\n",(unsigned int)__LINE__);return __LINE__;}}while(0)
int main(int argc,char **argv)
{
    unsigned char h[128],d[TJB_DESC_BYTES],step[160],out[4096];unsigned int a,b,p,old,i,n;
    CHECK(!TUIOPEN((const unsigned char *)"PD033 serial jobs",17U));
    if(argc>1&&!strcmp(argv[1],"interrupt")){
        descriptor(d,"spin",1U,4096U);CHECK(!submit(h,d,&a));
        CHECK(!TUILINE((const unsigned char *)"PD033 RESTART CUT READY",sizeof "PD033 RESTART CUT READY"-1U));action(h,a,TSA_IO_JOB_RUN);return 99;
    }
    if(argc>1&&!strcmp(argv[1],"recover")){
        CHECK(!action(h,0U,TSA_IO_JOB_STATUS));a=TJBWORD(h+48U);CHECK(a&&TJBWORD(h+52U)==TJB_INTERRUPTED);
        CHECK(!action(h,a,TSA_IO_JOB_STATUS)&&TJBWORD(h+72U)==12U&&!TJBWORD(h+76U));
        CHECK(!printer(h,&p));CHECK(!print(h,a,p));CHECK(!action(h,a,TSA_IO_JOB_PURGE));
        descriptor(d,"zero",1U,4096U);CHECK(!submit(h,d,&b)&&b>a);CHECK(!action(h,b,TSA_IO_JOB_RUN)&&TJBWORD(h+48U)==TJB_DONE);
        CHECK(!action(h,b,TSA_IO_JOB_PURGE));CHECK(!TUILINE((const unsigned char *)"PD033 RESTART PASS",18U));return 0;
    }
    descriptor(d,"zero",1U,4096U);
    init(h,sizeof h);TJBPUT(h+16U,sizeof d);pointer(h+24U,h);CHECK(TUIIO(sizeof h,h,TSA_IO_JOB_SUBMIT)==8U);
    d[32U+19U]=101U;init(h,sizeof h);TJBPUT(h+16U,sizeof d);pointer(h+24U,d);CHECK(TUIIO(sizeof h,h,TSA_IO_JOB_SUBMIT)==8U);d[51U]=4U;
    CHECK(!submit(h,d,&a));CHECK(!action(h,a,TSA_IO_JOB_HOLD));CHECK(action(h,a,TSA_IO_JOB_RUN)==16U);CHECK(!action(h,a,TSA_IO_JOB_RELEASE));
    CHECK(!action(h,a,TSA_IO_JOB_RUN)&&TJBWORD(h+48U)==TJB_DONE&&!TJBWORD(h+72U)&&TJBWORD(h+76U)==1U&&!TJBWORD(h+80U));
    descriptor(d,"eight",2U,4096U);CHECK(!submit(h,d,&b)&&b>a);CHECK(action(h,b,TSA_IO_JOB_RUN)==4U);
    CHECK(!printer(h,&p));CHECK(!print(h,a,p)&&TJBWORD(h+68U)==TJB_PRINT_DONE);
    CHECK(!action(h,a,TSA_IO_JOB_PURGE));CHECK(action(h,a,TSA_IO_JOB_STATUS)==8U);
    CHECK(!action(h,b,TSA_IO_JOB_RUN)&&TJBWORD(h+48U)==TJB_DONE&&TJBWORD(h+80U)==8U);
    init(h,sizeof h);TJBPUT(h+8U,b);TJBPUT(h+12U,2U);TJBPUT(h+16U,sizeof step);pointer(h+24U,step);
    CHECK(!TUIIO(sizeof h,h,TSA_IO_JOB_STATUS)&&TJBWORD(step+148U)==TJB_SKIP&&TJBWORD(step+144U)==1U);
    CHECK(!wait("PD033 JOB PRINTER OFFLINE READY"));CHECK(print(h,b,p)==12U&&TJBWORD(h+68U)==TJB_PRINT_FAILED&&TJBWORD(h+80U)==8U);
    init(h,64U);TJBPUT(h+8U,p);CHECK(!TUIIO(64U,h,TSA_IO_BATCH_CLOSE));
    CHECK(!wait("PD033 JOB PRINTER RESTORE READY"));CHECK(!printer(h,&p));CHECK(!print(h,b,p)&&TJBWORD(h+68U)==TJB_PRINT_DONE&&TJBWORD(h+96U)==2U);
    init(h,sizeof h);TJBPUT(h+8U,b);TJBPUT(h+16U,sizeof out);pointer(h+24U,out);CHECK(!TUIIO(sizeof h,h,TSA_IO_JOB_OUTPUT));n=TJBWORD(h+32U);
    for(i=0U;i<n;){CHECK(TJBWORD(out+i)>0U&&TJBWORD(out+i+8U)<=132U);i+=24U+TJBWORD(out+i+8U);}CHECK(i==n);
    CHECK(!action(h,b,TSA_IO_JOB_PURGE));old=b;
    descriptor(d,"missing",2U,4096U);CHECK(!submit(h,d,&a)&&a>old);CHECK(!action(h,a,TSA_IO_JOB_RUN)&&TJBWORD(h+48U)==TJB_FAILED&&TJBWORD(h+72U)==28U&&!TJBWORD(h+76U));CHECK(!action(h,a,TSA_IO_JOB_PURGE));
    descriptor(d,"fault",2U,4096U);CHECK(!submit(h,d,&a));CHECK(!action(h,a,TSA_IO_JOB_RUN)&&TJBWORD(h+48U)==TJB_FAILED&&TJBWORD(h+72U)==12U&&!TJBWORD(h+76U));CHECK(!action(h,a,TSA_IO_JOB_PURGE));
    descriptor(d,"quota",1U,256U);CHECK(!submit(h,d,&a));CHECK(!action(h,a,TSA_IO_JOB_RUN)&&TJBWORD(h+48U)==TJB_FAILED&&TJBWORD(h+72U)==4U&&!TJBWORD(h+76U));CHECK(!action(h,a,TSA_IO_JOB_PURGE));
    descriptor(d,"storage",1U,4096U);CHECK(!submit(h,d,&a));CHECK(!action(h,a,TSA_IO_JOB_RUN)&&TJBWORD(h+48U)==TJB_DONE&&!TJBWORD(h+80U));CHECK(!action(h,a,TSA_IO_JOB_PURGE));
    descriptor(d,"spin",1U,4096U);CHECK(!submit(h,d,&a));
    CHECK(!TUILINE((const unsigned char *)"PD033 CPU CANCEL READY",22U));
    CHECK(!action(h,a,TSA_IO_JOB_RUN)&&TJBWORD(h+48U)==TJB_CANCELLED&&TJBWORD(h+72U)==16U&&!TJBWORD(h+76U));CHECK(!action(h,a,TSA_IO_JOB_PURGE));
    descriptor(d,"wait",1U,4096U);CHECK(!submit(h,d,&a));CHECK(!wait("PD033 IO CANCEL READY"));
    CHECK(!action(h,a,TSA_IO_JOB_RUN)&&TJBWORD(h+48U)==TJB_CANCELLED&&TJBWORD(h+72U)==16U);CHECK(!action(h,a,TSA_IO_JOB_PURGE));
    CHECK(!wait("PD033 JOB ISOLATION READY"));
    CHECK(!TUILINE((const unsigned char *)"PD033 JOB PASS",14U));return 0;
}
