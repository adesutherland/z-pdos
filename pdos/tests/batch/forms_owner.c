/* SPDX-License-Identifier: MIT; native nested form-key ownership control. */
#include <stdio.h>
#include <string.h>
#include "twospace_ui.h"
extern int FONEST(unsigned int);
extern int FOADDR(void);
static void put(unsigned char *p,unsigned int v)
{p[0]=(unsigned char)(v>>24);p[1]=(unsigned char)(v>>16);p[2]=(unsigned char)(v>>8);p[3]=(unsigned char)v;}
static int emit(const char *p,unsigned int batch)
{
 unsigned char h[128];unsigned int n=(unsigned int)strlen(p);
 if(!batch){
  /* This child borrows semantic output, never a Workbench controller. */
  memset(h,0,32U);put(h,1U);put(h+4U,32U);put(h+12U,n);
  put(h+20U,(unsigned int)(unsigned long)p);
  return (int)TUIIO(32U,h,TSA_IO_LINE_WRITE);
 }
 memset(h,0,sizeof h);put(h,1U);put(h+4U,sizeof h);put(h+16U,n);put(h+20U,9U);
 put(h+28U,(unsigned int)(unsigned long)p);memcpy(h+64U,"SYSPRINT",8U);
 return (int)TUIIO(sizeof h,h,TSA_IO_JOB_EMIT);
}
int main(int argc,char **argv)
{
 char record[133];unsigned int batch=argc==2&&!strcmp(argv[1],"BATCH"),expected=batch?20U:16U;
 unsigned int form=TUICTRL(TSA_UI_KEYS_FORM),shell=TUICTRL(TSA_UI_KEYS_SHELL);
 sprintf(record,"PD033 OWNER PARENT REFUSED FORM=%u SHELL=%u",form,shell);
 if(emit(record,batch)||form!=expected||shell!=expected)return 41;
 if(FONEST(batch))return 42;
 sprintf(record,"PD033 OWNER CHILD PASS EXPECTED=%u",expected);if(emit(record,batch))return 43;
 /* BADDR31 is the qualified batch-entry image and requires its real DDs. */
 if(batch&&FOADDR())return 45;
 memset(record,'X',132U);memcpy(record,"PD033 LEFT EDGE ",16U);memcpy(record+116U,"RIGHT EDGE PD033",16U);record[132]=0;
 if(emit(record,batch)||emit(batch?"PD033 OWNER NESTED AND ADDRESS PASS":"PD033 OWNER FOREGROUND CHILD PASS",batch))return 46;
 return 0;
}
