/* SPDX-License-Identifier: MIT; independent direct-JCL admission controls. */
#include <stdio.h>
#include <string.h>
#include "twospace_jcl.h"
static TJCWORK work;
static TJCRESULT result;
static TJCERROR error;
static unsigned char deck[80U*(TJC_CARDS+1U)];
static unsigned int cards,checks,program_calls,dd_calls,tamper,deny;
static char observed_libs[4][45];
#define CHECK(x) do {++checks;if(!(x)){printf("JCL FAIL line %u card %u col %u reason %u\n",(unsigned int)__LINE__,error.card,error.column,error.reason);return 1;}} while(0)
static void reset(void)
{cards=program_calls=dd_calls=tamper=deny=0U;memset(observed_libs,0,sizeof observed_libs);}
static void card(const char *p)
{unsigned int n=(unsigned int)strlen(p);if(n>80U||cards>TJC_CARDS){puts("bad test card");return;}
 memset(deck+80U*cards,0x40,80U);TJCENC(p,n,deck+80U*cards);++cards;}
static void begin(void)
{reset();card("//TEST JOB CLASS=A,MSGCLASS=A,MSGLEVEL=(1,1),REGION=0M");}
static int program(void *context,const TJCSTEP *s,unsigned char out[8])
{unsigned int i;(void)context;++program_calls;
 for(i=0U;i<s->library_count;++i)strcpy(observed_libs[i],s->libraries[i].dsn);
 if(deny||strcmp(s->program,"RXVM"))return 8;
 memcpy(out,"12345678",8U);/* Replaced with independently expected native RXVM. */
 out[0]=0xd9U;out[1]=0xe7U;out[2]=0xe5U;out[3]=0xd4U;memset(out+4U,0x40,4U);return 0;}
static int dd(void *context,const TJCSTEP *s,const TJCDECL *d,TJCATTR *a)
{(void)context;(void)s;++dd_calls;
 if(deny)return 8;
 if(!a->direction)a->direction=!strcmp(d->name,"SYSPRINT")||!strcmp(d->name,"SYSTERM")||!strcmp(d->name,"SYSGO")?TJD_OUTPUT:TJD_INPUT;
 if(!a->recfm)a->recfm=TJD_FB;if(!a->lrecl)a->lrecl=80U;if(!a->blksize)a->blksize=a->lrecl;
 if(tamper)++a->lrecl;return 0;}
static int convert(void)
{TJCSITE site;site.context=0;site.program=program;site.dd=dd;
 return TJCCONV(&work,deck,cards*80U,&site,&result,&error);}
static int rejected(unsigned int reason)
{unsigned int i;const unsigned char *p=(const unsigned char *)&result;
 if(!convert()||error.reason!=reason||program_calls||dd_calls)return 0;
 for(i=0U;i<sizeof result;++i)if(p[i])return 0;return 1;}
static unsigned char *entry(unsigned int i){return result.dd+32U+i*TJD_ENTRY;}
int main(void)
{
 unsigned int i;unsigned char encoded[8];char decoded[8];TJCSITE site;
 static const unsigned char expected[]={0xd9U,0xe7U,0xe5U,0xd4U,0xadU,0xbdU,0x5fU,0x40U};
 CHECK(sizeof work<=65536U);
 CHECK(!TJCENC("RXVM[]^ ",8U,encoded)&&!memcmp(encoded,expected,8U));
 CHECK(!TJCDEC(expected,8U,decoded)&&!memcmp(decoded,"RXVM[]^ ",8U));
 encoded[0]=0U;CHECK(TJCDEC(encoded,1U,decoded)==20);CHECK(TJCENC("\n",1U,encoded)==20);
 begin();card("//RUN EXEC PGM=RXVM,PARM='-v',REGION=256M");card("//SYSIN DD DUMMY");
 card("//SYSPRINT DD SYSOUT=A");card("//SYSTERM DD SYSOUT=*");card("//");
 CHECK(!convert()&&result.valid&&result.steps==1U&&result.dds==3U&&program_calls==1U&&dd_calls==3U);
 CHECK(!TJBCHECK(result.job,TJB_DESC_BYTES)&&!TJDCHK(result.dd,1U));
 CHECK(TJBWORD(result.job+48U)==2U&&result.job[56U]==0x60U&&result.job[57U]==0xa5U);
 CHECK(TJDWORD(entry(0)+4U)==TJD_DUMMY&&TJDWORD(entry(0)+8U)==TJD_INPUT);
 CHECK(TJDWORD(entry(1)+4U)==TJD_SYSOUT&&TJDWORD(entry(1)+8U)==TJD_OUTPUT);
 /* Unquoted positional accounting, quoted description, embedded comma and
    doubled apostrophe, with an actual physical continuation/comment. */
 reset();card("//TEST JOB (A,B),'O''Brien',CLASS=A");card("//RUN EXEC PGM=RXVM,");
 card("//* comment inside continuation");card("//             PARM='a,b O''Brien'");card("//");
 CHECK(!convert());CHECK(TJBWORD(result.job+48U)==11U);
 {char parm[12];CHECK(!TJCDEC(result.job+56U,11U,parm));parm[11]=0;CHECK(!strcmp(parm,"a,b O'Brien"));}
 begin();card("//RUN EXEC PGM=RXVM,PARM='3462&&5'");card("//");CHECK(!convert()&&TJBWORD(result.job+48U)==6U&&result.job[60U]==0x50U);
 {char line[81];unsigned int prefix;begin();memset(line,' ',80U);line[80]=0;
  prefix=(unsigned int)strlen("//RUN EXEC PGM=RXVM,PARM='");memcpy(line,"//RUN EXEC PGM=RXVM,PARM='",prefix);
  for(i=prefix;i<68U;++i)line[i]='A';line[71]='X';card(line);card("//             B'");card("//");
  CHECK(!convert()&&TJBWORD(result.job+48U)==72U-prefix);
  CHECK(result.job[56U]==0xc1U&&result.job[56U+68U-prefix]==0x40U&&result.job[56U+71U-prefix]==0xc2U);}
 begin();card("// SET HLQ=LABA01");card("//RUN EXEC PGM=RXVM,PARM='&HLQ'");
 card("//SYSIN DD DSN=&HLQ..SRC(MAIN),DISP=SHR");card("//");
 CHECK(!convert()&&TJBWORD(result.job+48U)==6U);
 CHECK(work.declaration[0].kind==TJD_DATASET&&!strcmp(work.declaration[0].dsn,"LABA01.SRC")&&!strcmp(work.declaration[0].member,"MAIN"));
 CHECK(entry(0)[40U]==0xd3U&&entry(0)[84U]==0xd4U);
 begin();card("//JOBLIB DD DSN=SITE.FIRST,DISP=SHR");card("// DD DSN=SITE.SECOND,DISP=SHR");
 card("//A EXEC PGM=RXVM");card("//B EXEC PGM=RXVM,COND=(0,NE)");
 card("//STEPLIB DD DSN=APP.FIRST,DISP=SHR");card("// DD DSN=APP.SECOND,DISP=SHR");card("//");
 CHECK(!convert()&&result.steps==2U&&result.dds==0U);
 CHECK(result.step[0].library_count==2U&&!strcmp(result.step[0].libraries[0].dsn,"SITE.FIRST")&&!strcmp(result.step[0].libraries[1].dsn,"SITE.SECOND"));
 CHECK(result.step[1].library_count==2U&&!strcmp(observed_libs[0],"APP.FIRST")&&!strcmp(observed_libs[1],"APP.SECOND"));
 CHECK(TJBWORD(result.job+32U+TJB_STEP_BYTES+20U)==TJB_COND_NE);
 begin();card("//RUN EXEC PGM=RXVM");card("//SYSIN DD DSN=APP.ONE,DISP=SHR,DCB=(RECFM=VB,LRECL=84,");
 card("//             BLKSIZE=88)");card("// DD DSN=APP.TWO,DISP=SHR");card("//");
 CHECK(!convert()&&result.dds==2U&&TJDWORD(entry(1)+92U)==1U);
 CHECK(TJDWORD(entry(1)+12U)==TJD_VB&&TJDWORD(entry(1)+16U)==84U);
 begin();card("//RUN EXEC PGM=RXVM");card("//SYSIN DD DATA,DLM='@@'");card("// input, not JCL");card("/*");card("@@");card("//");
 CHECK(!convert()&&TJDWORD(entry(0)+28U)==164U);
 CHECK(result.dd[TJD_DATA]==0U&&result.dd[TJD_DATA+1U]==80U&&result.dd[TJD_DATA+2U]==0x61U&&result.dd[TJD_DATA+3U]==0x61U);
 begin();card("//RUN EXEC PGM=RXVM");card("//SYSIN DD *");card("");card("/*");card("//");
 CHECK(!convert()&&TJDWORD(entry(0)+28U)==82U&&result.dd[TJD_DATA+2U]==0x40U);
 begin();card("//A EXEC PGM=RXVM");card("//SYSGO DD DSN=&&OBJ,DISP=(NEW,PASS,DELETE)");
 card("//B EXEC PGM=RXVM,COND=(0,NE)");card("//SYSIN DD DSN=&&OBJ,DISP=(OLD,DELETE,DELETE)");card("//");
 CHECK(!convert()&&TJDWORD(entry(0)+4U)==TJD_TEMP&&TJDWORD(entry(0)+92U)==0x10100U&&TJDWORD(entry(1)+92U)==0x20200U);
 /* Failure controls prove that even a valid prefix publishes no result and
    invokes no site callback when the final syntax/framing is invalid. */
 begin();card("//RUN EXEC PGM=RXVM");CHECK(rejected(TJC_END));
 begin();card("//RUN EXEC PGM=RXVM");card("//");card("//TRAIL EXEC PGM=RXVM");CHECK(rejected(TJC_END));
 begin();card("//RUN EXEC PGM=RXVM");card("//SYSIN DD *");card("abc");CHECK(rejected(TJC_END));
 begin();card("//RUN EXEC PGM=RXVM");card("//SYSIN DD *");card("//NEXT EXEC PGM=RXVM");card("/*");card("//");CHECK(rejected(TJC_END));
 begin();card("//RUN EXEC PGM=RXVM");card("//SYSIN DD DATA,DLM='@@'");card("abc");card("@@trailing");CHECK(rejected(TJC_END));
 begin();card("//RUN EXEC PGM=RXVM,PARM='unterminated");card("//");CHECK(rejected(TJC_SYNTAX));
 begin();card("//RUN EXEC PGM=RXVM,UNKNOWN=1");card("//");CHECK(rejected(TJC_FEATURE)&&error.card==2U&&error.column==21U);
 begin();card("//RUN EXEC PGM=RXVM,");card("//             UNKNOWN=1");card("//");CHECK(rejected(TJC_FEATURE)&&error.card==3U&&error.column==16U);
 begin();card("//RUN EXEC PGM=RXVM,PARM='&MISSING'");card("//");CHECK(rejected(TJC_SYMBOL)&&error.card==2U&&error.column==27U);
 begin();card("// SET LONG='1234567890123456789012345678901234567890'");
 card("//RUN EXEC PGM=RXVM,PARM='&LONG&LONG&LONG'");card("//");CHECK(rejected(TJC_LIMIT));
 begin();card("//RUN EXEC PROC=CRXCG");card("//");CHECK(rejected(TJC_FEATURE));
 begin();card("//RUN EXEC PGM=RXVM,COND=(4,GT)");card("//");CHECK(rejected(TJC_FEATURE));
 begin();card("//RUN EXEC PGM=RXVM,PARM=(A,B)");card("//");CHECK(rejected(TJC_FEATURE));
 begin();card("//RUN EXEC PGM=RXVM");card("//SYSIN DD DATA,DLM='&&'");card("&&");card("//");CHECK(rejected(TJC_FEATURE));
 begin();card("//RUN EXEC PGM=RXVM,REGION=8M");card("//");CHECK(rejected(TJC_FEATURE));
 begin();card("//RUN EXEC PGM=RXVM");card("//SYSIN DD DSN=APP.SRC,DISP=SHR,SPACE=(TRK,(1,1))");card("//");CHECK(rejected(TJC_FEATURE));
 begin();card("//RUN EXEC PGM=RXVM");card("//SYSIN DD DSN=APP.SRC");card("//");CHECK(rejected(TJC_FEATURE));
 begin();card("//RUN EXEC PGM=RXVM");card("//SYSIN DD DSN=APP..SRC,DISP=SHR");card("//");CHECK(rejected(TJC_SYNTAX));
 begin();card("//RUN EXEC PGM=RXVM");card("//SYSIN DD DUMMY");card("//SYSIN DD DUMMY");card("//");CHECK(rejected(TJC_SYNTAX));
 begin();card("//RUN EXEC PGM=RXVM");card("//STEPLIB DD DSN=APP.LOAD,DISP=SHR");card("//STEPLIB DD DSN=APP.OTHER,DISP=SHR");card("//");CHECK(rejected(TJC_SYNTAX));
 begin();card("//RUN EXEC PGM=RXVM,PGM=RXVM");card("//");CHECK(rejected(TJC_SYNTAX));
 begin();card("//RUN EXEC PGM=RXVM");card("//RUN EXEC PGM=RXVM");card("//");CHECK(rejected(TJC_SYNTAX));
 begin();card("//RUN EXEC PGM=RXVM");card("// DD DSN=APP.SRC,DISP=SHR");card("//");CHECK(rejected(TJC_SYNTAX));
 begin();card("//RUN EXEC PGM=RXVM");card("//SYSIN DD DUMMY");card("// DD DSN=APP.SRC,DISP=SHR");card("//");CHECK(rejected(TJC_FEATURE));
 begin();card("//RUN EXEC PGM=RXVM");card("//SYSIN DD DUMMY,DCB=(RECFM=FB,LRECL=80,LRECL=80)");card("//");CHECK(rejected(TJC_SYNTAX));
 begin();card("//RUN EXEC PGM=RXVM");card("//SYSIN DD DUMMY,DCB=(RECFM=FB,LRECL=999999999999999999999)");card("//");CHECK(rejected(TJC_SYNTAX));
 begin();card("//RUN EXEC PGM=RXVM");card("//SYSIN DD *");for(i=0U;i<19U;++i)card("data");card("/*");card("//");CHECK(rejected(TJC_LIMIT));
 begin();for(i=0U;i<9U;++i){char line[40];strcpy(line,"//S0 EXEC PGM=RXVM");line[3]=(char)('0'+i);card(line);}card("//");CHECK(rejected(TJC_LIMIT));
 begin();card("//RUN EXEC PGM=RXVM");for(i=0U;i<9U;++i){char line[40];strcpy(line,"//D0 DD DUMMY");line[3]=(char)('0'+i);card(line);}card("//");CHECK(rejected(TJC_LIMIT));
 begin();card("//RUN EXEC PGM=RXVM");card("//SYSIN DD DSN=APP.ONE,DISP=SHR");
 for(i=0U;i<3U;++i)card("// DD DSN=APP.MORE,DISP=SHR");card("//");CHECK(!convert()&&result.dds==4U&&TJDWORD(entry(3)+92U)==3U);
 begin();card("//RUN EXEC PGM=RXVM");card("//SYSIN DD DSN=APP.ONE,DISP=SHR");
 for(i=0U;i<4U;++i)card("// DD DSN=APP.MORE,DISP=SHR");card("//");CHECK(rejected(TJC_FEATURE));
 begin();card("//RUN EXEC PGM=RXVM");card("//SYSIN DD DSN=APP.SRC,DISP=SHR,DCB=(LRECL=80)");card("//");tamper=1U;
 CHECK(convert()&&error.reason==TJC_RESOLVE&&!result.valid&&program_calls==1U&&dd_calls==1U);
 begin();card("//RUN EXEC PGM=RXVM");card("//");deny=1U;CHECK(convert()&&error.reason==TJC_RESOLVE&&!result.valid);
 reset();for(i=0U;i<TJC_CARDS+1U;++i)card("//* comment");CHECK(rejected(TJC_LIMIT));
 site.context=0;site.program=program;site.dd=dd;reset();
 CHECK(TJCCONV(&work,deck,79U,&site,&result,&error)&&error.reason==TJC_END&&!result.valid);
 CHECK(TJCCONV(&work,deck,0U,&site,&result,&error)&&error.reason==TJC_END);
 CHECK(TJCCONV(0,deck,80U,&site,&result,&error)==8&&!result.valid);
 /* Single-octet mutations exercise the actual native byte boundary. Success
    must still pass the independent K validators; any failure clears all output. */
 begin();card("//RUN EXEC PGM=RXVM,PARM='-v'");card("//SYSIN DD DUMMY");card("//SYSPRINT DD SYSOUT=A");card("//SYSTERM DD SYSOUT=A");card("//");
 {unsigned int at,value,n=cards*80U;unsigned char old;int rc;
  for(at=0U;at<n;++at){old=deck[at];for(value=0U;value<256U;value+=17U){
    unsigned int k;const unsigned char *p=(const unsigned char *)&result;
    deck[at]=(unsigned char)value;program_calls=dd_calls=0U;rc=convert();
    if(rc){CHECK(!result.valid&&error.reason&&error.card&&error.column);for(k=0U;k<sizeof result&&p[k]==0U;++k){}CHECK(k==sizeof result);}
    else CHECK(result.valid&&!TJBCHECK(result.job,TJB_DESC_BYTES)&&!TJDCHK(result.dd,result.steps));
   }deck[at]=old;}}
 printf("PD033 JCL PASS %u checks; workspace %u bytes\n",checks,(unsigned int)sizeof work);return 0;
}
