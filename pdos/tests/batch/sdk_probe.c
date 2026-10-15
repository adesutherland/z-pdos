/* SPDX-License-Identifier: MIT; explicit job controller for shared native bytes. */
#include <stdio.h>
#include <string.h>
#include "twospace_ui.h"
#include "twospace_job.h"
#include "twospace_jobdd.h"
static unsigned char desc[TJB_DESC_BYTES],allocs[TJD_BYTES],h[128];
static unsigned int count,data_at,id,printer;
static void init(unsigned char *p,unsigned int n){memset(p,0,n);TJBPUT(p,1U);TJBPUT(p+4U,n);}
static void pointer(unsigned char *p,const void *a){TJBPUT(p,0U);TJBPUT(p+4U,(unsigned int)(unsigned long)a);}
static void name(unsigned char *p,const char *s){memset(p,' ',8U);memcpy(p,s,strlen(s));}
static void start(unsigned int steps)
{init(desc,sizeof desc);name(desc+8U,"SDKBATCH");TJBPUT(desc+16U,steps);TJBPUT(desc+20U,65536U);
 init(allocs,sizeof allocs);TJBPUT(allocs,2U);TJBPUT(allocs+12U,steps);TJBPUT(allocs+16U,1U);count=0U;data_at=TJD_DATA;}
static void step(unsigned int n,const char *program,const char *parm)
{unsigned char *s=desc+32U+(n-1U)*TJB_STEP_BYTES;char label[2];label[0]=(char)('A'+n-1U);label[1]=0;
 name(s,label);name(s+8U,program);TJBPUT(s+16U,(unsigned int)strlen(parm));memcpy(s+24U,parm,strlen(parm));
 if(n>1U)TJBPUT(s+20U,TJB_COND_NE);}
static unsigned char *dd(unsigned int step,const char *label,unsigned int kind,unsigned int mode,unsigned int fmt,unsigned int lr,unsigned int bl)
{unsigned char *e=allocs+32U+count++*TJD_ENTRY;TJBPUT(e,step);TJBPUT(e+4U,kind);TJBPUT(e+8U,mode);
 TJBPUT(e+12U,fmt);TJBPUT(e+16U,lr);TJBPUT(e+20U,bl);name(e+32U,label);return e;}
static void standard(unsigned int s)
{dd(s,"SYSIN",TJD_DUMMY,TJD_INPUT,TJD_FB,80U,80U);dd(s,"SYSPRINT",TJD_SYSOUT,TJD_OUTPUT,TJD_FB,132U,132U);dd(s,"SYSTERM",TJD_SYSOUT,TJD_OUTPUT,TJD_FB,132U,132U);}
static void dataset(unsigned int s,const char *label,const char *dsn,unsigned int mode,unsigned int fmt,unsigned int lr,unsigned int bl)
{unsigned char *e=dd(s,label,TJD_DATASET,mode,fmt,lr,bl);memset(e+40U,' ',44U);memcpy(e+40U,dsn,strlen(dsn));}
static void temporary(unsigned int s,unsigned int mode,unsigned int status,unsigned int disp)
{unsigned char *e=dd(s,"ASM",TJD_TEMP,mode,TJD_VB,4100U,8200U);memset(e+40U,' ',44U);name(e+40U,"ASM");TJBPUT(e+24U,131072U);TJBPUT(e+92U,status<<16|disp<<8);}
static void inline_begin(unsigned char *e){TJBPUT(e+24U,data_at);}
static void inline_line(unsigned char *e,const unsigned char *p,unsigned int n)
{allocs[data_at]=(unsigned char)(n>>8);allocs[data_at+1U]=(unsigned char)n;if(n)memcpy(allocs+data_at+2U,p,n);data_at+=n+2U;TJBPUT(e+28U,data_at-TJBWORD(e+24U));}
static void source(unsigned int bad)
{unsigned char *e=dd(1U,"SRC",TJD_INLINE,TJD_INPUT,TJD_VB,512U,516U);
 static const unsigned char good[]={SDK_SOURCE_BYTES};
 static const unsigned char wrong[]={SDK_BAD_BYTES};
 static const unsigned char address[]={SDK_ADDRESS_BYTES};
 static const unsigned char parse[]={SDK_PARSE_BYTES};
 const unsigned char *p=bad==3U?parse:bad==2U?address:bad?wrong:good;unsigned int n=bad==3U?sizeof parse:bad==2U?sizeof address:bad?sizeof wrong:sizeof good,at=0U,i;
 inline_begin(e);for(i=0U;i<n;++i)if(p[i]==10U){inline_line(e,p+at,i-at);at=i+1U;}if(at<n)inline_line(e,p+at,n-at);}
static unsigned int action(unsigned int op)
{init(h,sizeof h);TJBPUT(h+8U,id);return TUIIO(sizeof h,h,op);}
static unsigned int run(unsigned int os,unsigned int app)
{TJBPUT(allocs+8U,count);if(TJDCHK(allocs,TJBWORD(desc+16U)))return 6U;
 init(h,sizeof h);TJBPUT(h+16U,sizeof desc);pointer(h+24U,desc);
 if(TUIIO(sizeof h,h,TSA_IO_JOB_SUBMIT))return 1U;id=TJBWORD(h+8U);
 init(h,sizeof h);TJBPUT(h+8U,id);TJBPUT(h+16U,sizeof allocs);pointer(h+24U,allocs);if(TUIIO(sizeof h,h,TSA_IO_JOB_BIND))return 2U;
 if(action(TSA_IO_JOB_RUN)||TJBWORD(h+72U)!=os||TJBWORD(h+76U)!=(os?0U:1U)||(!os&&TJBWORD(h+80U)!=app))return 3U;
 init(h,sizeof h);TJBPUT(h+8U,id);TJBPUT(h+20U,printer);if(TUIIO(sizeof h,h,TSA_IO_JOB_PRINT))return 4U;
 return action(TSA_IO_JOB_PURGE);}
#define CHECK(x) do{if(!(x)){printf("PD033 SDK FAILURE LINE %u ID %u\n",(unsigned int)__LINE__,id);return __LINE__;}}while(0)
int main(void)
{unsigned int i;unsigned char *e,reply[8],card[80];
 CHECK(!TUIOPEN((const unsigned char *)"Shared batch SDK",16U));init(h,64U);TJBPUT(h+12U,2U);TJBPUT(h+16U,14U);CHECK(!TUIIO(64U,h,TSA_IO_BATCH_OPEN));printer=TJBWORD(h+8U);
 start(1U);step(1U,"BTEST31","BATCH");standard(1U);e=allocs+32U;TJBPUT(e+4U,TJD_INLINE);inline_begin(e);
 {static const unsigned char input[]={0xc9,0x95,0x97,0xa4,0xa3,0x40,0x83,0x81,0x86,0x51,0x5a};memset(card,0x40,sizeof card);memcpy(card,input,sizeof input);inline_line(e,card,sizeof card);}memset(card,0x40,sizeof card);inline_line(e,card,sizeof card);
 dataset(1U,"BYTES","PD4.BYTES",TJD_INPUT,TJD_FB,256U,256U);dataset(1U,"RESULT","PD4.RESULT",TJD_OUTPUT,TJD_VB,260U,264U);CHECK(!run(0U,0U));
 start(4U);for(i=1U;i<=4U;++i){step(i,i==1U?"BRXC":i==2U?"BRXAS":i==3U?"BRXLINK":"BRXVM",i==3U?"-h":"-v");standard(i);}CHECK(!run(0U,0U));
 start(3U);step(1U,"BRXC","-n -x -i LABA01.B33 -o DD:ASM DD:SRC");standard(1U);source(0U);temporary(1U,TJD_OUTPUT,TJD_NEW,TJD_PASS);
 step(2U,"BRXAS","-o DD:OBJ DD:ASM");standard(2U);temporary(2U,TJD_INPUT,TJD_OLD,TJD_DELETE);dataset(2U,"OBJ","PD4.OBJ",TJD_OUTPUT,TJD_VB,260U,264U);
 step(3U,"BRXVM","DD:OBJ -a SHARED");standard(3U);dataset(3U,"OBJ","PD4.OBJ",TJD_INPUT,TJD_VB,260U,264U);CHECK(!run(0U,0U));
 start(2U);step(1U,"BRXLINK","-o DD:LINK DD:OBJ");standard(1U);dataset(1U,"OBJ","PD4.OBJ",TJD_INPUT,TJD_VB,260U,264U);dataset(1U,"LINK","PD4.LINK",TJD_OUTPUT,TJD_VB,260U,264U);
 step(2U,"BRXVM","DD:LINK -a SHARED");standard(2U);dataset(2U,"LINK","PD4.LINK",TJD_INPUT,TJD_VB,260U,264U);CHECK(!run(0U,0U));
 start(2U);step(1U,"BRXC","-n -x -i LABA01.B33 -o DD:ASM DD:SRC");standard(1U);source(1U);temporary(1U,TJD_OUTPUT,TJD_NEW,TJD_PASS);
 step(2U,"BRXVM","DD:OBJ -a SHARED");standard(2U);dataset(2U,"OBJ","PD4.OBJ",TJD_INPUT,TJD_VB,260U,264U);CHECK(!run(0U,2U));
 start(1U);step(1U,"BRXVM","DD:NOSUCH");standard(1U);CHECK(!run(0U,2U));
 start(1U);step(1U,"BRXVM","DD:OBJ -a WRONG");standard(1U);dataset(1U,"OBJ","PD4.OBJ",TJD_INPUT,TJD_VB,260U,264U);CHECK(!run(0U,8U));
 start(1U);step(1U,"BTEST31","BATCH");standard(1U);--count;memset(allocs+32U+count*TJD_ENTRY,0,TJD_ENTRY);CHECK(!run(0U,16U));
 start(1U);step(1U,"BADDR31","TSO");standard(1U);dd(1U,"SYSTSPRT",TJD_SYSOUT,TJD_OUTPUT,TJD_FB,132U,132U);CHECK(!run(0U,0U));
 start(3U);step(1U,"BRXC","-n -i LABA01.B33 -o DD:ASM DD:SRC");standard(1U);source(2U);temporary(1U,TJD_OUTPUT,TJD_NEW,TJD_PASS);
 step(2U,"BRXAS","-o DD:AOBJ DD:ASM");standard(2U);temporary(2U,TJD_INPUT,TJD_OLD,TJD_DELETE);dataset(2U,"AOBJ","PD4.AOBJ",TJD_OUTPUT,TJD_VB,260U,264U);
 step(3U,"BRXVM","DD:AOBJ DSN:LABA01.B33.RXBIN(LIBRARY)");standard(3U);dataset(3U,"AOBJ","PD4.AOBJ",TJD_INPUT,TJD_VB,260U,264U);dd(3U,"SYSTSPRT",TJD_SYSOUT,TJD_OUTPUT,TJD_FB,132U,132U);CHECK(!run(0U,0U));
 start(3U);step(1U,"BRXC","-n -i LABA01.B33 -o DD:ASM DD:SRC");standard(1U);source(3U);temporary(1U,TJD_OUTPUT,TJD_NEW,TJD_PASS);
 step(2U,"BRXAS","-o DD:POBJ DD:ASM");standard(2U);temporary(2U,TJD_INPUT,TJD_OLD,TJD_DELETE);dataset(2U,"POBJ","PD4.POBJ",TJD_OUTPUT,TJD_VB,260U,264U);
 step(3U,"BRXVM","DD:POBJ DSN:LABA01.B33.RXBIN(LIBRARY)");standard(3U);dataset(3U,"POBJ","PD4.POBJ",TJD_INPUT,TJD_VB,260U,264U);CHECK(!run(0U,0U));
 CHECK(!TUILINE((const unsigned char *)"PD033 SDK ISOLATION READY",sizeof "PD033 SDK ISOLATION READY"-1U));
 CHECK(!TUIREAD(reply,sizeof reply,&i)&&i==2U&&!memcmp(reply,"GO",2U));CHECK(!TUILINE((const unsigned char *)"PD033 SDK PASS",14U));return 0;
}
