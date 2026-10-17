/* SPDX-License-Identifier: MIT; original Programs page, no second executor. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "twospace_programs.h"
#include "twospace_abi.h"
static int label(unsigned char *p,unsigned int width,const char *text)
{unsigned int n=(unsigned int)strlen(text);if(n>width)n=width;return TJCENC(text,n,p);}
unsigned int TDVPAGES(const TDVSTATE *s,const T27GEOMETRY *g)
{
 unsigned int at=0U,lines=0U,n,width,rows;
 if(!g||g->columns<=2U||g->rows<=4U)return 1U;width=g->columns-2U;rows=g->rows-4U;
 while(at<s->output_bytes){if(s->output_bytes-at<24U)return 1U;n=TJBWORD(s->jobs->output+at+8U);
  if(!n||n>132U||n>s->output_bytes-at-24U)return 1U;lines+=(n+width-1U)/width;at+=24U+n;}
 return lines?(lines+rows-1U)/rows:1U;
}
int TDVPAINT(const TDVSTATE *s,const T27GEOMETRY *g,unsigned char *out,unsigned int cap,unsigned int *used,unsigned int view,unsigned int page)
{
 unsigned char row[132];char b[160];unsigned int r,c,at=0U,i,n=0U,offset=0U,skip=page*(g->rows-4U),line=0U,part=0U,take;
 if(!g||g->rows<24U||g->columns<80U||g->columns>132U||cap<g->cells+g->rows*8U+16U)return 8;
 out[at++]=0xc3U;out[at++]=0x11U;if(T27ADDR(g,0U,out+at))return 8;at+=2U;
 for(r=0U;r<g->rows;++r){memset(row,0x40,g->columns);
  if(!r){if(view){sprintf(b,"cREXX / Output page %u of %u / PF3 Programs / PF7 Up / PF8 Down",page+1U,TDVPAGES(s,g));label(row+1U,g->columns-2U,b);}else label(row+1U,g->columns-2U,"cREXX / Programs");}
  else if(r==1U)label(row+1U,g->columns-2U,s->notice);
  else if(view&&r<g->rows-2U){
   while(offset<s->output_bytes){
    const unsigned char *p=s->jobs->output+offset;
    if(s->output_bytes-offset<24U)return 12;
    n=TJBWORD(p+8U);if(!n||n>132U||offset+24U+n>s->output_bytes)return 12;
    take=n-part;if(take>g->columns-2U)take=g->columns-2U;
    if(line++>=skip)for(i=0U;i<take;++i)row[1U+i]=p[24U+part+i]<0x40U?0x4bU:p[24U+part+i];
    part+=take;if(part==n){offset+=24U+n;part=0U;}if(line>skip)break;
   }
   if(r==2U&&offset>=s->output_bytes&&line<=skip)label(row+1U,g->columns-2U,"End of output");
  }else if(!view){
   if(r==3U){label(row+1U,10U,"Member:");memset(row+12U,0,8U);label(row+12U,8U,s->member);}
   else if(r==5U){label(row+1U,10U,"Arguments:");memset(row+12U,0,60U);label(row+12U,60U,s->arguments);}
   else if(r==7U){label(row+1U,10U,"Action:");memset(row+12U,0,30U);}
   else if(r==9U)label(row+1U,g->columns-2U,"E Edit   B Build   R Run   O Output   D Details   Q Console");
   else if(r==11U)label(row+1U,g->columns-2U,"Type a member and action; arguments are plain application arguments.");
   else if(r>=13U&&r<13U+s->count){i=r-13U;memcpy(b,s->catalog+TDV_HEADER+2U+i*TDV_ENTRY,8U);if(!TJCDEC((unsigned char *)b,8U,b)){b[8]=0;label(row+1U,8U,b);}}
   else if(r==g->rows-3U)label(row+1U,g->columns-2U,"Capacity: two programs; BUILD saves; RUN requires current saved source.");
  }
  if(r==g->rows-1U)label(row+1U,g->columns-2U,view?"PF3 Return / PF7 Up / PF8 Down":"PF1 Help / PF2 Edit / PF3 Console / PF4 Build / PF5 Run / PF6 Output");
  for(c=0U;c<g->columns;++c){
   if(!view&&r==3U&&c==12U)out[at++]=0x13U;
   if(!c||c==g->columns-1U||(!view&&((r==3U&&(c==11U||c==20U))||(r==5U&&(c==11U||c==72U))||(r==7U&&(c==11U||c==42U))))){
    out[at++]=0x1dU;out[at++]=(unsigned char)(!view&&c==11U&&(r==3U||r==5U||r==7U)?0xc0U:0xf0U);
   }else out[at++]=row[c];
  }
 }*used=at;return 0;
}
int TDVINPUT(TDVSTATE *s,const T27GEOMETRY *g,const unsigned char *raw,unsigned int n,const unsigned char *buffer,unsigned int buffered,char command[199],unsigned int *aid)
{
 T27INPUTEVENT e,b;T27INPUTFIELD fields[16];unsigned char native[132];char member[9],args[61],action[31],*verb;unsigned int i,k,bytes,seen=0U,flag;char *target;
 if(T27INPUT(g,raw,n,&e,fields,16U)||e.kind!=T27_EVENT_KEY)return 8;*aid=e.aid;strcpy(member,s->member);strcpy(args,s->arguments);action[0]=0;
 for(i=0U;i<e.fields;++i){if(fields[i].address==3U*g->columns+12U){target=member;k=8U;flag=1U;}
  else if(fields[i].address==5U*g->columns+12U){target=args;k=60U;flag=2U;}
  else if(fields[i].address==7U*g->columns+12U){target=action;k=30U;flag=4U;}else return 8;
  if(seen&flag)return 8;seen|=flag;
  if(T27BUFFER(g,buffer,buffered,fields[i].address,native,sizeof native,&bytes,&b)||bytes>k)return 8;
  while(bytes&&(native[bytes-1U]==0U||native[bytes-1U]==0x40U))--bytes;
  for(k=0U;k<bytes;++k)if(!native[k])native[k]=0x40U;
  if(TJCDEC(native,bytes,target))return 8;target[bytes]=0;
 }
 for(i=0U;member[i];++i)member[i]=(char)toupper((unsigned char)member[i]);
 strcpy(s->member,member);strcpy(s->arguments,args);command[0]=0;
 if(e.aid==0xf3U)return 1;
 if(e.aid==0xf1U)verb="HELP";else if(e.aid==0xf2U)verb="EDIT";else if(e.aid==0xf4U)verb="BUILD";else if(e.aid==0xf5U)verb="RUN";else if(e.aid==0xf6U)verb="OUTPUT";
 else if(e.aid==0x7dU){for(i=0U;action[i];++i)action[i]=(char)toupper((unsigned char)action[i]);
  verb=!strcmp(action,"E")||!strcmp(action,"EDIT")?"EDIT":!strcmp(action,"B")||!strcmp(action,"BUILD")?"BUILD":!strcmp(action,"R")||!strcmp(action,"RUN")?"RUN":!strcmp(action,"O")||!strcmp(action,"OUTPUT")?"OUTPUT":!strcmp(action,"D")||!strcmp(action,"DETAILS")?"DETAILS":!strcmp(action,"H")||!strcmp(action,"HELP")?"HELP":!strcmp(action,"Q")?"QUIT":"";
 }else return 20;
 if(!strcmp(verb,"QUIT"))return 1;
 if(!strcmp(verb,"OUTPUT")||!strcmp(verb,"DETAILS")||!strcmp(verb,"HELP")){strcpy(command,verb);return 0;}
 if(!*verb)return 0;
 sprintf(command,"%s %s%s%s",verb,member,!strcmp(verb,"RUN")&&*args?" ":"",!strcmp(verb,"RUN")?args:"");return 0;
}
#ifdef TDV_PCOMM
#include "twospace_ui.h"
static void pointer(unsigned char *p,const void *v){TJBPUT(p,0U);TJBPUT(p+4U,(unsigned int)(unsigned long)v);}
static void request(unsigned char *r,unsigned int h,unsigned int g)
{memset(r,0,64U);TJBPUT(r,1U);TJBPUT(r+4U,64U);TJBPUT(r+8U,h);TJBPUT(r+12U,g);}
int TDVFORM(TDVSTATE *s)
{
 unsigned char r[64],caps[128],*raw,*stream,*buffer;T27GEOMETRY geo;unsigned int h=0U,g=0U,n,buffered,aid,rc=0U,closed,view=0U,page=0U;char command[199];int action;
 raw=(unsigned char *)malloc(40000U);stream=(unsigned char *)malloc(40000U);buffer=(unsigned char *)malloc(40000U);
 if(!raw||!stream||!buffer){rc=4U;goto done;}
 for(;;){
  if(!h){request(r,0U,0U);rc=TUIIO(64U,r,TSA_IO_DEVICE_OPEN);if(rc)break;h=TJBWORD(r+8U);g=TJBWORD(r+12U);
   for(;;){request(r,h,g);TJBPUT(r+28U,40000U);pointer(r+32U,raw);rc=TUIIO(64U,r,TSA_IO_DEVICE_EVENT);if(rc==16U){rc=0U;break;}if(rc)break;TJBPUT(r+16U,0U);rc=TUIIO(64U,r,TSA_IO_DEVICE_ACK);if(rc)break;}if(rc)break;
  }
  request(r,h,g);TJBPUT(r+28U,sizeof caps);pointer(r+32U,caps);rc=TUIIO(64U,r,TSA_IO_DEVICE_CAPS);if(rc)break;g=TJBWORD(r+12U);
  if(!(TJBWORD(caps+48U)&TSA_FEATURE_READ_BUFFER)||T27GEOM(&geo,TJBWORD(caps+16U),TJBWORD(caps+20U),TJBWORD(caps+40U))){rc=20U;break;}
  rc=(unsigned int)TDVPAINT(s,&geo,stream,40000U,&n,view,page);if(rc)break;
  request(r,h,g);TJBPUT(r+16U,1U);TJBPUT(r+20U,0xf1U);TJBPUT(r+28U,n);pointer(r+32U,stream);rc=TUIIO(64U,r,TSA_IO_DEVICE_WRITE);if(rc)break;if(view)s->reviewed=s->selected;
  request(r,h,g);TJBPUT(r+20U,0xf6U);TJBPUT(r+28U,40000U);pointer(r+32U,raw);rc=TUIIO(64U,r,TSA_IO_DEVICE_READ);if(rc)break;n=TJBWORD(r+48U);TJBPUT(r+16U,0U);rc=TUIIO(64U,r,TSA_IO_DEVICE_ACK);if(rc)break;
  if(view){T27INPUTEVENT event;if(T27EVENT(&geo,raw,n,&event)){rc=8U;break;}
   if(event.aid==0xf3U)view=0U;else if(event.aid==0xf7U&&page)--page;else if(event.aid==0xf8U&&page+1U<TDVPAGES(s,&geo))++page;continue;}
  buffered=0U;
  if(n>3U){request(r,h,g);TJBPUT(r+20U,0xf2U);TJBPUT(r+28U,40000U);pointer(r+32U,buffer);rc=TUIIO(64U,r,TSA_IO_DEVICE_READ);if(rc)break;buffered=TJBWORD(r+48U);TJBPUT(r+16U,0U);rc=TUIIO(64U,r,TSA_IO_DEVICE_ACK);if(rc)break;}
  action=TDVINPUT(s,&geo,raw,n,buffer,buffered,command,&aid);if(action==1)break;
  if(action){strcpy(s->notice,"Fields refused; use the displayed actions/PF keys");continue;}if(!*command)continue;
  if(!strcmp(command,"HELP")){strcpy(s->notice,"E opens source; B saves a build; R runs it; O reviews output before replacement");continue;}
  if(!strcmp(command,"DETAILS")){request(r,h,g);rc=TUIIO(64U,r,TSA_IO_DEVICE_CLOSE);h=0U;if(rc)break;
   {char details[40];sprintf(details,"STATUS %u",s->selected);TJFCMD(s->jobs,&s->ops.batch,details);}break;}
  /* A child owns its own raw session; no nested raw ownership is retained. */
  request(r,h,g);rc=TUIIO(64U,r,TSA_IO_DEVICE_CLOSE);h=0U;if(rc)break;
  action=TDVCMD(s,command);if(!strcmp(command,"OUTPUT")&&!action){view=1U;page=0U;s->reviewed=0U;}
 }
 done:if(h){request(r,h,g);closed=TUIIO(64U,r,TSA_IO_DEVICE_CLOSE);if(!rc)rc=closed;}
 free(raw);free(stream);free(buffer);return (int)rc;
}
#endif
