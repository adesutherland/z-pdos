/* SPDX-License-Identifier: MIT; U forms over the common checked batch service.
 * C strings use execution characters; view cells and source cards are IBM1047.
 * No direct disk, scheduler or persistent-layout implementation lives here. */
#include <string.h>
#include <ctype.h>
#include "twospace_batchui.h"
static int load(void *c,const char *name,unsigned char *p,unsigned int n,unsigned int *used)
{TBFSTATE *s=(TBFSTATE *)c;return s->backend.load(s->backend.context,name,p,n,used);}
static unsigned int service(void *c,unsigned int op,unsigned int n,unsigned char *h,void *p)
{TBFSTATE *s=(TBFSTATE *)c;return s->backend.service(s->backend.context,op,n,h,p);}
static int input(void *c,unsigned char *p,unsigned int n,unsigned int *used)
{TBFSTATE *s=(TBFSTATE *)c;return s->backend.input(s->backend.context,p,n,used);}
static void row(TBFSTATE *s,const unsigned char *p,unsigned int n)
{
 unsigned int first=s->page*s->height,i,take;
 if(s->total>=first&&s->total-first<s->height){
  unsigned char *out=s->rows[s->total-first];memset(out,0x40,TBF_WIDTH);
  take=s->column<n?n-s->column:0U;if(take>s->width)take=s->width;
  for(i=0U;i<take;++i){unsigned char v=p[s->column+i];out[i]=v<0x40U||v==0xffU?0x6fU:v;}
  ++s->shown;
 }++s->total;
}
static void line(void *c,const char *p)
{TBFSTATE *s=(TBFSTATE *)c;unsigned char encoded[256];unsigned int n=(unsigned int)strlen(p);
 if(!strncmp(p,"JCL ERROR",9U)||!strncmp(p,"SITE ",5U)){strncpy(s->notice,p,sizeof s->notice-1U);s->notice[sizeof s->notice-1U]=0;}
 if(n>sizeof encoded||TJCENC(p,n,encoded))return;row(s,encoded,n);}
static void record(void *c,const unsigned char *p,unsigned int n)
{row((TBFSTATE *)c,p,n);}
static void upper(char *p)
{while(*p){*p=(char)toupper((unsigned char)*p);++p;}}
static void number(char *p,unsigned int v)
{char digits[10];unsigned int i=0U,at=(unsigned int)strlen(p);do{digits[i++]=(char)('0'+v%10U);v/=10U;}while(v);while(i)p[at++]=digits[--i];p[at]=0;}
static void count(char *p,const char *label,unsigned int v)
{strcat(p,label);number(p,v);}
static void jobtext(char *p,const char *verb,unsigned int id)
{strcpy(p,verb);count(p," ",id);}
static int decimal(const char *p,unsigned int *v)
{unsigned int n=0U,d;if(!*p)return 8;while(*p){if(*p<'0'||*p>'9')return 8;d=(unsigned int)(*p++-'0');if(n>429496729U||(n==429496729U&&d>5U))return 8;n=n*10U+d;}if(!n)return 8;*v=n;return 0;}
static void invalidate(TBFSTATE *s)
{s->ready=0U;s->front->result.valid=0U;s->page=s->column=0U;if(s->screen==TBF_PREVIEW)s->screen=TBF_SUBMIT;}
static int failure(TBFSTATE *s,const char *text)
{strcpy(s->notice,text);s->rc=8U;return 8;}
int TBFINIT(TBFSTATE *s,TJFSTATE *front,const TJFOPS *ops)
{
 if(!s||!front||!ops||!ops->load||!ops->service||!ops->input||!ops->line||!ops->record)return 8;
 memset(s,0,sizeof *s);s->front=front;s->backend=*ops;s->bridge=*ops;
 s->bridge.context=s;s->bridge.load=load;s->bridge.service=service;s->bridge.input=input;
 s->bridge.line=line;s->bridge.record=record;s->initialized=1U;s->screen=TBF_SUBMIT;
 s->hold=1U;s->height=8U;s->width=78U;strcpy(s->proc,"CRXCG");strcpy(s->notice,"Enter fields; PREVIEW validates without submitting");return 0;
}
static int card(TBFSTATE *s,const char *p,unsigned int *bytes)
{unsigned int n=(unsigned int)strlen(p);if(n>71U||*bytes>sizeof s->front->deck-80U)return 8;
 memset(s->front->deck+*bytes,0x40,80U);if(TJCENC(p,n,s->front->deck+*bytes))return 8;*bytes+=80U;return 0;}
static int option(TBFSTATE *s,const char *key,const char *value,unsigned int *n,unsigned int more)
{char text[180];if(strlen(key)+strlen(value)+4U>=sizeof text)return 8;
 strcpy(text,"// ");strcat(text,key);strcat(text,value);if(more)strcat(text,",");return card(s,text,n);}
static int before_source(const char *p)
{return !strncmp(p,"//COMP.SYSIN ",sizeof "//COMP.SYSIN "-1U)||
 !strncmp(p,"//COMP.SYSPRINT ",sizeof "//COMP.SYSPRINT "-1U)||
 !strncmp(p,"//COMP.SYSTERM ",sizeof "//COMP.SYSTERM "-1U);}
static int generated(TBFSTATE *s,unsigned int *bytes)
{
 char text[180];unsigned int i;TJFSTATE *f=s->front;*bytes=0U;
 if(strcmp(s->proc,"CRXC")&&strcmp(s->proc,"CRXCG")&&strcmp(s->proc,"CRXCL")&&strcmp(s->proc,"CRXLGO"))return failure(s,"Choose CRXC, CRXCG, CRXCL or CRXLGO");
 if(!s->source[0])return failure(s,"SRC dataset/member is required");
 if((!strcmp(s->proc,"CRXC")||!strcmp(s->proc,"CRXCL"))&&!s->out[0])return failure(s,"OUT dataset/member is required for CRXC/CRXCL");
 if(card(s,"//BATCH JOB CLASS=A,MSGCLASS=A,REGION=0M",bytes))return 8;
 strcpy(text,"//LIBS JCLLIB ORDER=");strcat(text,f->proclib);if(card(s,text,bytes))return 8;
 strcpy(text,"//JOBLIB DD DSN=");strcat(text,f->loadlib);strcat(text,",DISP=SHR");if(card(s,text,bytes))return 8;
 strcpy(text,"//BUILD EXEC ");strcat(text,s->proc);strcat(text,",");if(card(s,text,bytes))return 8;
 if(option(s,"LIB=",f->prefix,bytes,1U)||option(s,"VOL=",f->volume,bytes,s->out[0]||s->parameters[0]))return 8;
 if(s->out[0]&&option(s,"OUT=",s->out,bytes,s->parameters[0]!=0))return 8;
 /* A free parameter line is intentionally bounded to one physical JCL card.
    The common converter owns quoting, symbols and unsupported operands. */
 if(s->parameters[0]&&option(s,"",s->parameters,bytes,0U))return 8;
 /* Common PROCs declare these three DDs before SRC. Keep caller order on
    each side of the generated SRC override; the converter checks that order. */
 for(i=0U;i<s->overrides;++i)if(before_source(s->override[i])&&card(s,s->override[i],bytes))return 8;
 strcpy(text,"//COMP.SRC DD DSN=");strcat(text,s->source);strcat(text,",DISP=SHR");if(card(s,text,bytes))return 8;
 for(i=0U;i<s->overrides;++i)if(!before_source(s->override[i])&&card(s,s->override[i],bytes))return 8;
 return card(s,"//",bytes);
}
static int preview(TBFSTATE *s)
{
 unsigned int n;int rc;
 invalidate(s);if(s->attempted)return failure(s,"Attempt already consumed; use NEW for deliberate resubmission");
 if(s->ticket==0xffffffffU)return failure(s,"Preview identity exhausted; return to the console");
 rc=TJFCONFIG(s->front,&s->bridge);if(rc){if(!s->notice[0])strcpy(s->notice,"Site admission failed; no job submitted");s->rc=(unsigned int)rc;return rc;}
 if(s->deck[0])rc=TJFPLAN(s->front,s->deck,0,0U);
 else{rc=generated(s,&n);if(!rc)rc=TJFPLAN(s->front,0,s->front->deck,n);}
 if(rc){s->rc=(unsigned int)rc;if(!s->notice[0]){strcpy(s->notice,"JCL/source admission status ");number(s->notice,(unsigned int)rc);strcat(s->notice,"; no job submitted");}return rc;}
 ++s->ticket;s->ready=1U;s->screen=TBF_PREVIEW;
 strcpy(s->notice,"PREVIEW ");number(s->notice,s->ticket);count(s->notice,": ",s->front->result.steps);count(s->notice," steps, ",s->front->result.dds);strcat(s->notice," DDs; SUBMIT publishes once");return 0;
}
static const char *field_names[]={"DECK","PROC","SRC","OUT","PARAMS","STEP","DD"};
static char *field_value(TBFSTATE *s,unsigned int i,unsigned int *capacity)
{
 switch(i){case 0:*capacity=sizeof s->deck;return s->deck;case 1:*capacity=sizeof s->proc;return s->proc;
 case 2:*capacity=sizeof s->source;return s->source;case 3:*capacity=sizeof s->out;return s->out;
 case 4:*capacity=sizeof s->parameters;return s->parameters;case 5:*capacity=sizeof s->step;return s->step;
 default:*capacity=sizeof s->dd;return s->dd;}
}
static int edit(TBFSTATE *s,unsigned int index,const char *value)
{
 unsigned int cap;char *p=field_value(s,index,&cap);
 if(strlen(value)>=cap)return failure(s,"Field is too long; value and preview were preserved");
 if(index==4U&&strlen(value)>67U)return failure(s,"PARAMS is limited to 67 characters per card");
 if(index<=4U)invalidate(s);else s->page=0U;
 strcpy(p,value);if(index!=4U)upper(p);
 if(index==1U)s->deck[0]=0;strcpy(s->notice,"Field updated; PREVIEW is required after source edits");return 0;
}
int TBFEVENT(TBFSTATE *s,const char *text,unsigned int aid)
{
 char command[257],action[40],*value,*p;unsigned int i,id;int rc=0;
 if(!s||!s->initialized||!text||strlen(text)>256U)return 8;if(s->exit)return 16;
 if(s->revision==0xffffffffU){invalidate(s);s->exit=1U;return failure(s,"Form revision exhausted; returning to the console");}++s->revision;
 for(i=0U;text[i];++i){unsigned char encoded;if(TJCENC(text+i,1U,&encoded))return failure(s,"Only printable interface text is accepted");}
 s->notice[0]=0;s->rc=0U;
 if(aid==0x6dU){strcpy(s->notice,"Form repainted; submission attempt identity is preserved");return 0;}
 if(aid==0xf3U){if(s->screen==TBF_PREVIEW)s->screen=TBF_SUBMIT;else{invalidate(s);s->exit=1U;}return 0;}
 if(aid==0xf1U){strcpy(s->notice,"Fields: DECK/PROC/SRC/OUT/PARAMS; PREVIEW then SUBMIT; SELECT id; PRINT/RELEASE/CANCEL/PURGE; PF3 returns");return 0;}
 if(aid==0xf7U)text="UP";else if(aid==0xf8U)text="DOWN";
 else if(aid==0xf4U)text="PREVIEW";else if(aid==0x7aU)text="SUBMIT";
 else if(aid==0x7bU)text="JOBS";else if(aid==0x7cU)text="OUTPUT";
 else if(aid==0xf5U||aid==0xf6U){s->editing=(s->editing+(aid==0xf5U?6U:1U))%7U;strcpy(s->notice,"EDIT the selected field, then enter its new value");return 0;}
 else if(aid==0xf2U){strcpy(s->notice,"EDIT");return 0;}
 else if(aid!=0x7dU)return failure(s,"Use Enter, PF2/3/4/5/6/7/8/10/11/12");
 strcpy(command,text);p=command+strlen(command);while(p>command&&p[-1]==' ')*--p=0;
 p=command;while(*p==' ')++p;if(p!=command)memmove(command,p,strlen(p)+1U);
 value=strchr(command,' ');if(value)*value++=0;else value=command+strlen(command);upper(command);
 if(!command[0]){strcpy(s->notice,"Enter refreshes; PREVIEW then explicit SUBMIT publishes");return 0;}
 if(!strcmp(command,"EXIT")||!strcmp(command,"QUIT")){if(*value)return failure(s,"EXIT takes no extra operands");invalidate(s);s->exit=1U;return 0;}
 if(!strcmp(command,"EDIT"))return edit(s,s->editing,value);
 for(i=0U;i<7U;++i)if(!strcmp(command,field_names[i])){s->editing=i;return edit(s,i,value);}
 if(!strcmp(command,"OVERRIDE")){if(s->overrides==8U||strlen(value)>71U||strncmp(value,"//",2U))return failure(s,"OVERRIDE needs one complete // DD card, at most 71 characters; maximum eight");
  invalidate(s);strcpy(s->override[s->overrides++],value);strcpy(s->notice,"DD override added; PREVIEW required");return 0;}
 if(!strcmp(command,"CLEAROVR")){invalidate(s);s->overrides=0U;return 0;}
 if(!strcmp(command,"NEW")){invalidate(s);s->attempted=s->last_id=0U;s->screen=TBF_SUBMIT;strcpy(s->notice,"New deliberate attempt; fields retained; PREVIEW required");return 0;}
 if(!strcmp(command,"HOLD")||!strcmp(command,"RUNHOLD")){
  upper(value);if(strcmp(value,"YES")&&strcmp(value,"NO"))return failure(s,"Use HOLD YES/NO or RUNHOLD YES/NO");
  invalidate(s);if(!strcmp(command,"HOLD"))s->hold=!strcmp(value,"YES");else s->runhold=!strcmp(value,"YES");return 0;}
 if(*value&&strcmp(command,"FORM")&&strcmp(command,"SELECT"))return failure(s,"This form action takes no extra operands");
 if(!strcmp(command,"PREVIEW"))return preview(s);
 if(!strcmp(command,"SUBMIT")){
  if(!s->ready||s->attempted)return failure(s,"No unused preview; use NEW then PREVIEW for another submission");
  s->attempted=s->ticket;s->ready=0U;rc=TJFPOST(s->front,s->runhold,s->hold,&s->last_id);
  if(s->last_id){s->selected=s->last_id;s->screen=TBF_JOBS;s->page=0U;strcpy(s->notice,"Attempt ");number(s->notice,s->attempted);count(s->notice," consumed; JOB ",s->last_id);count(s->notice,"; command status ",(unsigned int)rc);}
  else strcpy(s->notice,"Attempt consumed; acknowledgement unavailable; review JOBS before NEW");s->rc=(unsigned int)rc;return rc;
 }
 if(!strcmp(command,"FORM")){upper(value);if(strcmp(value,"SUBMIT"))return failure(s,"Use FORM SUBMIT, JOBS or OUTPUT");s->screen=TBF_SUBMIT;s->page=s->column=0U;return 0;}
 if(!strcmp(command,"JOBS")||!strcmp(command,"OUTPUT")){s->screen=!strcmp(command,"JOBS")?TBF_JOBS:TBF_OUTPUT;s->page=s->column=0U;return 0;}
 if(!strcmp(command,"UP")){if(s->page)--s->page;return 0;}
 if(!strcmp(command,"DOWN")){if((s->page+1U)*s->height<s->total)++s->page;return 0;}
 if(!strcmp(command,"LEFT")){s->column=0U;return 0;}
 if(!strcmp(command,"RIGHT")){s->column=s->width<TBF_WIDTH?TBF_WIDTH-s->width:0U;return 0;}
 if(!strcmp(command,"SELECT")){if(decimal(value,&id))return failure(s,"SELECT requires an actual nonzero job identity");s->selected=id;s->page=0U;return 0;}
 if(!strcmp(command,"STATUS")||!strcmp(command,"PRINT")||!strcmp(command,"RELEASE")||!strcmp(command,"CANCEL")||!strcmp(command,"PURGE")||!strcmp(command,"QUEUEHOLD")){
  if(*value||!s->selected)return failure(s,"SELECT an actual job identity first; actions take no extra operands");
  p=!strcmp(command,"QUEUEHOLD")?"HOLD":command;
  jobtext(action,p,s->selected);rc=TJFCMD(s->front,&s->bridge,action);
  strcpy(s->notice,"Job ");number(s->notice,s->selected);count(s->notice," action status ",(unsigned int)rc);strcat(s->notice,"; refresh shows current results");
  if(!rc&&!strcmp(p,"PURGE"))s->selected=0U;s->page=0U;s->rc=(unsigned int)rc;return rc;
 }
 if(!strcmp(command,"REFRESH"))return 0;
 return failure(s,"Unknown form action; PF3 returns to the console");
}
int TBFVIEW(TBFSTATE *s,unsigned int height,unsigned int width)
{
 unsigned int i,cap;char text[200],*p;int rc=0;
 if(!s||!s->initialized||!height||height>TBF_ROWS||!width||width>TBF_WIDTH)return 8;
 s->height=height;s->width=width;s->total=s->shown=0U;memset(s->rows,0x40,sizeof s->rows);
 if(s->screen==TBF_SUBMIT){
  for(i=0U;i<7U;++i){p=field_value(s,i,&cap);text[0]=s->editing==i?'>':' ';text[1]=' ';text[2]=0;strcat(text,field_names[i]);strcat(text," = ");strcat(text,p);line(s,text);}
  strcpy(text,s->hold?"HOLD YES; RUNHOLD ":"HOLD NO; RUNHOLD ");strcat(text,s->runhold?"YES":"NO");count(text,"; DD overrides ",s->overrides);line(s,text);
  line(s,"PF2 edit; PF5/6 select field; PF4 preview; PF10 submit");
 }else if(s->screen==TBF_PREVIEW){
  for(i=0U;i<s->front->source_bytes;i+=80U)row(s,s->front->deck+i,80U);
 }else if(s->screen==TBF_JOBS){
  rc=TJFCMD(s->front,&s->bridge,"JOBS");
  if(!rc&&s->selected){jobtext(text,"STATUS",s->selected);rc=TJFCMD(s->front,&s->bridge,text);}
  if(!s->total)line(s,"No retained jobs; FORM SUBMIT prepares a job");
 }else if(s->screen==TBF_OUTPUT){
  if(!s->selected)line(s,"SELECT an actual job identity; optional STEP and DD filters");
  else{jobtext(text,"OUTPUT",s->selected);if(s->step[0]){strcat(text," STEP=");strcat(text,s->step);}if(s->dd[0]){strcat(text," DD=");strcat(text,s->dd);}rc=TJFCMD(s->front,&s->bridge,text);if(!s->total)line(s,"No matching output records");}
 }else return 8;
 /* A live selection can shrink after purge; clamp and render the real page. */
 if(!s->shown&&s->page){s->page=0U;return TBFVIEW(s,height,width);}return rc;
}
#ifndef TBF_PCOMM
int TBFENTRY(TJFSTATE *s,const TJFOPS *ops,const char *command)
{(void)s;(void)ops;(void)command;return 20;}
#else
#include "twospace_ui.h"
#include "twospace_workbench.h"
static TBFSTATE native;
static int native_label(const char *p)
{unsigned char encoded[132];unsigned int n=(unsigned int)strlen(p);if(!n)return 0;if(n>sizeof encoded||TJCENC(p,n,encoded))return 8;return (int)TUILINE(encoded,n);}
int TBFENTRY(TJFSTATE *front,const TJFOPS *ops,const char *command)
{
 unsigned char cap[64],request[64],cells[TBF_ROWS*TBF_WIDTH],encoded[132],key[256];
 unsigned int handle=0U,screen,height=8U,width=78U,generation=0U,n,aid,i,rc=0U,keys_owned=0U;TWBPLAN plan;char text[257],heading[80];
 const char *title;
 if(!front||!ops||!command)return 8;if(native.initialized&&native.front!=front)return 16;
 if(!native.initialized&&TBFINIT(&native,front,ops))return 8;native.exit=0U;
 if(*command){rc=(unsigned int)TBFEVENT(&native,command,0x7dU);if(rc)goto done;}
 rc=TUICAPS(cap);if(rc)goto done;screen=TJBWORD(cap+8U)==TSA_DEVICE_3270;
 if(screen){if(TWBLAYOUT(TJBWORD(cap+16U),TJBWORD(cap+20U),&plan)||plan.body_rows<3U){rc=8U;goto done;}
  height=plan.body_rows-2U;if(height>TBF_ROWS-1U)height=TBF_ROWS-1U;width=plan.body_width;if(width>TBF_WIDTH)width=TBF_WIDTH;}
 rc=TUICTRL(TSA_UI_KEYS_FORM);if(rc)goto done;keys_owned=1U;
 while(!native.exit){
  rc=TUICAPS(cap);if(rc)break;generation=TJBWORD(cap+60U);
  if(screen){if(TWBLAYOUT(TJBWORD(cap+16U),TJBWORD(cap+20U),&plan)||plan.body_rows<3U){rc=8U;break;}
   height=plan.body_rows-2U;if(height>TBF_ROWS-1U)height=TBF_ROWS-1U;width=plan.body_width;if(width>TBF_WIDTH)width=TBF_WIDTH;}
  (void)TBFVIEW(&native,height,width);
  title=native.screen==TBF_SUBMIT?"BATCH / SUBMIT":native.screen==TBF_JOBS?"BATCH / JOBS":native.screen==TBF_OUTPUT?"BATCH / OUTPUT":"BATCH / JCL PREVIEW";
  strcpy(heading,title);count(heading," / REVISION ",native.revision);title=heading;
  if(native_label(title)||native_label(native.notice)) {rc=12U;break;}
  for(i=0U;i<native.shown;++i)if(TUILINE(native.rows[i],width)){rc=12U;break;}if(rc)break;
  if(screen){
   if(TWBLAYOUT(TJBWORD(cap+16U),TJBWORD(cap+20U),&plan)){rc=8U;break;}
   memset(request,0,sizeof request);TJBPUT(request,1U);TJBPUT(request+4U,64U);TJBPUT(request+8U,handle);TJBPUT(request+12U,generation);
   TJBPUT(request+20U,plan.body);TJBPUT(request+24U,1U);TJBPUT(request+28U,height+2U);TJBPUT(request+32U,width);
   n=(unsigned int)strlen(title);if(TJCENC(title,n,encoded)){rc=8U;break;}TJBPUT(request+40U,n);TJBPUT(request+48U,(unsigned int)(unsigned long)encoded);
   rc=TUIPANEL(handle?TSA_IO_PANEL_UPDATE:TSA_IO_PANEL_CREATE,request);if(rc)break;handle=TJBWORD(request+8U);
   memset(cells,0x40,sizeof cells);n=(unsigned int)strlen(native.notice);if(n>width)n=width;
   if(TJCENC(native.notice,n,cells)){rc=8U;break;}for(i=0U;i<native.shown;++i)memcpy(cells+(i+1U)*width,native.rows[i],width);
   TJBPUT(request+52U,(height+1U)*width);TJBPUT(request+60U,(unsigned int)(unsigned long)cells);rc=TUIPANEL(TSA_IO_PANEL_TEXT,request);if(rc)break;
   TJBPUT(request+8U,0U);rc=TUIPANEL(TSA_IO_PANEL_COMMIT,request);if(rc)break;
  }
  if(native_label("PF3 return; PF4 preview; PF7/8 page; PF10 submit; PF11 jobs; PF12 output")){rc=12U;break;}
  rc=TUIKEY(key,sizeof key,&n,&aid);if(rc)break;
  if(n>256U||TJCDEC(key,n,text)){strcpy(native.notice,"Input is outside the supported interface repertoire");continue;}text[n]=0;
  if(aid==0xf2U){unsigned int capacity;char *value=field_value(&native,native.editing,&capacity);unsigned char draft[256];
   n=(unsigned int)strlen(value);memcpy(text,"EDIT ",5U);memcpy(text+5U,value,n+1U);n+=5U;
   if(screen&&(!TJCENC(text,n,draft)))rc=TUISET(draft,n,n);
   else strcpy(native.notice,"Use EDIT value to update the selected field");if(rc)break;continue;
  }
  (void)TBFEVENT(&native,text,aid);
 }
done:
 native.ready=0U;front->result.valid=0U;
 if(handle){TJBPUT(request+8U,handle);if(TUIPANEL(TSA_IO_PANEL_CLOSE,request))rc=12U;TJBPUT(request+8U,0U);if(TUIPANEL(TSA_IO_PANEL_COMMIT,request))rc=12U;}
 if(keys_owned&&TUICTRL(TSA_UI_KEYS_SHELL))rc=12U;return (int)rc;
}
#endif
