/* SPDX-License-Identifier: MIT; explicit immutable dataset/temporary jobs. */
#include <stdio.h>
#include <string.h>
#include "twospace_ui.h"
#include "twospace_job.h"
#include "twospace_jobdd.h"
static unsigned char desc[TJB_DESC_BYTES],allocs[TJD_BYTES],h[128];static unsigned int count,data_at,id,printer;
static void init(unsigned char *p,unsigned int n){memset(p,0,n);TJBPUT(p,1U);TJBPUT(p+4U,n);}
static void pointer(unsigned char *p,const void *a){TJBPUT(p,0U);TJBPUT(p+4U,(unsigned int)(unsigned long)a);}
static void name(unsigned char *p,const char *s){memset(p,' ',8U);memcpy(p,s,strlen(s));}
static unsigned char *dd(unsigned int step,const char *label,unsigned int kind,unsigned int mode,unsigned int fmt,unsigned int lr,unsigned int bl)
{unsigned char *e=allocs+32U+count++*TJD_ENTRY;TJBPUT(e,step);TJBPUT(e+4U,kind);TJBPUT(e+8U,mode);
 TJBPUT(e+12U,fmt);TJBPUT(e+16U,lr);TJBPUT(e+20U,bl);name(e+32U,label);return e;}
static void standard(unsigned int step)
{dd(step,"SYSIN",TJD_DUMMY,TJD_INPUT,TJD_FB,80U,80U);dd(step,"SYSPRINT",TJD_SYSOUT,TJD_OUTPUT,TJD_FB,80U,80U);dd(step,"SYSTERM",TJD_SYSOUT,TJD_OUTPUT,TJD_FB,80U,80U);}
static void dataset(unsigned int step,const char *label,const char *dsn,const char *member,unsigned int mode,unsigned int fmt,unsigned int lr,unsigned int bl)
{unsigned char *e=dd(step,label,TJD_DATASET,mode,fmt,lr,bl);memset(e+40U,' ',44U);memcpy(e+40U,dsn,strlen(dsn));if(member)name(e+84U,member);}
static void temp(unsigned int step,unsigned int mode,unsigned int status,unsigned int disp,unsigned int quota)
{unsigned char *e=dd(step,"WORK",TJD_TEMP,mode,TJD_FB,32U,64U);memset(e+40U,' ',44U);name(e+40U,"WORK");TJBPUT(e+24U,quota);TJBPUT(e+92U,status<<16|disp<<8);}
static void start(unsigned int steps,const char *mode)
{unsigned int i;init(desc,sizeof desc);name(desc+8U,"DDDATA");TJBPUT(desc+16U,steps);TJBPUT(desc+20U,8192U);
 init(allocs,sizeof allocs);TJBPUT(allocs,2U);TJBPUT(allocs+12U,steps);TJBPUT(allocs+16U,1U);count=0U;data_at=TJD_DATA;
 for(i=0U;i<steps;++i){unsigned char *s=desc+32U+i*TJB_STEP_BYTES;char n[2];n[0]=(char)('A'+i);n[1]=0;name(s,n);name(s+8U,"D4STEP");
  if(mode){TJBPUT(s+16U,(unsigned int)strlen(mode));memcpy(s+24U,mode,strlen(mode));}}
}
static void mode(unsigned int step,const char *s)
{unsigned char *p=desc+32U+(step-1U)*TJB_STEP_BYTES;memset(p+24U,0,100U);TJBPUT(p+16U,(unsigned int)strlen(s));memcpy(p+24U,s,strlen(s));}
static unsigned int action(unsigned int op)
{init(h,sizeof h);TJBPUT(h+8U,id);return TUIIO(sizeof h,h,op);}
static unsigned int run(unsigned int expected,unsigned int os)
{TJBPUT(allocs+8U,count);init(h,sizeof h);TJBPUT(h+16U,sizeof desc);pointer(h+24U,desc);
 if(TUIIO(sizeof h,h,TSA_IO_JOB_SUBMIT))return 1U;id=TJBWORD(h+8U);
 init(h,sizeof h);TJBPUT(h+8U,id);TJBPUT(h+16U,sizeof allocs);pointer(h+24U,allocs);if(TUIIO(sizeof h,h,TSA_IO_JOB_BIND))return 2U;
 if(action(TSA_IO_JOB_RUN)||TJBWORD(h+48U)!=expected||TJBWORD(h+72U)!=os||
    (os?TJBWORD(h+76U)!=0U:TJBWORD(h+76U)!=1U||TJBWORD(h+80U)))return 3U;
 init(h,sizeof h);TJBPUT(h+8U,id);TJBPUT(h+20U,printer);if(TUIIO(sizeof h,h,TSA_IO_JOB_PRINT))return 4U;
 return action(TSA_IO_JOB_PURGE);}
#define CHECK(x) do{if(!(x)){printf("PD033 DATA FAILURE LINE %u ID %u\n",(unsigned int)__LINE__,id);return __LINE__;}}while(0)
int main(void)
{
 unsigned int s,i;unsigned char *e,reply[8];
 CHECK(!TUIOPEN((const unsigned char *)"Native DD data",14U));init(h,64U);TJBPUT(h+12U,2U);TJBPUT(h+16U,14U);
 CHECK(!TUIIO(64U,h,TSA_IO_BATCH_OPEN));printer=TJBWORD(h+8U);
 start(2U,"write");mode(2U,"read");
 for(s=1U;s<=2U;++s){standard(s);
  dataset(s,s==1U?"PSOUT":"PSIN","PD3.OUT",0,s==1U?TJD_OUTPUT:TJD_INPUT,TJD_FB,32U,64U);
  dataset(s,s==1U?"MEMOUT":"MEMIN","PD3.LIB","ONE",s==1U?TJD_OUTPUT:TJD_INPUT,TJD_VB,20U,64U);
  dataset(s,"SECOND","PD3.LIB","SECOND",s==1U?TJD_OUTPUT:TJD_INPUT,TJD_VB,20U,64U);
  dataset(s,"ZERO","PD3.LIB","EMPTY",s==1U?TJD_OUTPUT:TJD_INPUT,TJD_VB,20U,64U);}
 CHECK(!run(TJB_DONE,0U));
 start(1U,"replace");standard(1U);
 dataset(1U,"MEMOUT","PD3.LIB","ONE",TJD_OUTPUT,TJD_VB,20U,64U);dataset(1U,"MEMIN","PD3.LIB","ONE",TJD_INPUT,TJD_VB,20U,64U);
 dataset(1U,"SECOND","PD3.LIB","SECOND",TJD_INPUT,TJD_VB,20U,64U);CHECK(!run(TJB_DONE,0U));
 start(1U,"concat");standard(1U);
 e=dd(1U,"JOIN",TJD_INLINE,TJD_INPUT,TJD_FB,32U,32U);TJBPUT(e+24U,data_at);TJBPUT(e+28U,34U);allocs[data_at+1U]=32U;
 for(i=0U;i<32U;++i)allocs[data_at+2U+i]=(unsigned char)(64U+i);data_at+=34U;
 e=dd(1U,"JOIN",TJD_DUMMY,TJD_INPUT,TJD_FB,32U,32U);TJBPUT(e+92U,1U);
 dataset(1U,"JOIN","PD3.FB",0,TJD_INPUT,TJD_FB,32U,64U);TJBPUT(allocs+32U+(count-1U)*TJD_ENTRY+92U,2U);
 dataset(1U,"JOIN","PD3.FBLIB","INPUT",TJD_INPUT,TJD_FB,32U,32U);TJBPUT(allocs+32U+(count-1U)*TJD_ENTRY+92U,3U);
 CHECK(!run(TJB_DONE,0U));
 start(3U,"tempwrite");mode(2U,"tempkeep");mode(3U,"tempdelete");
 for(s=1U;s<=3U;++s){standard(s);temp(s,s==1U?TJD_OUTPUT:TJD_INPUT,s==1U?TJD_NEW:TJD_OLD,s==1U?TJD_PASS:s==2U?TJD_KEEP:TJD_DELETE,4096U);}
 CHECK(!run(TJB_DONE,0U));
 start(2U,"tempwrite");mode(2U,"tempdelete");standard(1U);temp(1U,TJD_OUTPUT,TJD_NEW,TJD_DELETE,4096U);standard(2U);temp(2U,TJD_INPUT,TJD_OLD,TJD_KEEP,4096U);
 CHECK(!run(TJB_FAILED,12U));
 start(2U,"tempwrite");mode(2U,"tempkeep");standard(1U);temp(1U,TJD_OUTPUT,TJD_NEW,TJD_KEEP,4096U);standard(2U);temp(2U,TJD_INPUT,TJD_OLD,TJD_KEEP,4096U);
 CHECK(!run(TJB_DONE,0U));
 start(1U,"tempdelete");standard(1U);temp(1U,TJD_INPUT,TJD_OLD,TJD_KEEP,4096U);CHECK(!run(TJB_FAILED,12U));
 start(2U,"tempwrite");mode(2U,"tempreplace");standard(1U);temp(1U,TJD_OUTPUT,TJD_NEW,TJD_PASS,4096U);standard(2U);temp(2U,TJD_OUTPUT,TJD_OLD,TJD_KEEP,4096U);
 temp(2U,TJD_INPUT,TJD_OLD,TJD_KEEP,4096U);name(allocs+32U+(count-1U)*TJD_ENTRY+32U,"TREAD");CHECK(!run(TJB_DONE,0U));
 start(2U,"tempempty");mode(2U,"tempzero");standard(1U);temp(1U,TJD_OUTPUT,TJD_NEW,TJD_PASS,4096U);standard(2U);temp(2U,TJD_INPUT,TJD_OLD,TJD_DELETE,4096U);CHECK(!run(TJB_DONE,0U));
 start(1U,"quota");standard(1U);temp(1U,TJD_OUTPUT,TJD_NEW,TJD_PASS,64U);CHECK(!run(TJB_FAILED,4U));
 start(1U,"fault");standard(1U);dataset(1U,"PSOUT","PD3.OUT",0,TJD_OUTPUT,TJD_FB,32U,64U);temp(1U,TJD_OUTPUT,TJD_NEW,TJD_KEEP,4096U);
 CHECK(!run(TJB_FAILED,12U));
 start(1U,"tempdelete");standard(1U);temp(1U,TJD_INPUT,TJD_OLD,TJD_KEEP,4096U);CHECK(!run(TJB_FAILED,12U));
 CHECK(!TUILINE((const unsigned char *)"PD033 DATA ISOLATION READY",sizeof "PD033 DATA ISOLATION READY"-1U));
 CHECK(!TUIREAD(reply,sizeof reply,&i)&&i==2U&&!memcmp(reply,"GO",2U));
 CHECK(!TUILINE((const unsigned char *)"PD033 DATA PASS",15U));return 0;
}
