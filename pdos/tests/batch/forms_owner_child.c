/* SPDX-License-Identifier: MIT; a child cannot change its parent's key mode. */
#include <stdio.h>
#include <string.h>
#include "twospace_ui.h"
static void put(unsigned char *p,unsigned int v)
{p[0]=(unsigned char)(v>>24);p[1]=(unsigned char)(v>>16);p[2]=(unsigned char)(v>>8);p[3]=(unsigned char)v;}
int main(int argc,char **argv)
{
 char text[80];unsigned char request[32];
 unsigned int batch=argc==2&&!strcmp(argv[1],"BATCH"),expected=batch?20U:16U;
 unsigned int form=TUICTRL(TSA_UI_KEYS_FORM),shell=TUICTRL(TSA_UI_KEYS_SHELL);
 if(!batch){
  sprintf(text,"PD033 OWNER CHILD REFUSED FORM=%u SHELL=%u",form,shell);
  memset(request,0,sizeof request);put(request,1U);put(request+4U,sizeof request);
  put(request+12U,(unsigned int)strlen(text));put(request+20U,(unsigned int)(unsigned long)text);
  if(TUIIO(sizeof request,request,TSA_IO_LINE_WRITE))return 50;
 }
 return form==expected&&shell==expected?0:49;
}
