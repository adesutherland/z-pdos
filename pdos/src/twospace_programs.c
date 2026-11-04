/* SPDX-License-Identifier: MIT; original Programs page, no second executor. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "twospace_programs.h"
#include "twospace_abi.h"
#include "twospace_workspace.h"
static int label(unsigned char *p,unsigned int width,const char *text)
{unsigned int n=(unsigned int)strlen(text);if(n>width)n=width;return TJCENC(text,n,p);}
/* FB132 padding is not another display row. Keep one row for an actual blank
 * record; the retained record and its bytes remain unchanged. */
static unsigned int display_width(const unsigned char *p,unsigned int n)
{while(n>1U&&p[n-1U]==0x40U)--n;return n;}
unsigned int TDVPAGES(const TDVSTATE *s,const T27GEOMETRY *g)
{
 unsigned int at=0U,lines=0U,n,width,rows;
 if(!g||g->columns<=2U||g->rows<=5U)return 1U;width=g->columns-2U;rows=g->rows-5U;
 while(at<s->output_bytes){if(s->output_bytes-at<24U)return 1U;n=TJBWORD(s->jobs->output+at+8U);
  if(!n||n>132U||n>s->output_bytes-at-24U)return 1U;lines+=(display_width(s->jobs->output+at+24U,n)+width-1U)/width;at+=24U+n;}
 return lines?(lines+rows-1U)/rows:1U;
}
int TDVPAINT(const TDVSTATE *s,const T27GEOMETRY *g,unsigned char *out,unsigned int cap,unsigned int *used,unsigned int view,unsigned int page)
{
 unsigned char row[132],context[8];char b[160],status[100];const char *panel,*keys;
 unsigned int r,c,at=0U,i,n=0U,offset=0U,skip,line=0U,part=0U,take,visible,context_bytes;
 static const char *help[]={"Programs / one saved-source workspace",
  "Member and Arguments are retained when returning from Source or Output.",
  "PF2 Edit: open the selected source member. PF2 saves inside Source.",
  "PF3 Back: Source refuses unsaved changes; CANCEL explicitly discards.",
  "PF4 Build: compile, assemble, link, then safely publish the executable.",
  "PF5 Run: requires the current saved source and matching built artifact.",
  "PF6 Output: explicitly review the selected native result before replacement.",
  "D Details: inspect native steps without authorising output replacement.",
  "PF7/PF8 page Output. PF1 or PF3 returns from this Help panel.",
  "Build and Run execute synchronously. Busy panels accept no panel commands.",
  "Two programs; FB80 source; 64KiB logical and packed publication limits.",
  "Advanced JCL and job recovery remain available through console BATCH."};
 if(!s||!g||g->rows<24U||g->rows>43U||g->columns<80U||g->columns>132U||g->cells!=g->rows*g->columns||!out||!used||cap<g->cells+g->rows*8U+16U||view>TDV_VIEW_RUN)return 8;
 skip=page*(g->rows-5U);context_bytes=(unsigned int)strlen(s->member);if(context_bytes>8U||TJCENC(s->member,context_bytes,context))return 8;
 panel=view==TDV_VIEW_OUTPUT?TWP_OUTPUT:view==TDV_VIEW_DETAILS?TWP_DETAILS:view==TDV_VIEW_HELP?TWP_HELP:view==TDV_VIEW_BUILD?TWP_BUILD:view==TDV_VIEW_RUN?TWP_RUN:TWP_PROGRAMS;
 keys=view==TDV_VIEW_PROGRAMS?"PF1 Help PF2 Edit PF3 Back PF4 Build PF5 Run PF6 Output":view==TDV_VIEW_OUTPUT?"PF1 Help PF3 Back PF7 Up PF8 Down":view==TDV_VIEW_HELP?"PF1 Close help PF3 Back":"PF1 Help PF3 Back PF6 Output";
 if(view==TDV_VIEW_OUTPUT)sprintf(status,"Job %u / page %u of %u",s->selected,page+1U,TDVPAGES(s,g));
 else sprintf(status,"Selected job %u",s->selected);
 out[at++]=0xc3U;out[at++]=0x11U;if(T27ADDR(g,0U,out+at))return 8;at+=2U;
 for(r=0U;r<g->rows;++r){
  if(TWPFRAME(g,r,panel,context,context_bytes,status,s->notice,keys,row))return 8;
  if(r>=2U&&r<g->rows-3U){
   if(view==TDV_VIEW_OUTPUT){
    while(offset<s->output_bytes){const unsigned char *p=s->jobs->output+offset;
     if(s->output_bytes-offset<24U)return 12;n=TJBWORD(p+8U);if(!n||n>132U||offset+24U+n>s->output_bytes)return 12;
     visible=display_width(p+24U,n);take=visible-part;if(take>g->columns-2U)take=g->columns-2U;
     if(line++>=skip)for(i=0U;i<take;++i)row[1U+i]=p[24U+part+i]<0x40U||p[24U+part+i]==0xffU?0x4bU:p[24U+part+i];
     part+=take;if(part==visible){offset+=24U+n;part=0U;}if(line>skip)break;
    }
    if(r==2U&&offset>=s->output_bytes&&line<=skip)label(row+1U,g->columns-2U,"End of output");
   }else if(view==TDV_VIEW_HELP){if(r-2U<sizeof help/sizeof help[0])label(row+1U,g->columns-2U,help[r-2U]);
    else if(r==14U){sprintf(b,"Source library: %s",s->source);label(row+1U,g->columns-2U,b);}
    else if(r==15U){sprintf(b,"Executable library: %s",s->output);label(row+1U,g->columns-2U,b);}
   }
   else if(view!=TDV_VIEW_PROGRAMS){
    if(s->detail_valid){if(TWPJOBROW(g,r,s->detail,row))return 8;}
    else if(r==2U)label(row+1U,g->columns-2U,"No checked native step details available.");
    if(r==14U)label(row+1U,g->columns-2U,"PF6 explicitly reviews retained output; PF3 returns to Programs.");
   }else{
    if(r==3U){label(row+1U,10U,"Member:");memset(row+12U,0,8U);label(row+12U,8U,s->member);}
    else if(r==5U){label(row+1U,10U,"Arguments:");memset(row+12U,0,60U);label(row+12U,60U,s->arguments);}
    else if(r==7U)label(row+1U,g->columns-2U,"E Edit   B Build   R Run   O Output   D Details   Q Back");
    else if(r==9U)label(row+1U,g->columns-2U,"Type a member and command; arguments are plain application arguments.");
    else if(r==11U)label(row+1U,g->columns-2U,"Registered programs:");
    else if(r>=12U&&r<12U+s->count){i=r-12U;memcpy(b,s->catalog+TDV_HEADER+2U+i*TDV_ENTRY,8U);if(!TJCDEC((unsigned char *)b,8U,b)){b[8]=0;label(row+1U,8U,b);}}
    else if(r==g->rows-4U)label(row+1U,g->columns-2U,"Two programs; FB80 source; 64KiB executable publication limit.");
   }
  }
  if(r==g->rows-3U){if(!view)memset(row+7U,0,g->columns-8U);else memset(row+1U,0x40,g->columns-2U);}
  for(c=0U;c<g->columns;++c){
   if(!view&&r==3U&&c==12U)out[at++]=0x13U;
   if(!c||c==g->columns-1U||(!view&&((r==3U&&(c==11U||c==20U))||(r==5U&&(c==11U||c==72U))||(r==g->rows-3U&&c==6U)))){
    out[at++]=0x1dU;out[at++]=(unsigned char)(!view&&((c==11U&&(r==3U||r==5U))||(r==g->rows-3U&&c==6U))?0xc0U:0xf0U);
   }else out[at++]=row[c];
  }
 }*used=at;return 0;
}
int TDVINPUT(TDVSTATE *s,const T27GEOMETRY *g,const unsigned char *raw,unsigned int n,const unsigned char *buffer,unsigned int buffered,char command[199],unsigned int *aid)
{
 T27INPUTEVENT e,b;T27INPUTFIELD fields[16];unsigned char native[132];char member[9],args[61],action[125],*verb;unsigned int i,k,bytes,seen=0U,flag;char *target;
 if(T27INPUT(g,raw,n,&e,fields,16U)||e.kind!=T27_EVENT_KEY)return 8;*aid=e.aid;strcpy(member,s->member);strcpy(args,s->arguments);action[0]=0;
 for(i=0U;i<e.fields;++i){if(fields[i].address==3U*g->columns+12U){target=member;k=8U;flag=1U;}
  else if(fields[i].address==5U*g->columns+12U){target=args;k=60U;flag=2U;}
  else if(fields[i].address==(g->rows-3U)*g->columns+7U){target=action;k=g->columns-8U;flag=4U;}else return 8;
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
 if(!*verb)return *action?20:0;
 sprintf(command,"%s %s%s%s",verb,member,!strcmp(verb,"RUN")&&*args?" ":"",!strcmp(verb,"RUN")?args:"");return 0;
}
#ifdef TDV_PCOMM
#include "twospace_ui.h"
static void pointer(unsigned char *p,const void *v){TJBPUT(p,0U);TJBPUT(p+4U,(unsigned int)(unsigned long)v);}
static void request(unsigned char *r,unsigned int h,unsigned int g)
{memset(r,0,64U);TJBPUT(r,1U);TJBPUT(r+4U,64U);TJBPUT(r+8U,h);TJBPUT(r+12U,g);}
static unsigned int workspace(TDVSTATE *s,unsigned int task)
{
 unsigned char native[16];const char *member=*s->member?s->member:"Selected result";unsigned int n;
 n=(unsigned int)strlen(member);if(n>sizeof native||TJCENC(member,n,native))return 8U;
 return TUIWORK(native,n,task);
}
int TDVFORM(TDVSTATE *s)
{
 unsigned char r[64],caps[128],*raw,*stream,*buffer;T27GEOMETRY geo;
 unsigned int h=0U,g=0U,n,buffered,aid,rc=0U,closed,view=0U,page=0U,prior=0U,prior_page=0U,task;
 char command[199];int action;
 s->form=1U;
 raw=(unsigned char *)malloc(40000U);stream=(unsigned char *)malloc(40000U);buffer=(unsigned char *)malloc(40000U);
 if(!raw||!stream||!buffer){rc=4U;goto done;}
 for(;;){
  if(!h){request(r,0U,0U);rc=TUIIO(64U,r,TSA_IO_DEVICE_OPEN);if(rc)break;h=TJBWORD(r+8U);g=TJBWORD(r+12U);
   for(;;){request(r,h,g);TJBPUT(r+28U,40000U);pointer(r+32U,raw);rc=TUIIO(64U,r,TSA_IO_DEVICE_EVENT);if(rc==16U){rc=0U;break;}if(rc)break;TJBPUT(r+16U,0U);rc=TUIIO(64U,r,TSA_IO_DEVICE_ACK);if(rc)break;}if(rc)break;
  }
  request(r,h,g);TJBPUT(r+28U,sizeof caps);pointer(r+32U,caps);rc=TUIIO(64U,r,TSA_IO_DEVICE_CAPS);if(rc)break;g=TJBWORD(r+12U);
  if(!(TJBWORD(caps+48U)&TSA_FEATURE_READ_BUFFER)||T27GEOM(&geo,TJBWORD(caps+16U),TJBWORD(caps+20U),TJBWORD(caps+40U))){rc=20U;break;}
  rc=(unsigned int)TDVPAINT(s,&geo,stream,40000U,&n,view,page);if(rc)break;
  request(r,h,g);TJBPUT(r+16U,1U);TJBPUT(r+20U,0xf1U);TJBPUT(r+28U,n);pointer(r+32U,stream);rc=TUIIO(64U,r,TSA_IO_DEVICE_WRITE);if(rc)break;
  if(view==TDV_VIEW_OUTPUT)s->reviewed=s->selected;
  request(r,h,g);TJBPUT(r+20U,0xf6U);TJBPUT(r+28U,40000U);pointer(r+32U,raw);rc=TUIIO(64U,r,TSA_IO_DEVICE_READ);if(rc)break;n=TJBWORD(r+48U);TJBPUT(r+16U,0U);rc=TUIIO(64U,r,TSA_IO_DEVICE_ACK);if(rc)break;
  if(view){T27INPUTEVENT event;if(T27EVENT(&geo,raw,n,&event)){rc=8U;break;}
   if(view==TDV_VIEW_HELP){if(event.aid==0xf1U||event.aid==0xf3U){view=prior;page=prior_page;}continue;}
   if(event.aid==0xf1U){prior=view;prior_page=page;view=TDV_VIEW_HELP;continue;}
   if(event.aid==0xf3U){view=TDV_VIEW_PROGRAMS;continue;}
   if(view==TDV_VIEW_OUTPUT){if(event.aid==0xf7U&&page)--page;else if(event.aid==0xf8U&&page+1U<TDVPAGES(s,&geo))++page;continue;}
   if(event.aid!=0xf6U)continue;strcpy(command,"OUTPUT");
  }else{
   buffered=0U;
   if(n>3U){request(r,h,g);TJBPUT(r+20U,0xf2U);TJBPUT(r+28U,40000U);pointer(r+32U,buffer);rc=TUIIO(64U,r,TSA_IO_DEVICE_READ);if(rc)break;buffered=TJBWORD(r+48U);TJBPUT(r+16U,0U);rc=TUIIO(64U,r,TSA_IO_DEVICE_ACK);if(rc)break;}
   action=TDVINPUT(s,&geo,raw,n,buffer,buffered,command,&aid);if(action==1)break;
   if(action){strcpy(s->notice,"Action/fields refused; use E/B/R/O/D or PF1 Help");continue;}if(!*command)continue;
  }
  if(!strcmp(command,"HELP")){prior=view;prior_page=page;view=TDV_VIEW_HELP;continue;}
  if(!strcmp(command,"DETAILS")){(void)TDVDETAIL(s);view=TDV_VIEW_DETAILS;continue;}
  task=!strncmp(command,"EDIT ",5U)?TSA_WORKSPACE_SOURCE:!strncmp(command,"BUILD ",6U)?TSA_WORKSPACE_BUILD:!strncmp(command,"RUN ",4U)?TSA_WORKSPACE_RUN:TSA_WORKSPACE_OUTPUT;
  if(task!=TSA_WORKSPACE_OUTPUT&&!*s->member){strcpy(s->notice,"Enter a member before Edit, Build or Run");continue;}
  rc=workspace(s,task);if(rc)break;
  /* Child EDIT owns a separate raw session; this invocation retains no handle. */
  request(r,h,g);rc=TUIIO(64U,r,TSA_IO_DEVICE_CLOSE);h=0U;if(rc)break;
  action=TDVCMD(s,command);
  if(!strcmp(command,"OUTPUT")&&!action){view=TDV_VIEW_OUTPUT;page=0U;s->reviewed=0U;}
  else if((task==TSA_WORKSPACE_BUILD||task==TSA_WORKSPACE_RUN)&&s->attempt_submitted){
   /* Detail failure must not replace the final publication/freshness notice. */
   char notice[160];strcpy(notice,s->notice);(void)TDVDETAIL(s);strcpy(s->notice,notice);
   view=task==TSA_WORKSPACE_BUILD?TDV_VIEW_BUILD:TDV_VIEW_RUN;
  }else view=TDV_VIEW_PROGRAMS;
 }
 done:
 closed=TUIWORK(0,0U,0U);if(!rc)rc=closed;s->form=0U;
 if(h){request(r,h,g);closed=TUIIO(64U,r,TSA_IO_DEVICE_CLOSE);if(!rc)rc=closed;}
 free(raw);free(stream);free(buffer);return (int)rc;
}
#endif
