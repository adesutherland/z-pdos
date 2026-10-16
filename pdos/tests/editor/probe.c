/* SPDX-License-Identifier: MIT; native editor fixture and safe-save probes.
 * Test-only: fixed disposable LABA01.B33 inputs, no physical DASD writes. */
#include <stdio.h>
#include <string.h>
#include "twospace_ui.h"
#include "apps/edit_session.h"
#include "apps/edit_text.h"
#define ORIGINAL "LABA01.B33.SRC(LIBRARY)"
#define SEED "LABA01.B33.SRC(EDSEED)"
#define SAVED "LABA01.B33.SRC(EDSAVE)"
static unsigned char original[TEF_LIMIT],seed[TEF_LIMIT],other[TEF_LIMIT];
static unsigned char original_info[TEF_BYTES],info[TEF_BYTES],fresh[TEF_BYTES];
static EDCSTATE snapshot;
static void put(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static int emit(const char *text,int batch)
{
 unsigned char h[128];unsigned int n=(unsigned int)strlen(text);
 if(!batch)return (int)TUILINE((const unsigned char *)text,n);
 memset(h,0,sizeof h);put(h,1U);put(h+4U,sizeof h);put(h+16U,n);put(h+20U,9U);
 put(h+28U,(unsigned int)(unsigned long)text);memcpy(h+64U,"SYSPRINT",8U);
 return (int)TUIIO(sizeof h,h,TSA_IO_JOB_EMIT);
}
static unsigned int seed_bytes(void)
{
 unsigned int n,i,at;char line[80];
 memset(original_info,0,sizeof original_info);
 if(TEFFILE(TEF_LOAD,ORIGINAL,original,sizeof original,original_info))return 0U;
 n=TEDWORD(original_info+16U);
 if(TEDWORD(original_info+20U)!=0x90U||TEDWORD(original_info+24U)!=80U||
    TEDWORD(original_info+28U)!=3120U||TEDWORD(original_info+32U)!=13U||n!=13U*82U)return 0U;
 memcpy(seed,original,n);at=n;
 for(i=1U;i<=22U;++i){
  sprintf(line,"/* editor page guard %02u */",i);seed[at++]=0U;seed[at++]=80U;
  memset(seed+at,0x40U,80U);if(TEDCENC(line,(unsigned int)strlen(line),seed+at))return 0U;at+=80U;
 }
 return at;
}
static int source_unchanged(void)
{
 unsigned char check[TEF_BYTES];memset(check,0,sizeof check);
 return TEFFILE(TEF_LOAD,ORIGINAL,other,sizeof other,check)||
  TEDWORD(check+16U)!=TEDWORD(original_info+16U)||memcmp(other,original,TEDWORD(check+16U))||
  memcmp(check+104U,original_info+104U,32U);
}
static int command(TEDSTATE *s,const char *text)
{char b[160];if(strlen(text)>=sizeof b)return 8;strcpy(b,text);return TEDCMD(s,b);}
static int saved_probe(unsigned int n)
{
 TEDSTATE s;int result=0;char change[100];unsigned int i,k,at=0U;
 if(TEDINIT(&s,TEFFILE))return 61;
 if(TEDOPEN(&s,SAVED))result=62;
 else if(command(&s,"FIND PD033 EDITOR LIBRARY")||s.text->current!=12U)result=63;
 else{
  sprintf(change,"CHANGE %u /PD033 EDITOR LIBRARY/PD033 STAGE4 LIBRARY/",s.text->current);
  if(command(&s,change)||!TEDDIRTY(&s))result=63;
  /* Do not publish the inverse change. Compare the full native record set. */
  for(i=0U;!result&&i<s.text->count;++i){
   if(TEDEBC(s.text->line[i].text,s.text->line[i].length,other+at+2U,256U,&k)||k!=80U)result=64;
   else{other[at]=0U;other[at+1U]=80U;at+=82U;}
  }
  if(!result&&(at!=n||memcmp(seed,other,n)))result=65;
 }
 TEDFREE(&s);return result;
}
static int run(const char *mode)
{
 unsigned int n,rc;TEDSTATE s;FILE *held=0;int result=0;
 n=seed_bytes();if(!n)return 41;
 memset(info,0,sizeof info);rc=TEFFILE(TEF_LOAD,SEED,other,sizeof other,info);if(rc)return 42;
 if(!strcmp(mode,"SEED")){
  if(TEDWORD(info+36U)&TEF_EXISTS)return 43;
  if(TEFFILE(TEF_NEW,SEED,seed,n,info))return 44;
 }else if(TEDWORD(info+16U)!=n||memcmp(other,seed,n))return 45;
 if(!strcmp(mode,"VERIFY")){result=saved_probe(n);if(result)return result;}
 else if(strcmp(mode,"SEED")){
  if(strcmp(mode,"CAS")&&strcmp(mode,"BUSY"))return 46;
  if(TEDINIT(&s,TEFFILE))return 47;
  if(TEDOPEN(&s,SEED)||command(&s,"CHANGE 35 /guard 22/guard XX/")||!TEDDIRTY(&s)){result=48;goto done;}
  snapshot=*s.text;
  if(!strcmp(mode,"BUSY")){
   held=fopen(SEED,"rb");if(!held){result=49;goto done;}
   rc=(unsigned int)TEDSAVE(&s,0);
   if(rc!=16U||!TEDDIRTY(&s)||!EDCEQUAL(&snapshot,s.text))result=50;
   if(fclose(held)&&!result)result=51;held=0;
  }else{
   memcpy(other,seed,n);other[13U*82U+2U]=0x40U;
   memcpy(fresh,info,sizeof fresh);
   if(TEFFILE(TEF_SAVE,SEED,other,n,fresh)){result=52;goto done;}
   if(TEDSAVE(&s,0)!=16||!TEDDIRTY(&s)||!EDCEQUAL(&snapshot,s.text)){result=53;goto done;}
   /* An identical-content retry with the old token reconciles without a write. */
   if(TEFFILE(TEF_SAVE,SEED,other,n,info)){result=54;goto done;}
   /* Attributes are independently checked even for identical content. */
   memcpy(fresh,info,sizeof fresh);put(fresh+28U,6240U);
   if(TEFFILE(TEF_SAVE,SEED,other,n,fresh)!=16U){result=55;goto done;}
   if(TEFFILE(TEF_SAVE,SEED,seed,n,info)){result=56;goto done;}
  }
 done:
  if(held)fclose(held);TEDFREE(&s);if(result)return result;
 }
 memset(fresh,0,sizeof fresh);
 if(TEFFILE(TEF_LOAD,SEED,other,sizeof other,fresh)||TEDWORD(fresh+16U)!=n||memcmp(seed,other,n))return 57;
 return source_unchanged()?58:0;
}
int main(int argc,char **argv)
{
 unsigned char h[TSA_DEVICE_BYTES];unsigned int rc;int result;char line[100];
 if(argc!=2)return 8;
 if(!strcmp(argv[1],"OWNER")){
  memset(h,0,sizeof h);put(h,1U);put(h+4U,sizeof h);
  rc=TUIIO(sizeof h,h,TSA_IO_DEVICE_OPEN);
  if(!rc)TUIIO(sizeof h,h,TSA_IO_DEVICE_CLOSE);
  result=rc==20U?0:71;
  if(emit(result?"PD033 EDIT BATCH OWNER FAIL":"PD033 EDIT BATCH OWNER REFUSED 20",1))return 72;
  return result;
 }
 if(TUIOPEN((const unsigned char *)"Editor native probes",20U))return 73;
 result=run(argv[1]);sprintf(line,"PD033 EDIT PROBE %s %s RC=%d",argv[1],result?"FAIL":"PASS",result);
 if(emit(line,0)&&!result)result=74;if(TUICLOSE()&&!result)result=75;return result;
}
