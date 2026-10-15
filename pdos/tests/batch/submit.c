/* SPDX-License-Identifier: MIT; controller protocol checks over real job core.
 * Native applications/devices are simulated here; guest execution is separate. */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "twospace_submit.h"
#include "twospace_abi.h"
static TJFSTATE state;
static TJBPOOL pool;
static unsigned char storage[TJB_BYTES],source[40960],config[1600];
static unsigned char expected_job[TJB_FULL_BYTES],expected_dd[TJD_EXT_BYTES];
static unsigned int source_bytes,config_bytes,submitted,runs,prints,records,opened,closed,read_at;
static unsigned int printer_failure,short_reader,bad_site,input_at,input_count;
static const char *input[32];
static char messages[32768];
static void card(unsigned char *p,unsigned int *used,const char *text)
{unsigned int n=(unsigned int)strlen(text);assert(n<=80U);memset(p+*used,0x40,80U);assert(!TJCENC(text,n,p+*used));*used+=80U;}
static void setup(void)
{
 source_bytes=config_bytes=submitted=runs=prints=records=opened=closed=read_at=0U;
 printer_failure=short_reader=bad_site=input_at=input_count=0U;messages[0]=0;assert(!TJBINIT(&pool,storage,sizeof storage));
 card(config,&config_bytes,"PD033 SITE 1");card(config,&config_bytes,"LOADLIB SITE.LOAD");card(config,&config_bytes,"PROCLIB SITE.PROCLIB");
 card(config,&config_bytes,"LIBPREFIX LABA01.B33");card(config,&config_bytes,"VOLUME PDOS00");
 card(config,&config_bytes,"RXC BRXC");card(config,&config_bytes,"RXAS BRXAS");card(config,&config_bytes,"RXLINK BRXLINK");card(config,&config_bytes,"RXVM BRXVM");
 card(source,&source_bytes,"//CASE JOB CLASS=A,MSGCLASS=A");card(source,&source_bytes,"//LIBS JCLLIB ORDER=SITE.PROCLIB");
 card(source,&source_bytes,"//JOBLIB DD DSN=SITE.LOAD,DISP=SHR");card(source,&source_bytes,"//BUILD EXEC CRXCG,LIB=LABA01.B33,VOL=PDOS00");
 card(source,&source_bytes,"//COMP.SRC DD DSN=APP.SRC(MAIN),DISP=SHR");card(source,&source_bytes,"//");
}
static int load(void *ctx,const char *name,unsigned char *data,unsigned int cap,unsigned int *used)
{
 unsigned int n=0U;char path[128],line[128];FILE *f;const char *open;(void)ctx;
 if(!strcmp(name,"PDOS.BATCH.SITE")){if(cap<config_bytes)return 4;memcpy(data,config,config_bytes);*used=config_bytes;return 0;}
 if(!strcmp(name,"APP.JCL(CASE)")){if(cap<source_bytes)return 4;memcpy(data,source,source_bytes);*used=source_bytes;return 0;}
 open=strchr(name,'(');if(strncmp(name,"SITE.PROCLIB(",13U)||!open)return 4;
 strcpy(path,"pdos/src/procs/");strncat(path,open+1U,strlen(open+1U)-1U);strcat(path,".jcl");
 f=fopen(path,"r");if(!f)return 4;
 while(fgets(line,sizeof line,f)){unsigned int len=(unsigned int)strlen(line);assert(len&&line[len-1U]=='\n');line[len-1U]=0;
  if(n+80U>cap){fclose(f);return 4;}card(data,&n,line);}
 fclose(f);*used=n;return 0;
}
static void message(void *ctx,const char *text)
{(void)ctx;assert(strlen(messages)+strlen(text)+2U<sizeof messages);strcat(messages,text);strcat(messages,"\n");}
static void record(void *ctx,const unsigned char *p,unsigned int n)
{unsigned char expected[9];(void)ctx;++records;assert(n==9U&&!TJCENC("RESULT 42",9U,expected)&&!memcmp(p,expected,9U));}
static int console(void *ctx,unsigned char *data,unsigned int cap,unsigned int *used)
{unsigned int n;(void)ctx;if(input_at==input_count)return 12;n=(unsigned int)strlen(input[input_at]);assert(n<=cap);
 assert(!TJCENC(input[input_at++],n,data));*used=n;return 0;}
static void status_header(unsigned char *h,const unsigned char *j)
{TJBPUT(h+8U,TJBWORD(j));memcpy(h+40U,j,88U);}
static unsigned int service(void *ctx,unsigned int op,unsigned int bytes,unsigned char *h,void *data)
{
 unsigned int id=TJBWORD(h+8U),rc=0U,i;unsigned char *j;char dsn[45],member[9];(void)ctx;
 assert(TJBWORD(h)==1U&&TJBWORD(h+4U)==bytes);
 if(op==TSA_IO_JOB_QUERY){unsigned int length=44U,organisation=0x0200U,fmt=TJD_FB,lr=80U,bl=3120U;
  while(length&&h[40U+length-1U]==0x40U)--length;assert(!TJCDEC(h+40U,length,dsn));dsn[length]=0;
  if(h[84U]){length=8U;while(length&&h[84U+length-1U]==0x40U)--length;assert(!TJCDEC(h+84U,length,member));member[length]=0;}else member[0]=0;
  if(!strcmp(dsn,"PDOS.BATCH.SITE")){if(bad_site)return 4;organisation=0x4000U;}
  else if(!strncmp(dsn,"TSO31.BR",8U)){organisation=0x4000U;fmt=0x80U;lr=bl=18452U;}
  else if(!strcmp(dsn,"LABA01.B33.RXBIN")){fmt=TJD_VB;lr=260U;bl=2640U;}
  else if(strcmp(dsn,"APP.JCL")&&strcmp(dsn,"APP.SRC")&&strcmp(dsn,"SITE.PROCLIB"))return 4;
  TJBPUT(h+96U,organisation);TJBPUT(h+100U,fmt);TJBPUT(h+104U,lr);TJBPUT(h+108U,bl);TJBPUT(h+112U,1U);TJCENC("PDOS00",6U,h+116U);return 0;
 }
 if(op==TSA_IO_BATCH_OPEN){++opened;TJBPUT(h+8U,TJBWORD(h+12U));return 0;}
 if(op==TSA_IO_BATCH_CLOSE){++closed;return 0;}
 if(op==TSA_IO_BATCH_READ){assert(bytes==64U&&id==1U&&data&&TJBWORD(h+24U)==80U);
  if(short_reader){TJBPUT(h+28U,79U);return 0;}
  if(read_at==source_bytes){TJBPUT(h+28U,0U);TJBPUT(h+60U,1U);return 0;}
  memcpy(data,source+read_at,80U);read_at+=80U;TJBPUT(h+28U,80U);return 0;
 }
 if(op==TSA_IO_JOB_SUBMIT){const unsigned char *p=(const unsigned char *)data;
  assert(TJBWORD(h+12U)==1U&&TJBWORD(h+16U)==TJB_FULL_BYTES+TJD_EXT_BYTES);
  rc=(unsigned int)TJBCHECK(p,TJB_FULL_BYTES);if(!rc)rc=(unsigned int)TJDCHK(p+TJB_FULL_BYTES,TJBWORD(p+16U));if(rc)return rc;
  rc=(unsigned int)TJBSUB(&pool,7U,p,TJB_FULL_BYTES,&id);if(rc)return rc;assert(!TJBBIND(&pool,7U,id,p+TJB_FULL_BYTES,TJD_EXT_BYTES));++submitted;
  j=TJBFIND(&pool,7U,id);status_header(h,j);return 0;
 }
 if(op==TSA_IO_JOB_STATUS&&!id){for(i=40U;i<128U;++i)h[i]=0;
  for(i=0U;i<TJB_SLOTS;++i){j=storage+TJB_BASE+i*TJB_SLOT_BYTES;TJBPUT(h+48U+12U*i,TJBWORD(j));TJBPUT(h+52U+12U*i,TJBWORD(j+8U));TJBPUT(h+56U+12U*i,TJBWORD(j+28U));}return 0;}
 j=TJBFIND(&pool,7U,id);if(!j)return 8;
 if(op==TSA_IO_JOB_RUN){rc=(unsigned int)TJBSTART(&pool,7U,id);if(rc)return rc;++runs;
  for(;;){unsigned char dd[8],payload[9];int next=TJBNEXT(&pool,7U,id);if(next==1)break;assert(!next);
   memset(dd,0x40,8U);TJCENC("SYSPRINT",8U,dd);TJCENC("RESULT 42",9U,payload);
   assert(!TJBAPP(&pool,7U,id,dd,9U,payload,9U)&&!TJBEND(&pool,7U,id,0U,1U,0U,0U,0U));}
 }else if(op==TSA_IO_JOB_SOURCE){assert(data&&TJBWORD(h+16U)==TJB_META_BYTES);memcpy(data,j+TJB_META_BASE,TJB_META_BYTES);}
 else if(op==TSA_IO_JOB_STATUS){if(TJBWORD(h+12U)){assert(data&&TJBWORD(h+16U)==TJB_STEP_BYTES);memcpy(data,TJBSTEP(j,TJBWORD(h+12U)-1U),TJB_STEP_BYTES);}}
 else if(op==TSA_IO_JOB_OUTPUT){i=TJBWORD(j+24U);assert(data&&i<=TJBWORD(h+16U));memcpy(data,TJBOUT(&pool,id),i);TJBPUT(h+32U,i);}
 else if(op==TSA_IO_JOB_PRINT){++prints;TJBPUT(j+28U,printer_failure?TJB_PRINT_FAILED:TJB_PRINT_DONE);if(printer_failure)return 12;}
 else if(op>=TSA_IO_JOB_HOLD&&op<=TSA_IO_JOB_PURGE){rc=(unsigned int)TJBACT(&pool,7U,id,op-TSA_IO_JOB_HOLD+1U);if(rc)return rc;}
 else assert(0);
 status_header(h,j);return 0;
}
int main(void)
{
 TJFOPS ops;unsigned int before,before_print;memset(&ops,0,sizeof ops);ops.load=load;ops.service=service;ops.input=console;ops.line=message;ops.record=record;
 assert(offsetof(TJCRESULT,dd)==offsetof(TJCRESULT,job)+TJB_FULL_BYTES);
 assert(TJFKNOW("submit app.jcl(case)")&&TJFKNOW("HELP BATCH")&&!TJFKNOW("EDIT APP.SRC"));
 setup();assert(!TJFCMD(&state,&ops,"SUBMIT APP.JCL(CASE) RUNHOLD")&&submitted==1U&&!runs&&!opened);
 memcpy(expected_job,state.result.job,sizeof expected_job);memcpy(expected_dd,state.result.dd,sizeof expected_dd);
 assert(!TJFCMD(&state,&ops,"JOBS")&&strstr(messages,"STATE=2"));assert(!TJFCMD(&state,&ops,"RELEASE 1")&&runs==1U&&prints==1U&&opened==closed);
 assert(!TJFCMD(&state,&ops,"STATUS 1")&&strstr(messages,"BUILD.ASSEM"));
 assert(!TJFCMD(&state,&ops,"OUTPUT 1 STEP=BUILD.RUN DD=SYSPRINT")&&records==1U);
 assert(TJFCMD(&state,&ops,"OUTPUT 1 STEP=TYPO")==8);assert(!TJFCMD(&state,&ops,"PURGE 1"));
 read_at=0U;assert(!TJFCMD(&state,&ops,"SUBMIT READER RUNHOLD")&&submitted==2U&&opened==closed);
 assert(!memcmp(expected_job,state.result.job,sizeof expected_job)&&!memcmp(expected_dd,state.result.dd,sizeof expected_dd));
 assert(!TJFCMD(&state,&ops,"CANCEL 2")&&!TJFCMD(&state,&ops,"PURGE 2"));
 short_reader=1U;before=submitted;assert(TJFCMD(&state,&ops,"SUBMIT READER")&&submitted==before&&opened==closed);short_reader=0U;
 /* Console records are the same complete deck and require a separate END. */
 input[0]="//CASE JOB CLASS=A,MSGCLASS=A";input[1]="//LIBS JCLLIB ORDER=SITE.PROCLIB";
 input[2]="//JOBLIB DD DSN=SITE.LOAD,DISP=SHR";input[3]="//BUILD EXEC CRXCG,LIB=LABA01.B33,VOL=PDOS00";
 input[4]="//COMP.SRC DD DSN=APP.SRC(MAIN),DISP=SHR";input[5]="//";input[6]="END PROOF";input_count=7U;input_at=0U;
 assert(!TJFCMD(&state,&ops,"SUBMIT * PROOF RUNHOLD")&&input_at==7U&&submitted==3U);
 assert(!memcmp(expected_job,state.result.job,sizeof expected_job)&&!memcmp(expected_dd,state.result.dd,sizeof expected_dd));
 assert(!TJFCMD(&state,&ops,"CANCEL 3")&&!TJFCMD(&state,&ops,"PURGE 3"));
 input_at=0U;input_count=6U;before=submitted;assert(TJFCMD(&state,&ops,"SUBMIT * PROOF")&&submitted==before&&input_at==6U);
 input_at=0U;input_count=7U;bad_site=1U;assert(TJFCMD(&state,&ops,"SUBMIT * PROOF")&&submitted==before&&input_at==7U);bad_site=0U;
 assert(strstr(messages,"SITE catalog query RC=4"));
 input[0]="ABORT PROOF";input_count=1U;input_at=0U;assert(TJFCMD(&state,&ops,"SUBMIT * PROOF")==16&&submitted==before);
 printer_failure=1U;assert(TJFCMD(&state,&ops,"SUBMIT APP.JCL(CASE)")==12&&runs==2U&&opened==closed);
 assert(TJBWORD(TJBFIND(&pool,7U,4U)+8U)==TJB_DONE&&TJBWORD(TJBFIND(&pool,7U,4U)+28U)==TJB_PRINT_FAILED);
 printer_failure=0U;assert(!TJFCMD(&state,&ops,"PRINT 4")&&runs==2U&&!TJFCMD(&state,&ops,"PURGE 4"));
 /* Native-style HOLD retains output, while RUNHOLD delays execution. */
 before=runs;before_print=prints;
 assert(!TJFCMD(&state,&ops,"SUBMIT APP.JCL(CASE) HOLD")&&runs==before+1U&&prints==before_print);
 assert(TJBWORD(TJBFIND(&pool,7U,5U)+8U)==TJB_DONE&&TJBWORD(TJBFIND(&pool,7U,5U)+28U)==TJB_PRINT_PENDING);
 assert(!TJFCMD(&state,&ops,"STATUS 5")&&strstr(messages,"OUTPUT POLICY=HELD"));
 assert(!TJFCMD(&state,&ops,"PRINT 5")&&prints==before_print+1U&&!TJFCMD(&state,&ops,"PURGE 5"));
 before=runs;before_print=prints;
 assert(!TJFCMD(&state,&ops,"SUBMIT APP.JCL(CASE) HOLD RUNHOLD")&&runs==before&&prints==before_print);
 assert(!TJBLOAD(&pool,storage,sizeof storage));
 assert(!TJFCMD(&state,&ops,"RELEASE 6")&&runs==before+1U&&prints==before_print);
 assert(TJBWORD(TJBFIND(&pool,7U,6U)+TJB_META_BASE+228U)==TJB_OUTPUT_HOLD);
 assert(!TJFCMD(&state,&ops,"PRINT 6")&&prints==before_print+1U&&!TJFCMD(&state,&ops,"PURGE 6"));
 card(source,&source_bytes,"//EXTRA EXEC PGM=RXVM");before=submitted;assert(TJFCMD(&state,&ops,"SUBMIT APP.JCL(CASE)")&&submitted==before);
 puts("PD033 console/reader identity, framing, controls and print recovery PASS");return 0;
}
