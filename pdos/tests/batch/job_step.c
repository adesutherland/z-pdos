/* SPDX-License-Identifier: MIT; Stage 2 native lifecycle probe, no batch DDs. */
#include <string.h>
#include <stdlib.h>
#include "twospace_ui.h"
#include "twospace_job.h"
static void init(unsigned char *h,unsigned int n)
{memset(h,0,n);TJBPUT(h,1U);TJBPUT(h+4U,n);}
static void pointer(unsigned char *h,const void *p)
{TJBPUT(h,0U);TJBPUT(h+4U,(unsigned int)(unsigned long)p);}
static unsigned int emit(unsigned int notice,const unsigned char *p,unsigned int n)
{unsigned char h[128];init(h,sizeof h);TJBPUT(h+12U,notice);TJBPUT(h+16U,n);TJBPUT(h+20U,9U);pointer(h+24U,p);memcpy(h+64U,"SYSOUT  ",8U);return TUIIO(sizeof h,h,TSA_IO_JOB_EMIT);}
int main(int argc,char **argv)
{
    unsigned char h[64],card[80],line[132];unsigned int handle,i;
    const char *mode=argc>1?argv[1]:"zero";
    /* Operator bytes stay available for the controller after the batch job. */
    init(h,sizeof h);if(TUIIO(1U,card,TSA_IO_LINE_READ)!=20U)return 70;
    if(!strcmp(mode,"fault")){volatile unsigned int *bad=(volatile unsigned int *)0xfffffff0U;return (int)*bad;}
    if(!strcmp(mode,"spin")){volatile unsigned int tick=0U;for(;;)++tick;}
    if(!strcmp(mode,"storage")){void *p=malloc(300U*1024U*1024U);if(p){free(p);return 77;}return 0;}
    if(!strcmp(mode,"wait")){
        init(h,sizeof h);TJBPUT(h+12U,1U);TJBPUT(h+16U,12U);
        if(TUIIO(sizeof h,h,TSA_IO_BATCH_OPEN))return 71;handle=TJBWORD(h+8U);
        if(emit(1U,(const unsigned char *)"WAIT DEVICE",11U))return 72;
        init(h,sizeof h);TJBPUT(h+8U,handle);TJBPUT(h+24U,sizeof card);pointer(h+32U,card);
        /* Cancellation can retire this read without resuming the step. */
        return TUIIO(sizeof h,h,TSA_IO_BATCH_READ)?73:74;
    }
    if(!strcmp(mode,"quota")){
        memset(line,'Q',sizeof line);for(i=0U;i<8U;++i)if(emit(0U,line,sizeof line)==4U)return 0;return 75;
    }
    if(emit(0U,(const unsigned char *)"NATIVE STEP RECORD",18U))return 76;
    return !strcmp(mode,"eight")?8:0;
}
