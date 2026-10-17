/* SPDX-License-Identifier: MIT; original portable developer controller.
 * Jobs use the existing converter and exactly-once POST. Publication is a
 * checked two-pair journal protocol over the existing compare/save service. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "twospace_develop.h"
#include "twospace_abi.h"
#include "twospace_digest.h"
#define HEADER TDV_HEADER
#define ENTRY TDV_ENTRY
static unsigned int status(TDVSTATE *,unsigned int,unsigned int,unsigned char *,unsigned char *);
static int recover(TDVSTATE *);
static void init(unsigned char *p,unsigned int n)
{memset(p,0,n);TJBPUT(p,1U);TJBPUT(p+4U,n);}
static void say(TDVSTATE *s,const char *text)
{if(text!=s->notice){strncpy(s->notice,text,sizeof s->notice-1U);s->notice[sizeof s->notice-1U]=0;}s->ops.batch.line(s->ops.batch.context,s->notice);}
static char *word(char **p)
{char *b,*e;while(**p==' ')++*p;b=*p;if(!*b)return 0;e=b;while(*e&&*e!=' ')++e;if(*e)*e++=0;*p=e;return b;}
static void upper(char *p){while(*p){*p=(char)toupper((unsigned char)*p);++p;}}
static int name(const char *p,unsigned int dsn)
{unsigned char b[44];unsigned int n;if(!p)return 0;n=(unsigned int)strlen(p);if(!n||n>(dsn?44U:8U))return 0;memset(b,0x40,sizeof b);if(TJCENC(p,n,b))return 0;return dsn?TJDDSN(b):TJDNAME(b);}
static unsigned int service(TDVSTATE *s,unsigned int op,unsigned char *h,void *p)
{return s->ops.batch.service(s->ops.batch.context,op,128U,h,p);}
static unsigned int file(TDVSTATE *s,unsigned int action,const char *id,unsigned char *p,unsigned int n,unsigned char *info)
{return s->ops.file(s->ops.batch.context,action,id,p,n,info);}
static int fail(TDVSTATE *s,const char *what,unsigned int rc)
{char text[160];sprintf(text,"%s (%u); no automatic retry",what,rc);say(s,text);return (int)rc;}
static void full(char *out,const char *dsn,const char *member)
{strcpy(out,dsn);strcat(out,"(");strcat(out,member);strcat(out,")");}
static unsigned char *entry(TDVSTATE *s,unsigned int i){return s->catalog+HEADER+i*ENTRY+2U;}
static int find(TDVSTATE *s,const char *member)
{unsigned int i;unsigned char b[8];memset(b,0x40,8U);TJCENC(member,(unsigned int)strlen(member),b);for(i=0U;i<s->count;++i)if(!memcmp(entry(s,i),b,8U))return (int)i;return -1;}
static int catalog(TDVSTATE *s)
{
 unsigned int rc,i,j,n;unsigned char expected[176],binding[176],*e;char *names[4];memset(s->catalog_info,0,TEF_BYTES);
 rc=file(s,TEF_LOAD,s->registry,s->catalog,TDV_REGISTRY,s->catalog_info);if(rc)return fail(s,"Workspace registry unavailable",rc);
 if(TJBWORD(s->catalog_info+20U)!=TJD_VB||TJBWORD(s->catalog_info+24U)!=260U||TJBWORD(s->catalog_info+28U)!=264U)return fail(s,"Unsupported registry format",20U);
 s->catalog_bytes=TJBWORD(s->catalog_info+16U);s->count=0U;
 memset(expected,0,sizeof expected);expected[0]=0x54U;expected[1]=0x44U;expected[2]=0x56U;expected[3]=3U;
 names[0]=s->source;names[1]=s->output;names[2]=s->work;names[3]=s->registry;
 memset(binding,0x40,sizeof binding);for(i=0U;i<4U;++i)TJCENC(names[i],(unsigned int)strlen(names[i]),binding+i*44U);
 TJJHASH(binding,sizeof binding,expected+8U);memset(expected+40U,0x40,88U);TJCENC(s->source,(unsigned int)strlen(s->source),expected+40U);TJCENC(s->output,(unsigned int)strlen(s->output),expected+84U);
 if(!s->catalog_bytes){s->catalog[0]=0U;s->catalog[1]=176U;memcpy(s->catalog+2U,expected,176U);s->catalog_bytes=HEADER;return 0;}
 if(s->catalog_bytes<HEADER||s->catalog[0]||s->catalog[1]!=176U)return fail(s,"Malformed workspace registry",8U);
 s->count=TJBWORD(s->catalog+6U);if(s->count>TDV_PROGRAMS)return fail(s,"Invalid workspace program count",8U);
 TJBPUT(expected+4U,s->count);
 if(memcmp(s->catalog+2U,expected,128U)||s->catalog_bytes!=HEADER+s->count*ENTRY)return fail(s,"Workspace profile changed or registry invalid",16U);
 for(i=0U;i<s->count;++i){e=entry(s,i);n=HEADER+i*ENTRY;
  if(s->catalog[n]||s->catalog[n+1U]!=208U||!TJDNAME(e)||TJBWORD(e+8U)>2U||TJBWORD(e+12U))return fail(s,"Invalid program registry entry",8U);
  for(j=16U+TJBWORD(e+8U)*TDV_PAIR;j<208U;++j)if(e[j])return fail(s,"Nonzero unused build identity",8U);
  for(j=0U;j<i;++j)if(!memcmp(e,entry(s,j),8U))return fail(s,"Duplicate program registry entry",8U);
 }
 if(TJBWORD(s->catalog+130U)){
  if((TJBWORD(s->catalog+134U)!=1U&&TJBWORD(s->catalog+134U)!=3U)||!TJDNAME(s->catalog+138U))return fail(s,"Invalid selected-result record",8U);
 }else for(i=130U;i<HEADER;++i)if(s->catalog[i])return fail(s,"Invalid empty selected-result record",8U);
 if(!s->selected)recover(s);return 0;
}
static int commit_catalog(TDVSTATE *s)
{
 unsigned int rc,n;unsigned char *check,info[TEF_BYTES];
 TJBPUT(s->catalog+6U,s->count);rc=file(s,TEF_SAVE,s->registry,s->catalog,s->catalog_bytes,s->catalog_info);
 if(rc==12U){check=(unsigned char *)malloc(TDV_REGISTRY);if(!check)return 4;memset(info,0,sizeof info);
  n=file(s,TEF_LOAD,s->registry,check,TDV_REGISTRY,info);
  if(!n&&TJBWORD(info+16U)==s->catalog_bytes&&!memcmp(check,s->catalog,s->catalog_bytes)){memcpy(s->catalog_info,info,sizeof info);rc=0U;}free(check);
 }return rc?fail(s,"Registry save refused/uncertain; executable unchanged",rc):0;
}
static int register_member(TDVSTATE *s,const char *member)
{
 unsigned char *e;int index=find(s,member);if(index>=0)return index;
 if(s->count==TDV_PROGRAMS){say(s,"Workspace full: this profile supports two programs; existing programs are safe");return -1;}
 index=(int)s->count++;s->catalog[s->catalog_bytes]=0U;s->catalog[s->catalog_bytes+1U]=208U;
 e=entry(s,(unsigned int)index);memset(e,0,208U);memset(e,0x40,8U);TJCENC(member,(unsigned int)strlen(member),e);s->catalog_bytes+=ENTRY;
 if(commit_catalog(s))return -1;return index;
}
int TDVCONFIG(TDVSTATE *s,TJFSTATE *jobs,const TDVOPS *ops)
{
 unsigned char cards[800],info[128];unsigned int n=0U,i,k,seen=0U,flag;char row[81],*p,*key,*value,*fields[4];
 if(!s||!jobs||!ops||!ops->file||!ops->edit||!ops->present)return 8;s->ops=*ops;s->jobs=jobs;s->ready=0U;
 if(TJFCONFIG(jobs,&ops->batch))return 8;
 init(info,128U);memset(info+40U,0x40,44U);TJCENC("PDOS.DEV.SITE",13U,info+40U);
 if(service(s,TSA_IO_JOB_QUERY,info,0)||TJBWORD(info+96U)!=0x4000U||TJBWORD(info+100U)!=TJD_FB||TJBWORD(info+104U)!=80U)return fail(s,"Developer SITE requires sequential FB80",20U);
 if(ops->batch.load(ops->batch.context,"PDOS.DEV.SITE",cards,sizeof cards,&n)||!n||n%80U)return fail(s,"Installed developer profile unavailable",8U);
 fields[0]=s->source;fields[1]=s->output;fields[2]=s->work;fields[3]=s->registry;
 for(i=0U;i<n;i+=80U){if(TJCDEC(cards+i,80U,row))return 8;row[80]=0;for(k=80U;k&&row[k-1U]==' ';--k)row[k-1U]=0;
  if(!*row||*row=='*')continue;if(!strcmp(row,"PD035 DEV 1")){if(seen&1U)return 8;seen|=1U;continue;}
  p=row;key=word(&p);value=word(&p);if(!key||!value||word(&p))return 8;
  if(!strcmp(key,"PROFILE")){if(seen&32U||strlen(value)!=64U)return 8;for(k=0U;k<64U;++k)if(!strchr("0123456789abcdef",value[k]))return 8;seen|=32U;continue;}
  for(k=0U;k<4U&&strcmp(key,k==0U?"SOURCE":k==1U?"OUTPUT":k==2U?"WORK":"STATE");++k){}if(k==4U||!name(value,1U))return 8;
  flag=2U<<k;if(seen&flag)return 8;seen|=flag;strcpy(fields[k],value);
 }
 if(seen!=63U)return fail(s,"Incomplete developer profile",8U);
 for(i=0U;i<4U;++i){char runtime[56];strcpy(runtime,jobs->prefix);strcat(runtime,".RXBIN");
  if(!strcmp(fields[i],jobs->loadlib)||!strcmp(fields[i],jobs->proclib)||!strcmp(fields[i],runtime)||!strcmp(fields[i],"PDOS.BATCH.SITE")||!strcmp(fields[i],"PDOS.DEV.SITE"))return fail(s,"Unsafe workspace alias",8U);
  for(k=0U;k<i;++k)if(!strcmp(fields[i],fields[k]))return fail(s,"Workspace names must be distinct",8U);
  init(info,128U);memset(info+40U,0x40,44U);TJCENC(fields[i],(unsigned int)strlen(fields[i]),info+40U);
  if(service(s,TSA_IO_JOB_QUERY,info,0)||TJBWORD(info+96U)!=(i<2U?0x0200U:0x4000U))return fail(s,"Workspace catalog/organisation invalid",8U);
  if(i==0U?(TJBWORD(info+100U)!=TJD_FB||TJBWORD(info+104U)!=80U):(TJBWORD(info+100U)!=TJD_VB||TJBWORD(info+104U)!=260U||TJBWORD(info+108U)!=264U))return fail(s,"Workspace record format invalid",20U);
 }
 TJJHASH(cards,n,s->profile);if(catalog(s))return 8;s->ready=1U;return 0;
}
static unsigned int status(TDVSTATE *s,unsigned int id,unsigned int step,unsigned char *h,unsigned char *p)
{init(h,128U);TJBPUT(h+8U,id);TJBPUT(h+12U,step);if(p)TJBPUT(h+16U,TJB_STEP_BYTES);return service(s,TSA_IO_JOB_STATUS,h,p);}
static void descriptor_hash(const unsigned char *steps,unsigned int count,unsigned char hash[32])
{
 unsigned char canonical[TJB_STEPS*124U];unsigned int i;
 for(i=0U;i<count;++i)memcpy(canonical+i*124U,steps+i*TJB_STEP_BYTES,124U);
 TJJHASH(canonical,count*124U,hash);
}
static int recover(TDVSTATE *s)
{
 unsigned char h[128],steps[TJB_STEPS*TJB_STEP_BYTES],hash[32];unsigned int id=TJBWORD(s->catalog+130U),i,count=TJBWORD(s->catalog+134U);
 if(!id||!count||status(s,id,0U,h,0)||TJBWORD(h+56U)!=count)return 4;
 for(i=0U;i<count;++i)if(status(s,id,i+1U,h,steps+i*TJB_STEP_BYTES))return 4;
 descriptor_hash(steps,count,hash);if(memcmp(hash,s->catalog+146U,32U))return 16;
 s->selected=id;s->kind=count;s->reviewed=0U;return 0;
}
static int result(TDVSTATE *s,unsigned int id,unsigned int expected)
{
 unsigned char h[128],step[TJB_STEP_BYTES];unsigned int i,rc;char text[120];static const char *stages[]={"Compile","Assemble","Link"};
 rc=status(s,id,0U,h,0);if(rc)return fail(s,"Job status unavailable",rc);
 if(TJBWORD(h+56U)!=expected)return fail(s,"Unexpected job shape",12U);
 for(i=1U;i<=expected;++i){rc=status(s,id,i,h,step);if(rc)return fail(s,"Step status unavailable",rc);
  if(TJBWORD(step+148U)!=TJB_NORMAL||TJBWORD(step+124U)||TJBWORD(step+128U)!=1U||TJBWORD(step+132U)){
   sprintf(text,"%s failed: OS %u / RC %s%u; OUTPUT shows diagnostics",expected==1U?"Run":stages[i-1U],TJBWORD(step+124U),TJBWORD(step+128U)?"":"unavailable ",TJBWORD(step+132U));say(s,text);return 8;}
 }
 if(TJBWORD(h+48U)!=TJB_DONE)return fail(s,"Job did not complete normally",8U);return 0;
}
static int output(TDVSTATE *s)
{
 unsigned char h[128];unsigned int at=0U,n,rc,len,sequence=0U;unsigned char empty[17];const unsigned char *r;
 if(!s->selected){say(s,"No selected result; JOBS and OUTPUT id review retained batch output");return 4;}
 s->reviewed=0U;init(h,128U);TJBPUT(h+8U,s->selected);TJBPUT(h+16U,sizeof s->jobs->output);
 rc=service(s,TSA_IO_JOB_OUTPUT,h,s->jobs->output);if(rc)return fail(s,"Output unavailable",rc);n=TJBWORD(h+32U);if(n>sizeof s->jobs->output)return 12;
 /* Validate the complete envelope before presenting any output/review token. */
 while(at<n){r=s->jobs->output+at;if(n-at<24U)return 12;len=TJBWORD(r+8U);if(!len||len>132U||len>n-at-24U||TJBWORD(r)!=++sequence||!TJBWORD(r+4U)||TJBWORD(r+4U)>s->kind||!TJDNAME(r+16U))return 12;at+=24U+len;}
 s->reviewed=0U;
 for(at=0U;at<n;at+=24U+TJBWORD(s->jobs->output+at+8U)){r=s->jobs->output+at;
  rc=s->ops.present(s->ops.batch.context,r+24U,TJBWORD(r+8U));if(rc)return fail(s,"Output display failed; result retained for review",rc);
 }
 s->output_bytes=n;
 if(!n){if(TJCENC("No printed output",17U,empty))return 8;rc=s->ops.present(s->ops.batch.context,empty,17U);if(rc)return fail(s,"Output display failed; result retained for review",rc);}
 s->reviewed=s->selected;
 say(s,"Output reviewed; the next BUILD/RUN replaces this selected result");return 0;
}
static int room(TDVSTATE *s)
{
 unsigned char h[128];unsigned int i,id,rc,found=0U;rc=status(s,0U,0U,h,0);if(rc)return fail(s,"Job service unavailable",rc);
 for(i=0U;i<TJB_SLOTS;++i){id=TJBWORD(h+48U+12U*i);if(!id)continue;
  if(id!=s->selected||s->reviewed!=id||TJBWORD(h+52U+12U*i)<TJB_DONE){say(s,"Retained result needs review: OUTPUT; use JOBS for another batch job");return 16;}found=1U;
 }
 /* Only after all source/artifact/argument/JCL preflight has succeeded. */
 if(!found)s->selected=s->reviewed=0U;
 if(found&&s->selected&&s->reviewed==s->selected){init(h,128U);TJBPUT(h+8U,s->selected);rc=service(s,TSA_IO_JOB_PURGE,h,0);if(rc)return fail(s,"Selected output could not be replaced",rc);s->selected=s->reviewed=0U;}
 return 0;
}
static int card(unsigned char *cards,unsigned int *n,const char *p)
{
 unsigned int len=(unsigned int)strlen(p),at=0U,take,width;unsigned char line[80];
 do{width=at?56U:71U;take=len-at;if(take>width)take=width;
  /* Do not split a doubled quote: the converter must see its complete pair
   * while deciding whether a continuation is within a quoted parameter. */
  if(at+take<len&&p[at+take-1U]=='\''&&p[at+take]=='\'')return 8;
  if(*n>1600U-80U)return 8;memset(line,0x40,80U);
  if(at&&TJCENC("//             ",15U,line))return 8;
  if(TJCENC(p+at,take,line+(at?15U:0U)))return 8;at+=take;
  if(at<len)line[71U]=0xe7U;memcpy(cards+*n,line,80U);*n+=80U;
 }while(at<len);return 0;
}
static int plan(TDVSTATE *s,const char *member,unsigned int run,const char *args)
{
 unsigned char cards[1600];unsigned int n=0U,i,at=0U,padding,saved;char row[256],quoted[124],source[56],artifact[56];
 full(source,s->source,member);full(artifact,s->output,member);
 if(card(cards,&n,"//DEVELOP JOB"))return 8;
 sprintf(row,"// JCLLIB ORDER=%s",s->jobs->proclib);if(card(cards,&n,row))return 8;
 if(!run){sprintf(row,"//BUILD EXEC CRXCL,LIB=%s,",s->jobs->prefix);if(card(cards,&n,row))return 8;
  sprintf(row,"// OUT=%s,VOL=%s",s->work,s->jobs->volume);if(card(cards,&n,row))return 8;
  sprintf(row,"//COMP.SRC DD DSN=%s,DISP=SHR",source);if(card(cards,&n,row))return 8;
 }else{
  if(strlen(args)>60U)return 8;for(i=0U;args[i];++i){if(args[i]=='\''||args[i]=='&')quoted[at++]=args[i];quoted[at++]=args[i];}quoted[at]=0;
  /* Continuation keeps the native PARM under 100 bytes and each card <=71. */
  if(card(cards,&n,"//RUN EXEC PGM=RXVM,"))return 8;
  saved=n;
  for(padding=0U;padding<9U;++padding){n=saved;strcpy(row,"//");memset(row+2U,' ',padding+1U);
   sprintf(row+3U+padding,"PARM='--no-autoload DD:RXBIN -a %s',REGION=256M",quoted);
   if(!card(cards,&n,row))break;
  }if(padding==9U)return 8;
  if(card(cards,&n,"//SYSIN DD DUMMY")||card(cards,&n,"//SYSPRINT DD SYSOUT=*,DCB=(RECFM=FB,LRECL=132,BLKSIZE=132)")||card(cards,&n,"//SYSTERM DD SYSOUT=*,DCB=(RECFM=FB,LRECL=132,BLKSIZE=132)"))return 8;
  sprintf(row,"//RXBIN DD DSN=%s,DISP=SHR",artifact);if(card(cards,&n,row))return 8;
 }
 if(card(cards,&n,"//"))return 8;return TJFPLAN(s->jobs,0,cards,n);
}
static int probe(TDVSTATE *s,const char *id,unsigned char *info)
{memset(info,0,TEF_BYTES);return (int)file(s,TEF_LOAD,id,0,0U,info);}
static int artifact(TDVSTATE *s,const char *id,unsigned char *p,unsigned char *info,unsigned char hash[32])
{unsigned int rc;memset(info,0,TEF_BYTES);rc=file(s,TEF_LOAD,id,p,TEF_LIMIT,info);if(rc)return (int)rc;
 if(TJBWORD(info+16U)>TEF_LIMIT||TJBWORD(info+20U)!=TJD_VB||TJBWORD(info+24U)!=260U||TJBWORD(info+28U)!=264U)return 20;
 TJJHASH(p,TJBWORD(info+16U),hash);return 0;}
static int paired(const TDVSTATE *s,const unsigned char *e,const unsigned char hash[32],const unsigned char *token)
{unsigned int i;for(i=TJBWORD(e+8U);i>0U;--i)if(!memcmp(e+16U+TDV_PAIR*(i-1U)+32U,hash,32U)&&(!token||(!memcmp(e+16U+TDV_PAIR*(i-1U),token,32U)&&!memcmp(e+16U+TDV_PAIR*(i-1U)+64U,s->profile,32U))))return (int)(i-1U);return -1;}

static int execute(TDVSTATE *s,const char *member,unsigned int run,const char *args)
{
 unsigned char before[TEF_BYTES],after[TEF_BYTES],outinfo[TEF_BYTES],workinfo[TEF_BYTES],hash[32],oldhash[32],*data,*old,*e,pair[TDV_PAIR];
 char source[56],target[56];unsigned char jobhash[32];unsigned int rc,id=0U,bytes,old_exists,packed,count;int index,prior;TEFATTR attr;
 full(source,s->source,member);full(target,s->output,member);index=find(s,member);
 if(index<0){say(s,"Program not registered; EDIT the member first");return 4;}
 if(probe(s,source,before)||!(TJBWORD(before+36U)&TEF_EXISTS)||!TJBWORD(before+32U)){say(s,"Source is missing/empty; EDIT and SAVE it first");return 4;}
 data=(unsigned char *)malloc(TEF_LIMIT);old=(unsigned char *)malloc(TEF_LIMIT);if(!data||!old){free(data);free(old);return 4;}
 rc=(unsigned int)artifact(s,target,old,outinfo,oldhash);if(rc)goto done;old_exists=TJBWORD(outinfo+36U)&TEF_EXISTS;e=entry(s,(unsigned int)index);
 prior=old_exists?paired(s,e,oldhash,0):-1;
 if(old_exists&&prior<0){say(s,"Executable identity is unknown; publication/run refused");rc=16U;goto done;}
 if(run&&(!old_exists||paired(s,e,oldhash,before+104U)<0)){say(s,"Source changed or no successful build; BUILD before RUN");rc=16U;goto done;}
 rc=(unsigned int)plan(s,member,run,args);if(rc){say(s,"Request refused: check arguments/profile; prior output preserved");goto done;}
 descriptor_hash(s->jobs->result.job+32U,run?1U:3U,jobhash);
 rc=(unsigned int)room(s);if(rc)goto done;
 rc=(unsigned int)TJFPOST(s->jobs,0U,1U,&id);if(id){s->selected=id;s->reviewed=0U;s->kind=run?1U:3U;
  TJBPUT(s->catalog+130U,id);TJBPUT(s->catalog+134U,s->kind);memset(s->catalog+138U,0x40,8U);TJCENC(member,(unsigned int)strlen(member),s->catalog+138U);memcpy(s->catalog+146U,jobhash,32U);
  {int saved_status=commit_catalog(s);if(saved_status){rc=(unsigned int)saved_status;goto done;}}
 }
 if(rc){fail(s,"Submission/execution uncertain; inspect JOBS, never resubmit automatically",rc);goto done;}
 rc=(unsigned int)result(s,id,run?1U:3U);if(rc)goto done;
 if(run){say(s,"Run complete; OUTPUT shows the program result");goto done;}
 rc=(unsigned int)probe(s,source,after);if(rc||memcmp(before+104U,after+104U,32U)){say(s,"Source changed during BUILD; previous executable retained");rc=16U;goto done;}
 rc=(unsigned int)artifact(s,s->work,data,workinfo,hash);if(rc){fail(s,"Build output exceeds publication capacity or cannot be read",rc);goto done;}bytes=TJBWORD(workinfo+16U);
 if(!bytes){rc=8U;say(s,"Build produced no executable; previous executable retained");goto done;}
 attr.format=TJD_VB;attr.lrecl=260U;attr.blksize=264U;
 rc=(unsigned int)TEFPACK(&attr,data,bytes,old,TEF_LIMIT,&packed,&count);
 if(rc){say(s,"Executable exceeds 64KiB native publication capacity; previous build retained");goto done;}
 /* Recheck actual persisted artifact immediately before selecting old pair. */
 rc=(unsigned int)artifact(s,target,old,outinfo,oldhash);if(rc)goto done;old_exists=TJBWORD(outinfo+36U)&TEF_EXISTS;prior=old_exists?paired(s,e,oldhash,0):-1;
 if(old_exists&&prior<0){rc=16U;say(s,"Executable changed during BUILD; publication refused");goto done;}
 if(prior>=0)memcpy(pair,e+16U+TDV_PAIR*(unsigned int)prior,TDV_PAIR);
 memset(e+16U,0,2U*TDV_PAIR);if(prior>=0)memcpy(e+16U,pair,TDV_PAIR);
 memcpy(e+16U+(prior>=0?TDV_PAIR:0U),before+104U,32U);memcpy(e+48U+(prior>=0?TDV_PAIR:0U),hash,32U);memcpy(e+80U+(prior>=0?TDV_PAIR:0U),s->profile,32U);TJBPUT(e+8U,prior>=0?2U:1U);
 rc=(unsigned int)commit_catalog(s);if(rc)goto done;
 rc=file(s,TEF_SAVE,target,data,bytes,outinfo);
 if(rc==12U){rc=(unsigned int)artifact(s,target,old,outinfo,oldhash);if(!rc&&memcmp(oldhash,hash,32U))rc=12U;}
 if(rc){fail(s,"Executable publication failed/uncertain; inspect before retry",rc);goto done;}
 say(s,"Build complete; executable saved. OUTPUT reviews compiler diagnostics");
 done:free(data);free(old);return (int)rc;
}
int TDVKNOW(const char *command)
{char text[199],*p,*verb,*arg;if(!command||strlen(command)>=sizeof text)return 0;strcpy(text,command);p=text;verb=word(&p);if(!verb)return 0;upper(verb);
 if(!strcmp(verb,"BUILD")||!strcmp(verb,"RUN")||!strcmp(verb,"PROGRAMS")||!strcmp(verb,"CREXX"))return 1;
 if(!strcmp(verb,"OUTPUT")&&!word(&p))return 1;
 if(!strcmp(verb,"EDIT")){arg=word(&p);if(arg){upper(arg);return strcmp(arg,"HELP")&&strcmp(arg,"LINE")&&name(arg,0U);} }
 return 0;}
int TDVCMD(TDVSTATE *s,const char *command)
{
 char text[199],*p,*verb,*member,*args,source[56],line[120];unsigned int i,force_line=0U;int rc;unsigned char opened[TEF_BYTES];
 if(!s||!s->ready||!command||strlen(command)>=sizeof text)return 8;strcpy(text,command);p=text;verb=word(&p);if(!verb)return 8;upper(verb);
 if(catalog(s))return 8;
 if(!strcmp(verb,"OUTPUT")){if(word(&p))return 8;return output(s);}
 if(!strcmp(verb,"PROGRAMS")||!strcmp(verb,"CREXX")){
  if(word(&p))return 8;say(s,"cREXX Programs: EDIT member / BUILD member / RUN member [arguments] / OUTPUT");
  for(i=0U;i<s->count;++i){if(TJCDEC(entry(s,i),8U,line))return 8;line[8]=0;say(s,line);}say(s,"Capacity: two programs; source FB80; executable <=64KiB packed");return 0;
 }
 member=word(&p);if(!member)return 8;upper(member);if(!name(member,0U))return 8;while(*p==' ')++p;args=p;
 if(!strcmp(verb,"EDIT")&&*args){if(strcmp(args,"LINE")&&strcmp(args,"line"))return 8;force_line=1U;args="";}
 if(strcmp(verb,"RUN")&&*args)return 8;if(strlen(args)>60U)return fail(s,"Arguments too long",8U);
 strcpy(s->member,member);if(!strcmp(verb,"RUN"))strcpy(s->arguments,args);
 if(!strcmp(verb,"EDIT")){
  if(find(s,member)<0&&s->count==TDV_PROGRAMS){say(s,"Workspace full: two programs; existing source and builds are safe");return 4;}
  full(source,s->source,member);rc=s->ops.edit(s->ops.batch.context,source,force_line);
  if(!probe(s,source,opened)&&(TJBWORD(opened+36U)&TEF_EXISTS)){
   if(register_member(s,member)<0)return 4;say(s,"Editor returned; BUILD uses the saved source");
  }else say(s,"Editor returned; no saved source was found");
  return rc;
 }
 if(!strcmp(verb,"BUILD")||!strcmp(verb,"RUN"))return execute(s,member,!strcmp(verb,"RUN"),args);
 return 8;
}
