/* SPDX-License-Identifier: MIT; one U controller over the checked K job service.
 * Interfaces are supplied for host controls; native adapters are at the end. */
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include "twospace_submit.h"
#include "twospace_abi.h"
#include "twospace_digest.h"
#ifdef TJF_PCOMM
#include "twospace_batchui.h"
#include "twospace_develop.h"
#endif
static const char *programs[]={"RXC","RXAS","RXLINK","RXVM","BTEST31","BADDR31"};
static void init(unsigned char *h,unsigned int n)
{memset(h,0,n);TJBPUT(h,1U);TJBPUT(h+4U,n);}
static void say(TJFSTATE *s,const char *p){s->ops.line(s->ops.context,p);}
static void append_number(char *p,unsigned int n)
{char digits[11];unsigned int i=0U,k=(unsigned int)strlen(p);do{digits[i++]=(char)('0'+n%10U);n/=10U;}while(n);while(i)p[k++]=digits[--i];p[k]=0;}
static void field(char *p,const char *label,unsigned int n)
{strcat(p,label);append_number(p,n);}
static char *token(char **next)
{char *p=*next,*end;while(*p==' ')++p;if(!*p){*next=p;return 0;}end=p;while(*end&&*end!=' ')++end;if(*end)*end++=0;*next=end;return p;}
static void upper(char *p)
{while(*p){*p=(char)toupper((unsigned char)*p);++p;}}
static int decimal(const char *p,unsigned int *n)
{unsigned int v=0U,d;if(!p||!*p)return 0;while(*p){if(*p<'0'||*p>'9')return 0;d=(unsigned int)(*p++-'0');if(v>429496729U||(v==429496729U&&d>5U))return 0;v=v*10U+d;}*n=v;return v!=0U;}
static int split_name(const char *p,char dsn[45],char member[9])
{
 char name[64],*open,*end;unsigned int n;unsigned char encoded[44];
 n=(unsigned int)strlen(p);if(!n||n>=sizeof name)return 0;strcpy(name,p);
 if(name[0]=='\''){if(n<3U||name[n-1U]!='\'')return 0;name[n-1U]=0;memmove(name,name+1U,n-1U);}
 upper(name);member[0]=0;open=strchr(name,'(');
 if(open){end=strchr(open,')');if(!end||end[1]||end-open<2||end-open>9)return 0;*end=0;strcpy(member,open+1U);*open=0;
  memset(encoded,0x40,8U);if(TJCENC(member,(unsigned int)strlen(member),encoded)||!TJDNAME(encoded))return 0;}
 n=(unsigned int)strlen(name);if(!n||n>44U)return 0;memset(encoded,0x40,sizeof encoded);
 if(TJCENC(name,n,encoded)||!TJDDSN(encoded))return 0;strcpy(dsn,name);return 1;
}
static unsigned int query(TJFSTATE *s,const char *dsn,const char *member,unsigned char h[128])
{
 unsigned int n=(unsigned int)strlen(dsn);if(n>44U)return 8U;init(h,128U);memset(h+40U,0x40,44U);
 if(TJCENC(dsn,n,h+40U))return 8U;
 if(member&&member[0]){n=(unsigned int)strlen(member);if(n>8U)return 8U;memset(h+84U,0x40,8U);if(TJCENC(member,n,h+84U))return 8U;}
 return s->ops.service(s->ops.context,TSA_IO_JOB_QUERY,128U,h,0);
}
static int site(TJFSTATE *s)
{
 unsigned int n=0U,i,k,seen=0U,rc;char row[81],*next,*key,*value,text[100];unsigned char info[128];
 rc=query(s,"PDOS.BATCH.SITE",0,info);
 if(rc){strcpy(text,"SITE catalog query");field(text," RC=",rc);say(s,text);return 8;}
 if(TJBWORD(info+100U)!=TJD_FB||TJBWORD(info+104U)!=80U){say(s,"SITE requires FB80 records");return 8;}
 rc=(unsigned int)s->ops.load(s->ops.context,"PDOS.BATCH.SITE",s->procedure,sizeof s->procedure,&n);
 if(rc){strcpy(text,"SITE input read");field(text," RC=",rc);say(s,text);return 8;}
 if(!n||n%80U){say(s,"SITE input is empty or has incomplete FB80 records");return 8;}
 memset(s->mapping,0,sizeof s->mapping);s->loadlib[0]=s->proclib[0]=s->prefix[0]=s->volume[0]=0;
 for(i=0U;i<n;i+=80U){if(TJCDEC(s->procedure+i,80U,row))return 8;row[80]=0;
  for(k=80U;k&&row[k-1U]==' ';--k)row[k-1U]=0;if(!row[0]||row[0]=='*')continue;
  if(!strcmp(row,"PD033 SITE 1")){if(seen&1U)return 8;seen|=1U;continue;}
  next=row;key=token(&next);value=token(&next);if(!key||!value||token(&next))return 8;
  if(!strcmp(key,"LOADLIB")||!strcmp(key,"PROCLIB")||!strcmp(key,"LIBPREFIX")){
   char dsn[45],member[9];unsigned int flag=!strcmp(key,"LOADLIB")?2U:!strcmp(key,"PROCLIB")?4U:8U;
   if(seen&flag||!split_name(value,dsn,member)||member[0])return 8;seen|=flag;
   strcpy(flag==2U?s->loadlib:flag==4U?s->proclib:s->prefix,dsn);
  }else if(!strcmp(key,"VOLUME")){if(seen&16U||strlen(value)!=6U)return 8;seen|=16U;strcpy(s->volume,value);}
  else {for(k=0U;k<6U&&strcmp(key,programs[k]);++k){}if(k==6U||s->mapping[k][0]||strlen(value)>8U||!value[0])return 8;
   strcpy(s->mapping[k],value);}
 }
 if(seen!=31U)return 8;
 {unsigned char volume[6];if(TJCENC(s->volume,6U,volume)||memcmp(info+116U,volume,6U)){say(s,"SITE VOLUME does not match the selected VOL1");return 8;}}
 for(i=0U;i<4U;++i)if(!s->mapping[i][0])return 8;return 0;
}
static int load_cards(TJFSTATE *s,const char *name,unsigned char *data,unsigned int cap,unsigned int *used)
{
 unsigned char h[128];char dsn[45],member[9],full[56];unsigned int rc;
 if(!split_name(name,dsn,member))return 8;rc=query(s,dsn,member,h);if(rc)return (int)rc;
 if((TJBWORD(h+96U)==0x0200U&&!member[0])||(TJBWORD(h+96U)==0x4000U&&member[0])||
    (TJBWORD(h+96U)!=0x0200U&&TJBWORD(h+96U)!=0x4000U))return 20;
 if(TJBWORD(h+100U)!=TJD_FB||TJBWORD(h+104U)!=80U)return 20;
 strcpy(full,dsn);if(member[0]){strcat(full,"(");strcat(full,member);strcat(full,")");}
 rc=(unsigned int)s->ops.load(s->ops.context,full,data,cap,used);if(rc)return (int)rc;
 return !*used||*used%80U?8:0;
}
static int proc_input(void *context,const char *name,const TJCLIB *libs,unsigned int count,const unsigned char **data,unsigned int *bytes)
{
 TJFSTATE *s=(TJFSTATE *)context;unsigned int i,n=count?count:1U;char full[56];int rc;
 for(i=0U;i<n;++i){strcpy(full,count?libs[i].dsn:s->proclib);strcat(full,"(");strcat(full,name);strcat(full,")");
  rc=load_cards(s,full,s->procedure,sizeof s->procedure,bytes);if(!rc){*data=s->procedure;return 0;}if(rc!=4)return rc;
 }return 4;
}
static int program_input(void *context,const TJCSTEP *step,unsigned char program[8])
{
 TJFSTATE *s=(TJFSTATE *)context;unsigned int i,j,index=(unsigned int)(step-s->work.result.step),n,found=0U;
 unsigned char h[128];char dsn[56],parm[101],*at,*part;unsigned int standard=0U,rc;
 for(i=0U;i<s->work.result.dds;++i){const TJCDECL *d=&s->work.declaration[i];if(d->step!=index+1U)continue;
  if(!strcmp(d->name,"SYSIN"))standard|=1U;else if(!strcmp(d->name,"SYSPRINT"))standard|=2U;else if(!strcmp(d->name,"SYSTERM"))standard|=4U;}
 if(standard!=7U)return 16;
 for(i=0U;i<6U&&strcmp(programs[i],step->program);++i){}if(i==6U||!s->mapping[i][0])return 20;
 n=step->library_count?step->library_count:1U;
 for(j=0U;j<n;++j){const char *library=step->library_count?step->libraries[j].dsn:s->loadlib;
  if(!strcmp(library,s->loadlib)){strcpy(dsn,"TSO31.");strcat(dsn,s->mapping[i]);rc=query(s,dsn,0,h);if(rc)return (int)rc;
   if(TJBWORD(h+96U)!=0x4000U)return 20;found=1U;break;}
  rc=query(s,library,step->program,h);if(!rc)return 20;if(rc!=4U)return (int)rc;
 }
 if(!found)return 4;
 if(i==0U){const unsigned char *d=s->work.result.job+32U+index*TJB_STEP_BYTES;
  n=TJBWORD(d+16U);if(TJCDEC(d+24U,n,parm))return 8;parm[n]=0;at=parm;found=0U;
  while((part=token(&at))!=0)if(!strcmp(part,"-i")){part=token(&at);if(!part||strcmp(part,s->prefix))return 8;found=1U;}
  if(!found)return 8;
  strcpy(dsn,s->prefix);strcat(dsn,".RXBIN");
  {static const char *members[]={"LIBRARY","RXFNSC","RXFNSG","RXCEXITS"};for(j=0U;j<4U;++j)if(query(s,dsn,members[j],h))return 4;}
 }
 memset(program,0x40,8U);return TJCENC(s->mapping[i],(unsigned int)strlen(s->mapping[i]),program);
}
static int dd_input(void *context,const TJCSTEP *step,const TJCDECL *d,TJCATTR *a)
{
 TJFSTATE *s=(TJFSTATE *)context;unsigned char h[128];unsigned int mode=0U,rc;
 if(!strcmp(d->name,"SYSIN")||!strcmp(d->name,"SRC")||!strcmp(d->name,"LIBRARY")||!strcmp(d->name,"BYTES"))mode=TJD_INPUT;
 else if(!strcmp(d->name,"SYSPRINT")||!strcmp(d->name,"SYSTERM")||!strcmp(d->name,"SYSTSPRT")||!strcmp(d->name,"SYSOUT")||!strcmp(d->name,"RESULT"))mode=TJD_OUTPUT;
 else if(!strcmp(d->name,"ASM"))mode=!strcmp(step->program,"RXC")?TJD_OUTPUT:TJD_INPUT;
 else if(!strcmp(d->name,"SYSGO")||!strcmp(d->name,"OBJ"))mode=!strcmp(step->program,"RXAS")?TJD_OUTPUT:TJD_INPUT;
 else if(!strcmp(d->name,"RXBIN")||!strcmp(d->name,"LINK"))mode=!strcmp(step->program,"RXLINK")?TJD_OUTPUT:TJD_INPUT;
 if(!mode||(a->direction&&a->direction!=mode))return 20;a->direction=mode;
 if(d->volume[0]&&strcmp(d->volume,s->volume))return 20;
 if(d->kind==TJD_DATASET){
  if(mode==TJD_OUTPUT){char library[52];strcpy(library,s->prefix);strcat(library,".RXBIN");
   if(!strcmp(d->dsn,library)||!strcmp(d->dsn,s->proclib)||!strcmp(d->dsn,s->loadlib)||!strcmp(d->dsn,"PDOS.BATCH.SITE"))return 20;}
  rc=query(s,d->dsn,d->member,h);
  if(rc==4U&&mode==TJD_OUTPUT&&d->member[0]){rc=query(s,d->dsn,0,h);if(!rc&&TJBWORD(h+96U)!=0x0200U)return 20;}
  if(rc)return (int)rc;
  if((TJBWORD(h+96U)==0x0200U&&!d->member[0])||(TJBWORD(h+96U)==0x4000U&&d->member[0])||
     (TJBWORD(h+96U)!=0x0200U&&TJBWORD(h+96U)!=0x4000U))return 20;
  a->recfm=TJBWORD(h+100U);a->lrecl=TJBWORD(h+104U);a->blksize=TJBWORD(h+108U);
 }else{
  if(!a->recfm)a->recfm=TJD_FB;
  if(!a->lrecl)a->lrecl=d->kind==TJD_SYSOUT?132U:80U;
  if(!a->blksize)a->blksize=a->recfm==TJD_VB?a->lrecl+4U:a->lrecl;
 }return 0;
}
static unsigned int action(TJFSTATE *s,unsigned int id,unsigned int op)
{init(s->header,128U);TJBPUT(s->header+8U,id);return s->ops.service(s->ops.context,op,128U,s->header,0);}
static void summary(TJFSTATE *s)
{
 char text[160];unsigned char *h=s->header;strcpy(text,"JOB ");append_number(text,TJBWORD(h+8U));
 field(text," STATE=",TJBWORD(h+48U));field(text," PRINT=",TJBWORD(h+68U));field(text," OS=",TJBWORD(h+72U));
 if(TJBWORD(h+76U))field(text," RC=",TJBWORD(h+80U));else strcat(text," RC=unavailable");say(s,text);
}
static int metadata(TJFSTATE *s,unsigned int id)
{init(s->header,128U);TJBPUT(s->header+8U,id);TJBPUT(s->header+16U,TJB_META_BYTES);
 return (int)s->ops.service(s->ops.context,TSA_IO_JOB_SOURCE,128U,s->header,s->meta);}
static void hash_text(char *out,const unsigned char *p)
{unsigned int i;static const char hex[]="0123456789abcdef";for(i=0U;i<32U;++i){out[2U*i]=hex[p[i]>>4];out[2U*i+1U]=hex[p[i]&15U];}out[64]=0;}
static void step_name(TJFSTATE *s,unsigned int index,const unsigned char *legacy,char name[19])
{unsigned int n=TJBWORD(s->meta)==0x544a4d31U?18U:8U;const unsigned char *p=n==18U?s->meta+48U+18U*index:legacy;
 while(n&&p[n-1U]==0x40U)--n;if(TJCDEC(p,n,name))n=0U;name[n]=0;}
static int status(TJFSTATE *s,unsigned int id)
{
 unsigned int rc,i,count;unsigned char step[160];char name[19],text[180];
 rc=action(s,id,TSA_IO_JOB_STATUS);if(rc)return (int)rc;summary(s);count=TJBWORD(s->header+56U);
 if(metadata(s,id))return 12;
 if(TJBWORD(s->meta)==0x544a4d31U){
  say(s,TJBWORD(s->meta+228U)&TJB_OUTPUT_HOLD?"OUTPUT POLICY=HELD (PRINT delivers; OUTPUT reviews)":"OUTPUT POLICY=PRINT (OUTPUT remains available)");
  strcpy(text,"SOURCE SHA256 ");hash_text(text+14U,s->meta+16U);say(s,text);
  for(i=0U;i<TJBWORD(s->meta+224U);++i){char procname[9];if(TJCDEC(s->meta+232U+8U*i,8U,procname))return 12;procname[8]=0;
   strcpy(text,"PROC ");strcat(text,procname);strcat(text," SHA256 ");hash_text(text+strlen(text),s->meta+264U+32U*i);say(s,text);}}
 for(i=0U;i<count;++i){init(s->header,128U);TJBPUT(s->header+8U,id);TJBPUT(s->header+12U,i+1U);TJBPUT(s->header+16U,sizeof step);
  rc=s->ops.service(s->ops.context,TSA_IO_JOB_STATUS,128U,s->header,step);if(rc)return (int)rc;
  step_name(s,i,step,name);strcpy(text,"STEP ");strcat(text,name);field(text," STATE=",TJBWORD(step+148U));field(text," OS=",TJBWORD(step+124U));
  if(TJBWORD(step+128U))field(text," RC=",TJBWORD(step+132U));else strcat(text," RC=unavailable");
  field(text," SKIP=",TJBWORD(step+144U));say(s,text);
 }return 0;
}
static unsigned int endpoint(TJFSTATE *s,unsigned int kind,unsigned int *handle)
{unsigned int rc;init(s->header,64U);TJBPUT(s->header+12U,kind);TJBPUT(s->header+16U,kind==1U?12U:14U);
 rc=s->ops.service(s->ops.context,TSA_IO_BATCH_OPEN,64U,s->header,0);if(!rc)*handle=TJBWORD(s->header+8U);return rc;}
static unsigned int close_endpoint(TJFSTATE *s,unsigned int handle)
{init(s->header,64U);TJBPUT(s->header+8U,handle);return s->ops.service(s->ops.context,TSA_IO_BATCH_CLOSE,64U,s->header,0);}
static int print_job(TJFSTATE *s,unsigned int id)
{unsigned int handle,rc,close;rc=endpoint(s,2U,&handle);if(rc)return (int)rc;
 init(s->header,128U);TJBPUT(s->header+8U,id);TJBPUT(s->header+20U,handle);
 rc=s->ops.service(s->ops.context,TSA_IO_JOB_PRINT,128U,s->header,0);close=close_endpoint(s,handle);return (int)(rc?rc:close);}
static int delivery(TJFSTATE *s,unsigned int id)
{char text[128];if(metadata(s,id))return 12;
 if(TJBWORD(s->meta+228U)&TJB_OUTPUT_HOLD){strcpy(text,"OUTPUT HELD; OUTPUT ");append_number(text,id);
  strcat(text," reviews it; PRINT ");append_number(text,id);strcat(text," delivers it");say(s,text);return 0;}
 return print_job(s,id);}
static int reader(TJFSTATE *s,unsigned int *bytes)
{
 unsigned int handle,rc,n=0U,close;unsigned char extra[80],*p;
 rc=endpoint(s,1U,&handle);if(rc)return (int)rc;
 for(;;){p=n<sizeof s->deck?s->deck+n:extra;init(s->header,64U);TJBPUT(s->header+8U,handle);TJBPUT(s->header+24U,80U);
  rc=s->ops.service(s->ops.context,TSA_IO_BATCH_READ,64U,s->header,p);if(rc)break;
  if(TJBWORD(s->header+60U)&1U){if(TJBWORD(s->header+28U))rc=8U;break;}
  if(TJBWORD(s->header+28U)!=80U){rc=8U;break;}if(n==sizeof s->deck){rc=4U;break;}n+=80U;
 }
 close=close_endpoint(s,handle);if(rc||close)return (int)(rc?rc:close);*bytes=n;return 0;
}
static int console_cards(TJFSTATE *s,const char *identity,unsigned int *bytes)
{
 char end[40],abort[40],text[257];unsigned char raw[256];unsigned int n,total=0U,bad=0U;
 if(!identity||!identity[0]||strlen(identity)>16U)return 8;strcpy(end,"END ");strcat(end,identity);strcpy(abort,"ABORT ");strcat(abort,identity);
 say(s,"JOB INPUT: enter cards; finish with the matching END token, or ABORT token");
 for(;;){n=0U;if(s->ops.input(s->ops.context,raw,sizeof raw,&n)||n>256U)return 12;
  if(TJCDEC(raw,n,text)){bad=1U;continue;}text[n]=0;
  if(!strcmp(text,abort)){say(s,"JOB INPUT ABORTED; no job submitted");return 16;}
  if(!strcmp(text,end)){if(bad)return 8;*bytes=total;return 0;}
  if(n>80U||total==sizeof s->deck){if(!bad)say(s,"JOB INPUT REJECTED; drain to matching END or ABORT token");bad=1U;continue;}
  if(!bad){memset(s->deck+total,0x40,80U);memcpy(s->deck+total,raw,n);total+=80U;}
 }
}
int TJFCONFIG(TJFSTATE *s,const TJFOPS *ops)
{
 if(!s||!ops||!ops->load||!ops->service||!ops->input||!ops->line||!ops->record)return 8;
 s->ops=*ops;s->result.valid=0U;
 if(site(s)){say(s,"SUBMIT: invalid or unavailable PDOS.BATCH.SITE");return 8;}return 0;
}
int TJFPLAN(TJFSTATE *s,const char *source,const unsigned char *cards,unsigned int n)
{
 unsigned int rc;char text[180];TJCERROR error;TJCSITE callbacks;
 if(!s||!s->ops.service)return 8;s->result.valid=0U;s->source_bytes=0U;
 if(source){if(cards||n)return 8;rc=(unsigned int)load_cards(s,source,s->deck,sizeof s->deck,&n);if(rc)return (int)rc;}
 else {if(!cards||!n||n>sizeof s->deck||n%80U)return 8;memmove(s->deck,cards,n);}
 s->source_bytes=n;memset(&callbacks,0,sizeof callbacks);callbacks.context=s;callbacks.program=program_input;callbacks.dd=dd_input;callbacks.procedure=proc_input;
 rc=(unsigned int)TJCCONV(&s->work,s->deck,n,&callbacks,&s->result,&error);
 if(rc){strcpy(text,"JCL ERROR");field(text," CARD=",error.card);field(text," COLUMN=",error.column);strcat(text," ");strcat(text,TJCMSG(error.reason));say(s,text);}return (int)rc;
}
int TJFPOST(TJFSTATE *s,unsigned int held,unsigned int output_held,unsigned int *identity)
{
 unsigned int id,rc;char text[80];
 if(identity)*identity=0U;if(!s||!identity||!s->result.valid||held>1U||output_held>1U)return 8;
 /* Never retry a prepared publication, including an uncertain service error. */
 s->result.valid=0U;
 if(output_held)TJBPUT(s->result.job+TJB_DESC_BYTES+228U,TJB_OUTPUT_HOLD);
 init(s->header,128U);TJBPUT(s->header+12U,1U);TJBPUT(s->header+16U,TJB_FULL_BYTES+TJD_EXT_BYTES);
 rc=s->ops.service(s->ops.context,TSA_IO_JOB_SUBMIT,128U,s->header,s->result.job);if(rc)return (int)rc;
 id=TJBWORD(s->header+8U);if(!id)return 12;*identity=id;strcpy(text,"SUBMITTED JOB ");append_number(text,id);say(s,text);
 if(held){rc=action(s,id,TSA_IO_JOB_HOLD);if(!rc)summary(s);return (int)rc;}
 rc=action(s,id,TSA_IO_JOB_RUN);if(rc){say(s,"JOB retained; execution not started");return (int)rc;}summary(s);
 rc=(unsigned int)delivery(s,id);if(rc)say(s,"JOB execution retained; print delivery failed; OUTPUT remains available");
 if(!action(s,id,TSA_IO_JOB_STATUS))summary(s);return (int)rc;
}
static int submit(TJFSTATE *s,char *source,char *next)
{
 unsigned int n=0U,id,rc,held=0U,output_held=0U;char *arg,*identity=0;
 if(!source)return 8;
 if(!strcmp(source,"*")){identity=token(&next);if(!identity)return 8;}
 while((arg=token(&next))!=0){upper(arg);
  if(!strcmp(arg,"RUNHOLD")&&!held)held=1U;
  else if(!strcmp(arg,"HOLD")&&!output_held)output_held=1U;
  else return 8;}
 upper(source);
 /* Once console framing is requested, consume to its explicit end even if
    site admission will fail. Remaining cards must never become shell commands. */
 if(!strcmp(source,"*")){rc=(unsigned int)console_cards(s,identity,&n);if(rc)return (int)rc;}
 if(TJFCONFIG(s,&s->ops))return 8;
 rc=(unsigned int)(!strcmp(source,"READER")?reader(s,&n):!strcmp(source,"*")?0:load_cards(s,source,s->deck,sizeof s->deck,&n));
 if(rc)return (int)rc;rc=(unsigned int)TJFPLAN(s,0,s->deck,n);if(rc)return (int)rc;
 return TJFPOST(s,held,output_held,&id);
}
static int output(TJFSTATE *s,unsigned int id,char *next)
{
 unsigned int rc,selected=0U,at=0U,n,i,count,step;char *part,*want_step=0,*want_dd=0,name[19],text[128];unsigned char ddname[8],legacy[160];
 while((part=token(&next))!=0){upper(part);if(!strncmp(part,"STEP=",5U)&&part[5]&&!want_step)want_step=part+5;
  else if(!strncmp(part,"DD=",3U)&&part[3]&&!want_dd)want_dd=part+3;else return 8;}
 if(want_dd){n=(unsigned int)strlen(want_dd);if(n>8U)return 8;memset(ddname,0x40,8U);if(TJCENC(want_dd,n,ddname)||!TJDNAME(ddname))return 8;}
 rc=action(s,id,TSA_IO_JOB_STATUS);if(rc)return (int)rc;count=TJBWORD(s->header+56U);if(metadata(s,id))return 12;
 if(want_step){for(i=0U;i<count;++i){init(s->header,128U);TJBPUT(s->header+8U,id);TJBPUT(s->header+12U,i+1U);TJBPUT(s->header+16U,sizeof legacy);
   rc=s->ops.service(s->ops.context,TSA_IO_JOB_STATUS,128U,s->header,legacy);if(rc)return (int)rc;step_name(s,i,legacy,name);if(!strcmp(name,want_step)){selected=i+1U;break;}}
  if(!selected)return 8;}
 init(s->header,128U);TJBPUT(s->header+8U,id);TJBPUT(s->header+16U,sizeof s->output);
 rc=s->ops.service(s->ops.context,TSA_IO_JOB_OUTPUT,128U,s->header,s->output);if(rc)return (int)rc;n=TJBWORD(s->header+32U);if(n>sizeof s->output)return 12;
 while(at<n){const unsigned char *r=s->output+at;unsigned int len;if(n-at<24U)return 12;len=TJBWORD(r+8U);step=TJBWORD(r+4U);
  if(!len||len>132U||len>n-at-24U||!step||step>count)return 12;
  if((!selected||selected==step)&&(!want_dd||!memcmp(ddname,r+16U,8U))){
   if(TJBWORD(s->meta)==0x544a4d31U)step_name(s,step-1U,r+16U,name);else {strcpy(name,"#");append_number(name,step);}
   strcpy(text,"STEP ");strcat(text,name);strcat(text," DD ");{char label[9];if(TJCDEC(r+16U,8U,label))return 12;label[8]=0;strcat(text,label);}field(text," SEQ=",TJBWORD(r));say(s,text);
   s->ops.record(s->ops.context,r+24U,len);
  }at+=24U+len;
 }return 0;
}
int TJFKNOW(const char *command)
{
 char text[199],*at,*first,*second;unsigned int n;
#ifdef TJF_PCOMM
 if(TDVKNOW(command))return 1;
#endif
 if(!command)return 0;n=(unsigned int)strlen(command);if(n>=sizeof text)return 0;
 strcpy(text,command);at=text;first=token(&at);if(!first)return 0;upper(first);
 if(!strcmp(first,"HELP")){second=token(&at);if(!second)return 0;upper(second);return !strcmp(second,"SUBMIT")||!strcmp(second,"JOBS")||!strcmp(second,"BATCH");}
 if(!strcmp(first,"BATCH"))return 1;
 return !strcmp(first,"SUBMIT")||!strcmp(first,"JOBS")||!strcmp(first,"STATUS")||!strcmp(first,"OUTPUT")||!strcmp(first,"HOLD")||!strcmp(first,"RELEASE")||!strcmp(first,"CANCEL")||!strcmp(first,"PURGE")||!strcmp(first,"PRINT");
}
int TJFCMD(TJFSTATE *s,const TJFOPS *ops,const char *command)
{
 char text[199],*next,*verb,*arg,line[128];unsigned int rc,id,i,state;int result;
 if(!s||!ops||!ops->load||!ops->service||!ops->input||!ops->line||!ops->record||!command||strlen(command)>198U)return 8;
 s->ops=*ops;strcpy(text,command);next=text;verb=token(&next);if(!verb)return 8;upper(verb);
 if(!strcmp(verb,"HELP")){say(s,"SUBMIT dataset(member)|READER [HOLD] [RUNHOLD]; SUBMIT * token [HOLD] [RUNHOLD]");
  say(s,"HOLD retains output for review; RUNHOLD delays execution until RELEASE id");
  say(s,"JOBS; STATUS id; OUTPUT id [STEP=name] [DD=name]; PRINT id");say(s,"HOLD id holds a queued job; RELEASE id dispatches; CANCEL id; PURGE id");
  say(s,"Active job: interrupt-key STATUS/CANCEL/CONTINUE; other commands busy");return 0;}
 arg=token(&next);
 if(!strcmp(verb,"SUBMIT"))result=submit(s,arg,next);
 else if(!strcmp(verb,"JOBS")){if(arg)return 8;rc=action(s,0U,TSA_IO_JOB_STATUS);if(rc)return (int)rc;
  for(i=0U;i<TJB_SLOTS;++i){id=TJBWORD(s->header+48U+12U*i);if(!id)continue;strcpy(line,"JOB ");append_number(line,id);field(line," STATE=",TJBWORD(s->header+52U+12U*i));field(line," PRINT=",TJBWORD(s->header+56U+12U*i));say(s,line);}return 0;
 }else{
  if(!decimal(arg,&id))return 8;
  if(!strcmp(verb,"OUTPUT"))result=output(s,id,next);
  else {if(token(&next))return 8;
   if(!strcmp(verb,"STATUS"))result=status(s,id);
   else if(!strcmp(verb,"PRINT"))result=print_job(s,id);
   else if(!strcmp(verb,"RELEASE")){rc=action(s,id,TSA_IO_JOB_STATUS);if(rc)return (int)rc;state=TJBWORD(s->header+48U);
    if(state==TJB_HELD)rc=action(s,id,TSA_IO_JOB_RELEASE);else if(state!=TJB_QUEUED)rc=16U;
    if(!rc)rc=action(s,id,TSA_IO_JOB_RUN);if(!rc){summary(s);rc=(unsigned int)delivery(s,id);}result=(int)rc;
   }else {unsigned int op=!strcmp(verb,"HOLD")?TSA_IO_JOB_HOLD:!strcmp(verb,"CANCEL")?TSA_IO_JOB_CANCEL:!strcmp(verb,"PURGE")?TSA_IO_JOB_PURGE:0U;
    if(!op)return 8;result=(int)action(s,id,op);if(!result&&op!=TSA_IO_JOB_PURGE)summary(s);}
  }
 }
 if(result){strcpy(line,"BATCH COMMAND STATUS=");append_number(line,(unsigned int)result);say(s,line);}return result;
}
#ifdef TJF_PCOMM
#include <stdio.h>
#include <stdlib.h>
#include "twospace_ui.h"
static TJFSTATE native_state;
static int native_load(void *context,const char *name,unsigned char *data,unsigned int cap,unsigned int *used)
{
 FILE *f;unsigned int n;int extra;(void)context;if(strlen(name)>55U)return 8;
 /* Callers supply a normalized DSN/member. PDPCLIB's native PDOS file
    interface takes that form directly, without TSO command-line quotes. */
 f=fopen(name,"rb");if(!f)return 4;
 n=(unsigned int)fread(data,1U,cap,f);extra=fgetc(f);if(ferror(f)||extra!=EOF){fclose(f);return 8;}
 if(fclose(f))return 12;*used=n;return 0;
}
static unsigned int native_service(void *context,unsigned int op,unsigned int n,unsigned char *h,void *payload)
{unsigned int offset=op==TSA_IO_BATCH_READ||op==TSA_IO_BATCH_WRITE?32U:24U;(void)context;
 if(sizeof(void *)!=4U)return 20U;if(payload){TJBPUT(h+offset,0U);TJBPUT(h+offset+4U,(unsigned int)(unsigned long)payload);}return TUIIO(n,h,op);}
static int native_input(void *context,unsigned char *p,unsigned int cap,unsigned int *used)
{(void)context;return (int)TUIREAD(p,cap,used);}
static void native_line(void *context,const char *p)
{unsigned int n=(unsigned int)strlen(p),at=0U;unsigned char encoded[130];(void)context;
 /* Controller labels use C execution characters; the console takes IBM1047.
    Application SYSOUT already has its external encoding: see native_record. */
 while(at<n){unsigned int take=n-at>130U?130U:n-at;if(TJCENC(p+at,take,encoded))return;TUILINE(encoded,take);at+=take;}}
static void native_record(void *context,const unsigned char *p,unsigned int n)
{(void)context;TUILINE(p,n);}
static TDVSTATE developer;
static unsigned int developer_file(void *context,unsigned int action,const char *name,unsigned char *p,unsigned int cap,unsigned char *info)
{(void)context;return TEFFILE(action,name,p,cap,info);}
static int developer_edit(void *context,const char *name,unsigned int force_line)
{
 char command[80];unsigned char caps[64],handoff[32];unsigned int rc;
 (void)context;rc=TUICAPS(caps);if(rc)return (int)rc;
 if(TJBWORD(caps+TSA_CAP_DEVICE_CLASS)==TSA_DEVICE_3270){
  /* Raw CLOSE repaints this controller's semantic input field. Surrender
   * that lease before a child editor acquires its own raw presentation. */
  init(handoff,sizeof handoff);rc=TUIIO(sizeof handoff,handoff,TSA_IO_SCREEN_ACQUIRE);if(rc)return (int)rc;
  init(handoff,sizeof handoff);rc=TUIIO(sizeof handoff,handoff,TSA_IO_SCREEN_RELEASE);if(rc)return (int)rc;
 }
 strcpy(command,"EDIT ");strcat(command,name);if(force_line)strcat(command," LINE");return system(command);
}
static unsigned int developer_present(void *context,const unsigned char *p,unsigned int n)
{(void)context;return TUILINE(p,n);}
int TJFENTRY(const char *command)
{TJFOPS ops;char text[199];memset(&ops,0,sizeof ops);ops.load=native_load;ops.service=native_service;ops.input=native_input;ops.line=native_line;ops.record=native_record;
 if(!command||strlen(command)>=sizeof text)return 8;strcpy(text,command);upper(text);
 if(TDVKNOW(command)){TDVOPS dev;unsigned char caps[64];dev.batch=ops;dev.file=developer_file;dev.edit=developer_edit;dev.present=developer_present;
  if(TDVCONFIG(&developer,&native_state,&dev))return 8;
  if(!strcmp(text,"PROGRAMS")||!strcmp(text,"CREXX")){
   if(TUICAPS(caps))return 12;if(TJBWORD(caps+TSA_CAP_DEVICE_CLASS)!=TSA_DEVICE_LINE)return TDVFORM(&developer);
  }return TDVCMD(&developer,command);
 }
 if(!strncmp(text,"BATCH",5U)&&(!text[5]||text[5]==' '))return TBFENTRY(&native_state,&ops,text[5]?command+6U:"");
 return TJFCMD(&native_state,&ops,command);}
#endif
