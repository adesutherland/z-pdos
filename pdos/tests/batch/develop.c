/* SPDX-License-Identifier: MIT; real controller/converter/job-core fault proof.
 * Only native tool execution and compare/save transport are simulated. */
#define main submit_reference_main
#include "submit.c"
#undef main
#include "twospace_develop.h"
#include "twospace_digest.h"
static TDVSTATE dev;
static TDVOPS devops;
typedef struct {char name[56];unsigned char bytes[TEF_LIMIT];unsigned int used,exists;} TESTFILE;
static TESTFILE files[6];
static unsigned int profile_tag,display_failure,bad_dev_format;
static unsigned int save_fault,save_after,save_target,change_source,compile_failure,large_output,version_byte;
static unsigned int devfile(void *ctx,unsigned int action,const char *name,unsigned char *data,unsigned int cap,unsigned char *info)
{
 unsigned int i,rc=0U,n;unsigned char token[32];TESTFILE *f;(void)ctx;
 for(i=0U;i<6U&&strcmp(files[i].name,name);++i){}assert(i<6U);f=files+i;TJJHASH(f->bytes,f->used,token);
 if(action!=TEF_LOAD){if(memcmp(token,info+104U,32U))return 16U;
  if(save_fault&&i==save_target&&!save_after)return save_fault;
  assert(cap<=sizeof f->bytes);memcpy(f->bytes,data,cap);f->used=cap;f->exists=1U;TJJHASH(f->bytes,f->used,token);
  if(save_fault&&i==save_target)rc=save_fault;
 }else if(cap){if(f->used>cap)return 4U;memcpy(data,f->bytes,f->used);}
 memset(info,0,TEF_BYTES);TJBPUT(info+16U,f->used);TJBPUT(info+20U,i==0U?TJD_FB:TJD_VB);TJBPUT(info+24U,i==0U?80U:260U);TJBPUT(info+28U,i==0U?3120U:264U);
 for(i=0U,n=0U;i+2U<=f->used;++n)i+=2U+((unsigned int)f->bytes[i]<<8)+f->bytes[i+1U];
 TJBPUT(info+32U,n);TJBPUT(info+36U,f->exists?TEF_EXISTS:0U);memcpy(info+104U,token,32U);return rc;
}
static unsigned int devservice(void *ctx,unsigned int op,unsigned int bytes,unsigned char *h,void *data)
{
 unsigned int rc,n,org,fmt,lr,bl;char dsn[45];
 if(op==TSA_IO_JOB_QUERY){n=44U;while(n&&h[40U+n-1U]==0x40U)--n;assert(!TJCDEC(h+40U,n,dsn));dsn[n]=0;
  org=0x0200U;fmt=TJD_FB;lr=80U;bl=3120U;
  if(!strcmp(dsn,"PDOS.DEV.SITE")){org=0x4000U;if(bad_dev_format)fmt=TJD_VB;}
  else if(!strcmp(dsn,"APP.RXBIN")){fmt=TJD_VB;lr=260U;bl=264U;}
  else if(!strcmp(dsn,"APP.WORK")||!strcmp(dsn,"APP.STATE")){org=0x4000U;fmt=TJD_VB;lr=260U;bl=264U;}
  else return service(ctx,op,bytes,h,data);
  TJBPUT(h+96U,org);TJBPUT(h+100U,fmt);TJBPUT(h+104U,lr);TJBPUT(h+108U,bl);TJCENC("PDOS00",6U,h+116U);return 0U;
 }
 rc=service(ctx,op,bytes,h,data);
 if(!rc&&op==TSA_IO_JOB_RUN){unsigned char *j=TJBFIND(&pool,7U,TJBWORD(h+8U));assert(j);
  if(TJBWORD(j+16U)==3U){files[2].used=large_output?254U*258U:5U;files[2].exists=1U;files[2].bytes[0]=0U;files[2].bytes[1]=3U;files[2].bytes[2]=version_byte;files[2].bytes[3]=0x22U;files[2].bytes[4]=0x33U;
   if(large_output){unsigned int block;for(block=0U;block<254U;++block){files[2].bytes[block*258U]=1U;files[2].bytes[block*258U+1U]=0U;memset(files[2].bytes+block*258U+2U,0x22,256U);}}
   if(compile_failure){TJBPUT(TJBSTEP(j,0U)+132U,8U);TJBPUT(j+8U,TJB_FAILED);}
   if(change_source)files[0].bytes[2]^=1U;
  }
 }return rc;
}
static int devload(void *ctx,const char *name,unsigned char *p,unsigned int cap,unsigned int *used)
{
 if(!strcmp(name,"PDOS.DEV.SITE")){unsigned int n=0U;assert(cap>=560U);card(p,&n,"PD035 DEV 1");card(p,&n,"SOURCE APP.SRC");card(p,&n,"OUTPUT APP.RXBIN");card(p,&n,"WORK APP.WORK");card(p,&n,"STATE APP.STATE");card(p,&n,profile_tag?"PROFILE 1123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef":"PROFILE 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");*used=n;return 0;}
 return load(ctx,name,p,cap,used);
}
static unsigned int devpresent(void *ctx,const unsigned char *p,unsigned int n)
{if(display_failure)return display_failure;if(n==9U)record(ctx,p,n);else{unsigned char empty[17];assert(n==17U&&!TJCENC("No printed output",17U,empty)&&!memcmp(p,empty,17U));}return 0U;}
static int devedit(void *ctx,const char *name,unsigned int force_line){(void)ctx;(void)force_line;assert(strstr(name,"APP.SRC("));if(!strcmp(name,"APP.SRC(SECOND)"))files[4].exists=1U;return 0;}
static void fresh(void)
{
 unsigned int n=0U;setup();memset(&dev,0,sizeof dev);memset(files,0,sizeof files);memset(&devops,0,sizeof devops);
 strcpy(files[0].name,"APP.SRC(HELLO)");strcpy(files[1].name,"APP.RXBIN(HELLO)");strcpy(files[2].name,"APP.WORK");strcpy(files[3].name,"APP.STATE");strcpy(files[4].name,"APP.SRC(SECOND)");strcpy(files[5].name,"APP.RXBIN(SECOND)");
 files[0].exists=files[2].exists=files[3].exists=1U;files[0].used=82U;files[0].bytes[1]=80U;memset(files[0].bytes+2U,0x40,80U);
 config_bytes=0U;card(config,&config_bytes,"PD033 SITE 1");card(config,&config_bytes,"LOADLIB SITE.LOAD");card(config,&config_bytes,"PROCLIB SITE.PROCLIB");card(config,&config_bytes,"LIBPREFIX LABA01.B33");card(config,&config_bytes,"VOLUME PDOS00");card(config,&config_bytes,"RXC BRXC");card(config,&config_bytes,"RXAS BRXAS");card(config,&config_bytes,"RXLINK BRXLINK");card(config,&config_bytes,"RXVM BRXVM");
 (void)n;save_fault=save_after=save_target=change_source=compile_failure=large_output=0U;version_byte=1U;profile_tag=display_failure=bad_dev_format=0U;
 devops.batch.load=devload;devops.batch.service=devservice;devops.batch.input=console;devops.batch.line=message;devops.batch.record=record;devops.file=devfile;devops.edit=devedit;devops.present=devpresent;
 assert(!TDVCONFIG(&dev,&state,&devops));assert(!TDVCMD(&dev,"EDIT HELLO"));
}
static void review(void){assert(!TDVCMD(&dev,"OUTPUT"));}
static void restart(void){memset(&dev,0,sizeof dev);assert(!TDVCONFIG(&dev,&state,&devops));}
int main(void)
{
 unsigned int before,n;unsigned char saved[5];char parm[101],purge[40];
 fresh();strcpy(dev.arguments,"one two");assert(!TDVCMD(&dev,"EDIT HELLO")&&!strcmp(dev.arguments,"one two"));{int result=TDVCMD(&dev,"BUILD HELLO");if(result)fprintf(stderr,"%s",messages);assert(!result);}assert(!strcmp(dev.arguments,"one two"));assert(files[1].exists&&files[1].used==5U&&submitted==1U);memcpy(saved,files[1].bytes,5U);
 display_failure=12U;assert(TDVCMD(&dev,"OUTPUT")==12&&!dev.reviewed);display_failure=0U;
 before=submitted;assert(TDVCMD(&dev,"RUN HELLO one two")==16&&submitted==before);review();assert(!TDVCMD(&dev,"RUN HELLO one two"));review();
 /* Syntax/admission errors preserve even reviewed prior output. */
 before=dev.selected;assert(TDVCMD(&dev,"RUN HELLO 1234567890123456789012345678901234567890123456789012345678901")&&dev.selected==before);assert(!TDVCMD(&dev,"EDIT SECOND"));assert(TDVCMD(&dev,"EDIT THIRD"));
 files[0].bytes[2]^=1U;before=submitted;assert(TDVCMD(&dev,"RUN HELLO")==16&&submitted==before);
 compile_failure=1U;assert(TDVCMD(&dev,"BUILD HELLO")&&!memcmp(saved,files[1].bytes,5U));compile_failure=0U;review();
 /* Equal executable bytes with two different source tokens must both match. */
 assert(!TDVCMD(&dev,"BUILD HELLO"));review();assert(!TDVCMD(&dev,"RUN HELLO"));review();
 change_source=1U;assert(TDVCMD(&dev,"BUILD HELLO")==16&&!memcmp(saved,files[1].bytes,5U));change_source=0U;review();
 /* Registry failure before commit: no executable publication. */
 version_byte=2U;save_target=3U;save_fault=12U;save_after=0U;assert(TDVCMD(&dev,"BUILD HELLO")==12&&!memcmp(saved,files[1].bytes,5U));review();
 save_after=1U;assert(!TDVCMD(&dev,"BUILD HELLO")&&files[1].bytes[2]==2U);review();save_fault=0U;
 /* Artifact failure before/after commit, restart, and A+B/artifact-A retry. */
 version_byte=3U;save_target=1U;save_fault=12U;save_after=0U;assert(TDVCMD(&dev,"BUILD HELLO")==12&&files[1].bytes[2]==2U);review();
 restart();assert(dev.selected==9U&&!dev.reviewed);before=submitted;assert(TDVCMD(&dev,"BUILD HELLO")==16&&submitted==before);review();
 save_fault=0U;version_byte=4U;assert(!TDVCMD(&dev,"BUILD HELLO")&&files[1].bytes[2]==4U);review();
 save_fault=12U;save_after=1U;version_byte=5U;assert(!TDVCMD(&dev,"BUILD HELLO")&&files[1].bytes[2]==5U);review();save_fault=0U;
 /* Malformed registry is rejected before admission/mutation. */
 files[3].bytes[1]^=1U;before=submitted;assert(TDVCMD(&dev,"BUILD HELLO")&&submitted==before);files[3].bytes[1]^=1U;
 /* Full STORE is surfaced; existing output and executable survive. */
 save_target=3U;save_fault=4U;assert(TDVCMD(&dev,"BUILD HELLO")==4&&files[1].bytes[2]==5U);review();save_fault=0U;
 /* Oversized packed output: no metadata or executable update. */
 large_output=1U;n=files[3].used;before=files[3].bytes[200U];assert(TDVCMD(&dev,"BUILD HELLO")&&files[1].bytes[2]==5U&&files[3].used==n&&files[3].bytes[200U]==before);
 /* A normal tool-profile update preserves names/source/artifact but forces
  * a new build; the old matching artifact remains a publication fallback. */
 fresh();assert(!TDVCMD(&dev,"BUILD HELLO"));review();profile_tag=1U;
 assert(!TDVCONFIG(&dev,&state,&devops)&&dev.count==1U);before=submitted;
 assert(TDVCMD(&dev,"RUN HELLO")==16&&submitted==before);
 assert(!TDVCMD(&dev,"BUILD HELLO"));review();assert(!TDVCMD(&dev,"RUN HELLO"));
 review();assert(!TDVCMD(&dev,"RUN HELLO 123456789012345678901234567890123456789012345678901234567890"));
 n=TJBWORD(state.result.job+48U);assert(n<sizeof parm);assert(!TJCDEC(state.result.job+56U,n,parm));parm[n]=0;
 assert(!strcmp(parm,"--no-autoload DD:RXBIN -a 123456789012345678901234567890123456789012345678901234567890"));
 review();assert(!TDVCMD(&dev,"RUN HELLO A&B O'Brien"));n=TJBWORD(state.result.job+48U);assert(n<sizeof parm);assert(!TJCDEC(state.result.job+56U,n,parm));parm[n]=0;
 assert(!strcmp(parm,"--no-autoload DD:RXBIN -a A&B O'Brien"));
 /* Prefer the most recent complete matching identity when A and B produced
  * equal bytes, then C's publication fails. B must remain runnable. */
 fresh();assert(!TDVKNOW("EDIT LINE")&&!TDVKNOW("EDIT HELP"));assert(!TDVCMD(&dev,"EDIT HELLO LINE"));
 assert(!TDVCMD(&dev,"BUILD HELLO"));review();files[0].bytes[2]=0x41U;assert(!TDVCMD(&dev,"BUILD HELLO"));review();
 files[0].bytes[2]=0x42U;version_byte=2U;save_fault=12U;save_target=1U;save_after=0U;assert(TDVCMD(&dev,"BUILD HELLO")==12);review();
 files[0].bytes[2]=0x41U;save_fault=0U;assert(!TDVCMD(&dev,"RUN HELLO"));
 review();sprintf(purge,"PURGE %u",dev.selected);assert(!TJFCMD(&state,&devops.batch,purge));assert(!TDVCMD(&dev,"RUN HELLO"));
 /* An unrelated completed job remains unselected and cannot be purged by
  * a developer command solely because it occupies the global completed slot. */
 memset(files[3].bytes+130U,0,48U);restart();before=submitted;
 assert(!dev.selected&&TDVCMD(&dev,"RUN HELLO")==16&&submitted==before&&TDVCMD(&dev,"OUTPUT")==4);
 fresh();assert(!TDVCMD(&dev,"BUILD HELLO"));TJBPUT(TJBFIND(&pool,7U,dev.selected)+24U,0U);
 display_failure=12U;assert(TDVCMD(&dev,"OUTPUT")==12&&!dev.reviewed);display_failure=0U;review();
 puts("PD035 controller: build/run, stale guard, publication/restart faults, capacity and output ownership PASS");return 0;
}
