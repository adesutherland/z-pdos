/* SPDX-License-Identifier: MIT; independent common-PROC and recovery controls. */
#include <stdio.h>
#include <string.h>
#include "twospace_jcl.h"
#include "twospace_digest.h"
static TJCWORK work;
static TJCRESULT result;
static TJCERROR error;
static unsigned char deck[40960],procs[4][40960],custom[40960];
static unsigned int cards,custom_bytes,queries,checks;
static const char *custom_name;
#define CHECK(x) do {++checks;if(!(x)){printf("PROC FAIL %u card %u column %u reason %u\n",(unsigned int)__LINE__,error.card,error.column,error.reason);return 1;}} while(0)
static void card_to(unsigned char *p,unsigned int *n,const char *text)
{unsigned int len=(unsigned int)strlen(text);memset(p+80U*(*n),0x40,80U);TJCENC(text,len,p+80U*(*n));++*n;}
static void card(const char *p){card_to(deck,&cards,p);}
static void begin(void)
{cards=queries=0U;custom_name=0;card("//CASE JOB CLASS=A,MSGCLASS=A");card("//LIBS JCLLIB ORDER=SITE.PROCLIB");card("//JOBLIB DD DSN=SITE.LOAD,DISP=SHR");}
static int program(void *context,const TJCSTEP *s,unsigned char out[8])
{(void)context;++queries;if(s->library_count!=1U||strcmp(s->libraries[0].dsn,"SITE.LOAD"))return 8;
 memset(out,0x40,8U);return TJCENC(s->program,(unsigned int)strlen(s->program),out);}
static int dd(void *context,const TJCSTEP *s,const TJCDECL *d,TJCATTR *a)
{
 (void)context;++queries;
 if(!a->direction){a->direction=TJD_INPUT;
  if(!strcmp(d->name,"SYSPRINT")||!strcmp(d->name,"SYSTERM")||
   (!strcmp(s->program,"RXC")&&!strcmp(d->name,"ASM"))||
   (!strcmp(s->program,"RXAS")&&!strcmp(d->name,"SYSGO"))||
   (!strcmp(s->program,"RXLINK")&&!strcmp(d->name,"RXBIN")))a->direction=TJD_OUTPUT;}
 if(!a->recfm)a->recfm=d->kind==TJD_DATASET&&strcmp(d->name,"SRC")?TJD_VB:TJD_FB;
 if(!a->lrecl)a->lrecl=d->kind==TJD_SYSOUT?132U:a->recfm==TJD_VB?260U:80U;
 if(!a->blksize)a->blksize=a->recfm==TJD_VB?264U:a->lrecl;return 0;
}
static int procedure(void *context,const char *name,const TJCLIB *libs,unsigned int count,
                       const unsigned char **data,unsigned int *bytes)
{
 static const char *names[]={"CRXC","CRXCG","CRXCL","CRXLGO"};
 unsigned int i,n=0U;char path[128],line[128];FILE *f;(void)context;
 if(count!=1U||strcmp(libs[0].dsn,"SITE.PROCLIB"))return 8;
 if(custom_name&&!strcmp(name,custom_name)){*data=custom;*bytes=custom_bytes;return 0;}
 for(i=0U;i<4U&&strcmp(names[i],name);++i){}if(i==4U)return 8;
 strcpy(path,"pdos/src/procs/");strcat(path,name);strcat(path,".jcl");f=fopen(path,"r");if(!f)return 8;
 while(fgets(line,sizeof line,f)){unsigned int len=(unsigned int)strlen(line);if(!len||line[len-1U]!='\n'||len>72U||n>=512U){fclose(f);return 8;}
  line[len-1U]=0;card_to(procs[i],&n,line);}
 if(ferror(f)){fclose(f);return 8;}fclose(f);*data=procs[i];*bytes=n*80U;return 0;
}
static int convert(void)
{TJCSITE site;memset(&site,0,sizeof site);site.program=program;site.dd=dd;site.procedure=procedure;
 return TJCCONV(&work,deck,cards*80U,&site,&result,&error);}
static void custom_proc(const char *name)
{custom_name=name;custom_bytes=0U;}
static void pc(const char *p)
{unsigned int n=custom_bytes/80U;card_to(custom,&n,p);custom_bytes=n*80U;}
static void source_override(void)
{card("//COMP.SRC DD *");card("options levelb");card("say 'STAGE4'");card("/*");}
static int bad(unsigned int reason)
{return convert()!=0&&error.reason==reason&&!result.valid&&!queries;}
static int digest(const unsigned char *p,unsigned int n,const char *expected)
{unsigned char hash[32];char text[65];unsigned int i;static const char hex[]="0123456789abcdef";
 if(TJJHASH(p,n,hash))return 0;for(i=0U;i<32U;++i){text[2U*i]=hex[hash[i]>>4];text[2U*i+1U]=hex[hash[i]&15U];}text[64]=0;return !strcmp(text,expected);}
int main(void)
{
 unsigned int i;char parm[101];unsigned char abc[]={0x61U,0x62U,0x63U};
 CHECK(digest(0,0U,"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
 CHECK(digest(abc,3U,"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
 memset(custom,0x61,100000U>sizeof custom?sizeof custom:100000U);
 CHECK(digest(custom,64U,"ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb"));
 for(i=0U;i<4U;++i){static const char *calls[]={
  "//BUILD EXEC CRXC,LIB=LABA01.B33,VOL=PDOS00,OUT=APP.OBJ",
  "//BUILD EXEC CRXCG,LIB=LABA01.B33,VOL=PDOS00,ARGS='-a SHARED'",
  "//BUILD EXEC CRXCL,LIB=LABA01.B33,VOL=PDOS00,OUT=APP.LINK",
  "//BUILD EXEC CRXLGO,LIB=LABA01.B33,VOL=PDOS00,ARGS='-a SHARED'"};
  static const unsigned int steps[]={2U,3U,3U,4U},dds[]={10U,15U,16U,20U};
  begin();card(calls[i]);source_override();card("//");CHECK(!convert());
  CHECK(result.valid&&result.steps==steps[i]&&result.dds==dds[i]);
  CHECK(!strcmp(result.step[0].name,"BUILD.COMP")&&!strcmp(result.step[1].name,"BUILD.ASSEM"));
  CHECK(TJBWORD(result.job)==2U&&!TJBCHECK(result.job,TJB_FULL_BYTES)&&!TJDCHK(result.dd,result.steps));
  CHECK(TJBWORD(result.dd)==3U&&TJBWORD(result.job+TJB_DESC_BYTES+224U)==1U);
  CHECK(TJBWORD(result.dd+32U+4U*TJD_ENTRY+24U)==1133280U);
  CHECK(!TJCDEC(result.job+56U,TJBWORD(result.job+48U),parm));parm[TJBWORD(result.job+48U)]=0;
  CHECK(!strcmp(parm,"-n -i LABA01.B33 -o DD:ASM DD:SRC"));
 }
 begin();card("// SET FLAGS='-x'");card("//BUILD EXEC CRXCG,LIB=LABA01.B33,VOL=PDOS00,FLAGS='-n'");source_override();card("//AFTER EXEC PGM=RXVM,PARM='&FLAGS'");card("//");
 CHECK(!convert()&&result.steps==4U);
 CHECK(TJBWORD(result.job+32U+3U*TJB_STEP_BYTES+16U)==2U&&result.job[32U+3U*TJB_STEP_BYTES+25U]==0xa7U);
 begin();card("//BUILD EXEC CRXCG,LIB=LABA01.B33,VOL=PDOS00,PARM.RUN='-v'");source_override();card("//");CHECK(!convert());
 CHECK(TJBWORD(result.job+32U+2U*TJB_STEP_BYTES+16U)==2U);
 begin();card("//BUILD EXEC CRXCG,LIB=LABA01.B33,VOL=PDOS00,TYPO=YES");card("//");CHECK(bad(TJC_SYMBOL));
 begin();card("//BUILD EXEC CRXCG,LIB=LABA01.B33,VOL=PDOS00");card("//RUN.SYSIN DD DUMMY");card("//COMP.SRC DD DUMMY");card("//");CHECK(bad(TJC_SYNTAX));
 begin();card("//BUILD EXEC CRXCG,LIB=LABA01.B33,VOL=PDOS00");card("//NOTHERE.SRC DD DUMMY");card("//");CHECK(bad(TJC_SYNTAX));
 begin();card("//BUILD EXEC MISSING");card("//");CHECK(bad(TJC_RESOLVE));
 begin();custom_proc("CYCLE");pc("//CYCLE PROC");pc("//AGAIN EXEC CYCLE");pc("// PEND");card("//BUILD EXEC CYCLE");card("//");CHECK(bad(TJC_FEATURE));
 begin();custom_proc("TESTPR");pc("//TESTPR PROC");pc("//RUN EXEC PGM=RXVM");pc("//SYSIN DD DSN=APP.ONE,DISP=SHR");pc("// DD DSN=APP.TWO,DISP=SHR");pc("// PEND");
 card("//BUILD EXEC TESTPR");card("//RUN.SYSIN DD");card("// DD DSN=APP.THREE,DISP=SHR");card("//");CHECK(!convert()&&result.dds==2U);
 CHECK(!strcmp(work.declaration[0].dsn,"APP.ONE")&&!strcmp(work.declaration[1].dsn,"APP.THREE"));
 begin();custom_proc("TESTPR");pc("//TESTPR PROC");pc("//RUN EXEC PGM=RXVM");pc("//SYSIN DD DUMMY,DCB=(RECFM=FB,LRECL=80,BLKSIZE=80)");pc("// PEND");
 card("//BUILD EXEC TESTPR");card("//RUN.SYSIN DD DSN=APP.SRC,DISP=SHR,DCB=(BLKSIZE=160)");card("//");CHECK(!convert()&&result.dds==1U&&TJBWORD(result.dd+32U+20U)==160U);
 begin();custom_proc("TESTPR");pc("//TESTPR PROC");pc("//RUN EXEC PGM=RXVM");pc("//SYSIN DD *");pc("DATA");pc("/*");pc("// PEND");
 card("//BUILD EXEC TESTPR");card("//");CHECK(bad(TJC_FEATURE));
 begin();custom_proc("TESTPR");pc("//TESTPR PROC X=A,X=B");pc("//RUN EXEC PGM=RXVM");pc("// PEND");
 card("//BUILD EXEC TESTPR");card("//");CHECK(bad(TJC_FEATURE));
 printf("PD033 PROC PASS %u checks; workspace %u bytes\n",checks,(unsigned int)sizeof work);return 0;
}
