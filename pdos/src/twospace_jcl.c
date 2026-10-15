/* SPDX-License-Identifier: MIT; PD-033 S4-01 direct JCL converter. */
#include <stddef.h>
#include <string.h>
#include "twospace_jcl.h"
/* Printable repertoire from the maintained pdos/scripts/ibm1047.table.
 * Lookup by execution character, never by an assumed ASCII character value. */
static const char printable[]=
 " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
static const unsigned char native[]={
 0x40U,0x5aU,0x7fU,0x7bU,0x5bU,0x6cU,0x50U,0x7dU,0x4dU,0x5dU,0x5cU,0x4eU,0x6bU,0x60U,0x4bU,0x61U,
 0xf0U,0xf1U,0xf2U,0xf3U,0xf4U,0xf5U,0xf6U,0xf7U,0xf8U,0xf9U,0x7aU,0x5eU,0x4cU,0x7eU,0x6eU,0x6fU,
 0x7cU,0xc1U,0xc2U,0xc3U,0xc4U,0xc5U,0xc6U,0xc7U,0xc8U,0xc9U,0xd1U,0xd2U,0xd3U,0xd4U,0xd5U,0xd6U,
 0xd7U,0xd8U,0xd9U,0xe2U,0xe3U,0xe4U,0xe5U,0xe6U,0xe7U,0xe8U,0xe9U,0xadU,0xe0U,0xbdU,0x5fU,0x6dU,
 0x79U,0x81U,0x82U,0x83U,0x84U,0x85U,0x86U,0x87U,0x88U,0x89U,0x91U,0x92U,0x93U,0x94U,0x95U,0x96U,
 0x97U,0x98U,0x99U,0xa2U,0xa3U,0xa4U,0xa5U,0xa6U,0xa7U,0xa8U,0xa9U,0xc0U,0x4fU,0xd0U,0xa1U};
int TJCENC(const char *p,unsigned int n,unsigned char *out)
{
 unsigned int i;const char *at;
 if(!p||!out)return 8;
 for(i=0U;i<n;++i){at=strchr(printable,p[i]);if(!p[i]||!at)return 20;out[i]=native[at-printable];}
 return 0;
}
int TJCDEC(const unsigned char *p,unsigned int n,char *out)
{
 unsigned int i,k;if(!p||!out)return 8;
 for(i=0U;i<n;++i){for(k=0U;k<sizeof native&&native[k]!=p[i];++k){}
  if(k==sizeof native)return 20;out[i]=printable[k];}
 return 0;
}
const char *TJCMSG(unsigned int reason)
{
 switch(reason){case TJC_SYNTAX:return "malformed JCL";
 case TJC_FEATURE:return "unsupported JCL operand or statement";
 case TJC_LIMIT:return "converter or descriptor capacity exceeded";
 case TJC_SYMBOL:return "undefined or invalid symbol";
 case TJC_END:return "incomplete or trailing job input";
 case TJC_RESOLVE:return "program/library/DD admission failed";
 case TJC_DESCRIPTOR:return "expanded descriptor rejected";
 default:return "invalid converter request";}
}
static int fail(TJCWORK *w,TJCERROR *e,unsigned int reason,unsigned int at)
{
 e->reason=reason;e->card=w->card;e->column=w->column;
 if(at<TJC_TEXT&&w->expanded_card[at]){e->card=w->expanded_card[at];e->column=w->expanded_column[at];}
 return reason==TJC_LIMIT?4:reason==TJC_FEATURE?20:8;
}
static int letter(char c)
{return c&&strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZ@#$",c)!=0;}
static int digit(char c){return c>='0'&&c<='9';}
static int ident(const char *p,unsigned int maximum,int national)
{
 unsigned int i,n=(unsigned int)strlen(p);if(!n||n>maximum||!letter(p[0]))return 0;
 for(i=0U;i<n;++i)if(!(letter(p[i])||(i&&digit(p[i])))||(!national&&strchr("@#$",p[i])))return 0;
 return 1;
}
static void native_name(unsigned char *out,unsigned int size,const char *p)
{memset(out,0x40,size);TJCENC(p,(unsigned int)strlen(p),out);}
static int number(const char *p,unsigned int maximum,unsigned int *value)
{
 unsigned int n=0U,i;if(!p[0])return 0;
 for(i=0U;p[i];++i){unsigned int d;if(!digit(p[i]))return 0;d=(unsigned int)(p[i]-'0');
  if(n>maximum/10U||(n==maximum/10U&&d>maximum%10U))return 0;n=n*10U+d;}
 *value=n;return 1;
}
/* Read one complete logical JCL statement. Columns 73-80 are sequence data;
 * column 72 is an explicit continuation indicator. Comma continuation and
 * comments between continuation cards are also accepted. */
static int statement(TJCWORK *w,const unsigned char *deck,unsigned int cards,
                     unsigned int *next,TJCERROR *e)
{
 char line[72];unsigned int i,n,start,used=0U,first=1U,more,quote=0U,prior,first_card=0U;
 memset(w->source_card,0,sizeof w->source_card);memset(w->expanded_card,0,sizeof w->expanded_card);
 w->name[0]=w->operation[0]=w->text[0]=0;
 for(;;){
  if(*next==cards)return fail(w,e,TJC_END,TJC_TEXT);
  w->card=*next+1U;w->column=1U;
  if(TJCDEC(deck+80U*(*next),72U,line))return fail(w,e,TJC_FEATURE,TJC_TEXT);
  ++*next;
  if(line[0]!='/'||line[1]!='/')return fail(w,e,TJC_SYNTAX,TJC_TEXT);
  if(line[2]=='*')continue;
  if(first){
   first_card=w->card;
   start=2U;
   if(line[start]!=' '){for(n=0U;start<71U&&line[start]!=' ';++start){
      if(n==sizeof w->name-1U)return fail(w,e,TJC_LIMIT,TJC_TEXT);w->name[n++]=line[start];}w->name[n]=0;}
   while(start<71U&&line[start]==' ')++start;
   for(n=0U;start<71U&&line[start]!=' ';++start){if(n==8U)return fail(w,e,TJC_FEATURE,TJC_TEXT);w->operation[n++]=line[start];}
   w->operation[n]=0;while(start<71U&&line[start]==' ')++start;
   if(!w->name[0]&&!w->operation[0]){
    if(line[71]!=' ')return fail(w,e,TJC_SYNTAX,TJC_TEXT);w->ended=1U;return 0;}
   first=0U;
  }else{
   if(line[2]!=' ')return fail(w,e,TJC_SYNTAX,TJC_TEXT);
   if(quote){for(i=3U;i<15U;++i)if(line[i]!=' ')return fail(w,e,TJC_SYNTAX,TJC_TEXT);start=15U;}
   else {start=3U;while(start<71U&&line[start]==' ')++start;}
   if(start==71U)return fail(w,e,TJC_SYNTAX,TJC_TEXT);
  }
  prior=quote;
  for(i=start;i<71U;++i)if(line[i]=='\''){
   if(prior&&i+1U<71U&&line[i+1U]=='\'')++i;else prior=!prior;}
  n=71U;while(n>start&&line[n-1U]==' ')--n;
  /* This selected quoted continuation requires an explicit column-72 mark.
   * Every byte through column 71, including blanks, belongs to the string. */
  if(prior&&line[71]!=' ')n=71U;
  if(n-start>=TJC_TEXT-used)return fail(w,e,TJC_LIMIT,TJC_TEXT);
  for(i=start;i<n;++i){w->source_card[used]=(unsigned short)w->card;
   w->source_column[used]=(unsigned short)(i+1U);w->text[used++]=line[i];}
  w->text[used]=0;quote=0U;
  for(i=0U;i<used;++i)if(w->text[i]=='\''){
   if(quote&&i+1U<used&&w->text[i+1U]=='\'')++i;else quote=!quote;}
  more=line[71]!=' '||(!quote&&used&&w->text[used-1U]==',');
  if(!more){if(quote)return fail(w,e,TJC_SYNTAX,TJC_TEXT);w->card=first_card;return 0;}
 }
}
static int substitute(TJCWORK *w,TJCERROR *e)
{
 unsigned int i=0U,n=0U,j,k,start;char key[9];const char *value;
 while(w->text[i]){
  start=i;value=0;
  if(w->text[i]=='&'&&w->text[i+1U]!='&'){
   ++i;for(k=0U;letter(w->text[i])||digit(w->text[i]);++i){if(k==8U)return fail(w,e,TJC_SYMBOL,TJC_TEXT);key[k++]=w->text[i];}
   key[k]=0;if(!ident(key,8U,1))return fail(w,e,TJC_SYMBOL,TJC_TEXT);
   for(j=0U;j<w->symbols;++j)if(!strcmp(key,w->symbol[j].name)){value=w->symbol[j].value;break;}
   if(!value){e->reason=TJC_SYMBOL;e->card=w->source_card[start];e->column=w->source_column[start];return 8;}
   if(w->text[i]=='.')++i;
  }
  if(value){for(k=0U;value[k];++k){if(n==TJC_TEXT-1U)return fail(w,e,TJC_LIMIT,TJC_TEXT);
    w->expanded[n]=value[k];w->expanded_card[n]=w->source_card[start];w->expanded_column[n++]=w->source_column[start];}}
  else{
   j=w->text[i]=='&'&&w->text[i+1U]=='&'?2U:1U;
   for(k=0U;k<j;++k){if(n==TJC_TEXT-1U)return fail(w,e,TJC_LIMIT,TJC_TEXT);
    w->expanded[n]=w->text[i];w->expanded_card[n]=w->source_card[i];w->expanded_column[n++]=w->source_column[i++];}
  }
 }
 w->expanded[n]=0;return 0;
}
/* Split only top-level commas. Nested tuples and doubled apostrophes remain
 * intact. Unquoted embedded blanks and trailing commas cannot be ignored. */
static int operand(TJCWORK *w,unsigned int *cursor,char *key,char *value,
                   unsigned int *where,TJCERROR *e)
{
 unsigned int i=*cursor,start=i,n=0U,depth=0U,quote=0U,equal=TJC_TEXT;
 if(!w->expanded[i])return 1;*where=i;
 for(;w->expanded[i];++i){char c=w->expanded[i];
  if(c=='\''){if(quote&&w->expanded[i+1U]=='\''){++i;continue;}quote=!quote;}
  else if(!quote){if(c=='(')++depth;else if(c==')'){if(!depth)return fail(w,e,TJC_SYNTAX,i);--depth;}
   else if(c==','&&!depth)break;else if(c=='='&&!depth){if(equal!=TJC_TEXT)return fail(w,e,TJC_SYNTAX,i);equal=i;}
   else if(c==' ')return fail(w,e,TJC_SYNTAX,i);}
 }
 if(quote||depth||i==start)return fail(w,e,TJC_SYNTAX,start);
 if(equal!=TJC_TEXT){if(equal==start||equal-start>8U)return fail(w,e,TJC_FEATURE,start);
  memcpy(key,w->expanded+start,equal-start);key[equal-start]=0;start=equal+1U;}
 else key[0]=0;
 n=i-start;if(!n||n>=TJC_TEXT)return fail(w,e,TJC_SYNTAX,start);
 memcpy(value,w->expanded+start,n);value[n]=0;
 if(w->expanded[i]){if(!w->expanded[i+1U])return fail(w,e,TJC_SYNTAX,i);++i;}
 *cursor=i;return 0;
}
static int unquote(char *p)
{
 unsigned int i,n=0U,len=(unsigned int)strlen(p);
 if(!len)return 0;if(p[0]!='\'')return strchr(p,'\'')==0;
 if(len<2U||p[len-1U]!='\'')return 0;
 for(i=1U;i<len-1U;++i){if(p[i]=='\''){if(i+1U>=len-1U||p[i+1U]!='\'')return 0;++i;}p[n++]=p[i];}
 p[n]=0;return 1;
}
static int region(const char *p){return !strcmp(p,"0M")||!strcmp(p,"256M");}
static int parameter(char *p)
{
 unsigned int i,n=0U;
 if(p[0]!='\'')for(i=0U;p[i];++i)if(!letter(p[i])&&!digit(p[i]))return 0;
 if(!unquote(p))return 0;
 for(i=0U;p[i];++i){if(p[i]=='&'&&p[i+1U]=='&')++i;p[n++]=p[i];}p[n]=0;return 1;
}
static int set_symbol(TJCWORK *w,TJCERROR *e)
{
 char key[9],value[TJC_TEXT];unsigned int at=0U,where=0U,i;int rc;
 rc=operand(w,&at,key,value,&where,e);if(rc)return rc==1?fail(w,e,TJC_SYNTAX,0U):rc;
 if(w->expanded[at]||!ident(key,8U,1)||!unquote(value)||strlen(value)>100U)return fail(w,e,TJC_SYMBOL,where);
 for(i=0U;i<w->symbols&&strcmp(key,w->symbol[i].name);++i){}
 if(i==TJC_SYMBOLS)return fail(w,e,TJC_LIMIT,where);if(i==w->symbols)++w->symbols;
 strcpy(w->symbol[i].name,key);strcpy(w->symbol[i].value,value);return 0;
}
static int job(TJCWORK *w,TJCERROR *e)
{
 char key[9],value[TJC_TEXT];unsigned int at=0U,where=0U,positional=0U,seen=0U,bit;int rc;
 if(w->started||!ident(w->name,8U,0))return fail(w,e,TJC_SYNTAX,0U);
 while((rc=operand(w,&at,key,value,&where,e))==0){
  if(!key[0]){if(seen||++positional>2U)return fail(w,e,TJC_SYNTAX,where);continue;}
  if(!strcmp(key,"CLASS")){bit=1U;if(strcmp(value,"A"))return fail(w,e,TJC_FEATURE,where);}
  else if(!strcmp(key,"MSGCLASS")){bit=2U;if(strcmp(value,"A"))return fail(w,e,TJC_FEATURE,where);}
  else if(!strcmp(key,"MSGLEVEL")){bit=4U;if(strcmp(value,"(1,1)"))return fail(w,e,TJC_FEATURE,where);}
  else if(!strcmp(key,"REGION")){bit=8U;if(!region(value))return fail(w,e,TJC_FEATURE,where);}
  else return fail(w,e,TJC_FEATURE,where);
  if(seen&bit)return fail(w,e,TJC_SYNTAX,where);seen|=bit;
 }
 if(rc!=1)return rc;w->started=1U;TJBPUT(w->result.job,1U);TJBPUT(w->result.job+4U,TJB_DESC_BYTES);
 native_name(w->result.job+8U,8U,w->name);TJBPUT(w->result.job+20U,TJB_PRINT_BYTES);return 0;
}
static int exec_step(TJCWORK *w,TJCERROR *e)
{
 char key[9],value[TJC_TEXT];unsigned int at=0U,where=0U,seen=0U,bit,i;int rc;
 TJCSTEP *s;unsigned char *d;
 if(!ident(w->name,8U,0))return fail(w,e,TJC_SYNTAX,0U);
 if(w->result.steps==TJB_STEPS)return fail(w,e,TJC_LIMIT,0U);
 for(i=0U;i<w->result.steps;++i)if(!strcmp(w->result.step[i].name,w->name))return fail(w,e,TJC_SYNTAX,0U);
 s=&w->result.step[w->result.steps];d=w->result.job+32U+w->result.steps*TJB_STEP_BYTES;
 strcpy(s->name,w->name);s->card=w->card;
 s->library_count=w->joblibs;memcpy(s->libraries,w->joblib,sizeof w->joblib);native_name(d,8U,w->name);
 while((rc=operand(w,&at,key,value,&where,e))==0){
  if(!strcmp(key,"PGM")){bit=1U;if(!ident(value,8U,1))return fail(w,e,TJC_SYNTAX,where);strcpy(s->program,value);}
  else if(!strcmp(key,"PARM")){unsigned int n;bit=2U;
   if(!parameter(value))return fail(w,e,TJC_FEATURE,where);n=(unsigned int)strlen(value);
   if(n>100U)return fail(w,e,TJC_LIMIT,where);TJBPUT(d+16U,n);TJCENC(value,n,d+24U);}
  else if(!strcmp(key,"COND")){bit=4U;if(strcmp(value,"(0,NE)"))return fail(w,e,TJC_FEATURE,where);TJBPUT(d+20U,TJB_COND_NE);}
  else if(!strcmp(key,"REGION")){bit=8U;if(!region(value))return fail(w,e,TJC_FEATURE,where);}
  else return fail(w,e,TJC_FEATURE,where);
  if(seen&bit)return fail(w,e,TJC_SYNTAX,where);seen|=bit;
 }
 if(rc!=1)return rc;if(!(seen&1U))return fail(w,e,TJC_FEATURE,0U);
 ++w->result.steps;return 0;
}
static int dataset(char *text,TJCDECL *d)
{
 unsigned int i,n,part=0U;char *open;
 if(!unquote(text))return 0;open=strchr(text,'(');
 if(open){n=(unsigned int)strlen(open);if(n<3U||n>10U||open[n-1U]!=')')return 0;
  open[n-1U]=0;if(!ident(open+1U,8U,1))return 0;strcpy(d->member,open+1U);*open=0;}
 if(text[0]=='&'&&text[1]=='&'){
  if(open||!ident(text+2U,8U,1))return 0;d->kind=TJD_TEMP;strcpy(d->dsn,text+2U);return 1;}
 n=(unsigned int)strlen(text);if(!n||n>44U)return 0;
 for(i=0U;i<n;++i){char c=text[i];if(c=='.'){if(!part)return 0;part=0U;}
  else {if(part==8U||!(letter(c)||(part&&digit(c))))return 0;++part;}}
 if(!part)return 0;strcpy(d->dsn,text);d->kind=TJD_DATASET;return 1;
}
static int dcb(TJCWORK *w,char *text,TJCATTR *a,unsigned int where,TJCERROR *e)
{
 char *p=text,*end,*eq;unsigned int seen=0U,bit,value,len=(unsigned int)strlen(text);
 if(len<3U||text[0]!='('||text[len-1U]!=')')return fail(w,e,TJC_SYNTAX,where);
 text[len-1U]=0;++p;
 while(*p){end=strchr(p,',');if(end)*end=0;eq=strchr(p,'=');if(!eq)return fail(w,e,TJC_SYNTAX,where);*eq++=0;
  if(!strcmp(p,"RECFM")){bit=1U;if(!strcmp(eq,"FB"))a->recfm=TJD_FB;else if(!strcmp(eq,"VB"))a->recfm=TJD_VB;else return fail(w,e,TJC_FEATURE,where);}
  else if(!strcmp(p,"LRECL")){bit=2U;if(!number(eq,32760U,&value)||!value)return fail(w,e,TJC_SYNTAX,where);a->lrecl=value;}
  else if(!strcmp(p,"BLKSIZE")){bit=4U;if(!number(eq,32760U,&value))return fail(w,e,TJC_SYNTAX,where);a->blksize=value;}
  else return fail(w,e,TJC_FEATURE,where);
  if(seen&bit)return fail(w,e,TJC_SYNTAX,where);seen|=bit;if(!end)break;p=end+1U;if(!*p)return fail(w,e,TJC_SYNTAX,where);
 }return 0;
}
static int inline_data(TJCWORK *w,const unsigned char *deck,unsigned int cards,
                       unsigned int *next,TJCDECL *d,const char *dlm,
                       unsigned int data,TJCERROR *e)
{
 unsigned char delimiter[2];unsigned int i,n=(unsigned int)strlen(dlm);
 if(n!=2U||TJCENC(dlm,n,delimiter))return fail(w,e,TJC_FEATURE,0U);
 d->kind=TJD_INLINE;d->attributes.direction=TJD_INPUT;
 if((d->attributes.recfm&&d->attributes.recfm!=TJD_FB)||(d->attributes.lrecl&&d->attributes.lrecl!=80U))return fail(w,e,TJC_FEATURE,0U);
 d->attributes.recfm=TJD_FB;d->attributes.lrecl=80U;if(!d->attributes.blksize)d->attributes.blksize=80U;
 d->offset=w->inline_at;
 while(*next<cards){const unsigned char *p=deck+80U*(*next);++*next;
  if(p[0]==delimiter[0]&&p[1]==delimiter[1]){
   for(i=2U;i<80U&&p[i]==0x40U;++i){}if(i==80U){d->bytes=w->inline_at-d->offset;return 0;}}
  /* Native DD * also ends on a following JCL statement. This selected input
   * contract requires its explicit delimiter, so reject that ambiguous deck.
   * DD DATA may carry // records as actual data. */
  if(data==1U&&p[0]==0x61U&&p[1]==0x61U)return fail(w,e,TJC_END,0U);
  if(w->inline_at>TJD_BYTES-82U)return fail(w,e,TJC_LIMIT,0U);
  w->result.dd[w->inline_at]=0U;w->result.dd[w->inline_at+1U]=80U;
  memcpy(w->result.dd+w->inline_at+2U,p,80U);w->inline_at+=82U;
 }
 return fail(w,e,TJC_END,0U);
}
static int dd(TJCWORK *w,const unsigned char *deck,unsigned int cards,
               unsigned int *next,TJCERROR *e)
{
 TJCDECL d,*old;TJCSTEP *s=0;char key[9],value[TJC_TEXT],dlm[3];
 unsigned int at=0U,where=0U,seen=0U,bit,data=0U,i,lib=0U;int rc;
 memset(&d,0,sizeof d);strcpy(dlm,"/*");d.card=w->card;d.column=1U;d.step=w->result.steps;
 if(w->result.steps)s=&w->result.step[w->result.steps-1U];
 if(!w->name[0]&&w->last_library)lib=w->last_library;
 if(!strcmp(w->name,"JOBLIB")){if(s)return fail(w,e,TJC_SYNTAX,0U);lib=1U;}
 else if(!strcmp(w->name,"STEPLIB")){if(!s)return fail(w,e,TJC_SYNTAX,0U);lib=2U;}
 else if(!lib&&!s)return fail(w,e,TJC_SYNTAX,0U);
 if(w->name[0]){if(!ident(w->name,8U,1))return fail(w,e,TJC_FEATURE,0U);strcpy(d.name,w->name);}
 else if(lib){} /* Anonymous DD continues this library, not an application DD. */
 else{
  if(!w->result.dds)return fail(w,e,TJC_SYNTAX,0U);old=&w->declaration[w->result.dds-1U];
  if(old->step!=d.step||old->ordinal==3U||old->kind!=TJD_DATASET)return fail(w,e,TJC_FEATURE,0U);
  strcpy(d.name,old->name);d.ordinal=old->ordinal+1U;d.attributes=old->attributes;
 }
 while((rc=operand(w,&at,key,value,&where,e))==0){
  if(!key[0]){
   bit=1U;if(!strcmp(value,"DUMMY"))d.kind=TJD_DUMMY;
   else if(!strcmp(value,"*"))data=1U;else if(!strcmp(value,"DATA"))data=2U;else return fail(w,e,TJC_FEATURE,where);
  }else if(!strcmp(key,"DSN")||!strcmp(key,"DSNAME")){bit=1U;if(!dataset(value,&d))return fail(w,e,TJC_SYNTAX,where);}
  else if(!strcmp(key,"SYSOUT")){bit=1U;if(strcmp(value,"A")&&strcmp(value,"*"))return fail(w,e,TJC_FEATURE,where);d.kind=TJD_SYSOUT;d.attributes.direction=TJD_OUTPUT;}
  else if(!strcmp(key,"DISP")){bit=2U;
   if(!strcmp(value,"SHR"))d.status=3U;else if(!strcmp(value,"OLD"))d.status=TJD_OLD;
   else if(!strcmp(value,"(NEW,PASS,DELETE)")){d.status=TJD_NEW;d.disposition=TJD_PASS;}
   else if(!strcmp(value,"(OLD,DELETE,DELETE)")){d.status=TJD_OLD;d.disposition=TJD_DELETE;}
   else if(!strcmp(value,"(OLD,PASS,DELETE)")){d.status=TJD_OLD;d.disposition=TJD_PASS;}
   else return fail(w,e,TJC_FEATURE,where);
  }else if(!strcmp(key,"DCB")){bit=4U;rc=dcb(w,value,&d.attributes,where,e);if(rc)return rc;}
  else if(!strcmp(key,"DLM")){bit=8U;if(!unquote(value)||strlen(value)!=2U)return fail(w,e,TJC_SYNTAX,where);
   if(strchr(value,'&'))return fail(w,e,TJC_FEATURE,where);strcpy(dlm,value);}
  else return fail(w,e,TJC_FEATURE,where);
  if(seen&bit)return fail(w,e,TJC_SYNTAX,where);seen|=bit;
 }
 if(rc!=1)return rc;if(!(seen&1U)||(seen&8U&&!data))return fail(w,e,TJC_SYNTAX,0U);
 if(lib){TJCLIB *list;unsigned int *count;
  if(data||d.kind!=TJD_DATASET||d.member[0]||d.status!=3U||(seen&~3U))return fail(w,e,TJC_FEATURE,0U);
  if(lib==1U){count=&w->joblibs;list=w->joblib;}
  else if(lib==2U&&s){count=&s->library_count;list=s->libraries;}
  else return fail(w,e,TJC_SYNTAX,0U);
  if(w->name[0]){
   if((lib==1U&&w->joblibs)||(lib==2U&&s->explicit_library))return fail(w,e,TJC_SYNTAX,0U);
   if(lib==2U){/* STEPLIB replaces JOBLIB, it is not appended to it. */
    s->explicit_library=1U;*count=0U;memset(list,0,sizeof s->libraries);}}
  if(*count==TJC_LIBS)return fail(w,e,TJC_LIMIT,0U);strcpy(list[(*count)++].dsn,d.dsn);w->last_library=lib;return 0;
 }
 if(d.kind==TJD_DATASET){if(d.disposition||!d.status)return fail(w,e,TJC_FEATURE,0U);}
 else if(d.kind==TJD_TEMP){if(!d.disposition||d.status==3U)return fail(w,e,TJC_FEATURE,0U);d.quota=TJD_TEMP_LIMIT;}
 else if(d.status||d.disposition)return fail(w,e,TJC_FEATURE,0U);
 if(w->result.dds==TJD_COUNT)return fail(w,e,TJC_LIMIT,0U);
 if(!d.ordinal){unsigned int count=0U;
  for(i=0U;i<w->result.dds;++i)if(w->declaration[i].step==d.step&&!w->declaration[i].ordinal)++count;
  if(count==8U)return fail(w,e,TJC_LIMIT,0U);}
 for(i=0U;i<w->result.dds;++i)if(!d.ordinal&&w->declaration[i].step==d.step&&!strcmp(w->declaration[i].name,d.name))return fail(w,e,TJC_SYNTAX,0U);
 if(data){rc=inline_data(w,deck,cards,next,&d,dlm,data,e);if(rc)return rc;}
 w->declaration[w->result.dds++]=d;w->last_library=0U;return 0;
}
static int attributes_equal(const TJCATTR *before,const TJCATTR *after)
{return (!before->direction||before->direction==after->direction)&&
 (!before->recfm||before->recfm==after->recfm)&&(!before->lrecl||before->lrecl==after->lrecl)&&
 (!before->blksize||before->blksize==after->blksize);}
static int resolve(TJCWORK *w,const TJCSITE *site,TJCERROR *e)
{
 unsigned int i;int rc;TJCRESULT *r=&w->result;
 TJBPUT(r->job+16U,r->steps);TJBPUT(r->dd,2U);TJBPUT(r->dd+4U,TJD_BYTES);
 TJBPUT(r->dd+8U,r->dds);TJBPUT(r->dd+12U,r->steps);TJBPUT(r->dd+16U,1U);
 for(i=0U;i<r->steps;++i){w->card=r->step[i].card;w->column=1U;
  if(!site->program||site->program(site->context,&r->step[i],r->job+40U+i*TJB_STEP_BYTES))return fail(w,e,TJC_RESOLVE,TJC_TEXT);}
 for(i=0U;i<r->dds;++i){TJCDECL *d=&w->declaration[i];TJCATTR a=d->attributes;unsigned char *out=r->dd+32U+i*TJD_ENTRY;
  w->card=d->card;w->column=d->column;
  if(!site->dd||site->dd(site->context,&r->step[d->step-1U],d,&a)||!attributes_equal(&d->attributes,&a))return fail(w,e,TJC_RESOLVE,TJC_TEXT);
  TJBPUT(out,d->step);TJBPUT(out+4U,d->kind);TJBPUT(out+8U,a.direction);TJBPUT(out+12U,a.recfm);
  TJBPUT(out+16U,a.lrecl);TJBPUT(out+20U,a.blksize);native_name(out+32U,8U,d->name);
  if(d->kind==TJD_DATASET||d->kind==TJD_TEMP){native_name(out+40U,44U,d->dsn);if(d->member[0])native_name(out+84U,8U,d->member);}
  if(d->kind==TJD_INLINE){TJBPUT(out+24U,d->offset);TJBPUT(out+28U,d->bytes);}
  if(d->kind==TJD_TEMP)TJBPUT(out+24U,d->quota);
  TJBPUT(out+92U,d->ordinal|(d->disposition<<8)|(d->kind==TJD_TEMP?d->status<<16:0U));
 }
 rc=TJBCHECK(r->job,TJB_DESC_BYTES);if(!rc)rc=TJDCHK(r->dd,r->steps);
 if(rc)return fail(w,e,TJC_DESCRIPTOR,TJC_TEXT);r->valid=1U;return 0;
}
int TJCCONV(TJCWORK *w,const unsigned char *deck,unsigned int bytes,
            const TJCSITE *site,TJCRESULT *out,TJCERROR *e)
{
 unsigned int next=0U,cards,i;int rc;
 if(out)memset(out,0,sizeof *out);if(e)memset(e,0,sizeof *e);
 if(!w||!deck||!site||!out||!e)return 8;
 memset(w,0,sizeof *w);w->card=w->column=1U;w->inline_at=TJD_DATA;
 if(!bytes||bytes%80U)return fail(w,e,TJC_END,TJC_TEXT);
 cards=bytes/80U;if(cards>TJC_CARDS)return fail(w,e,TJC_LIMIT,TJC_TEXT);
 while(next<cards){
  rc=statement(w,deck,cards,&next,e);if(rc)return rc;
  if(w->ended){if(!w->started||!w->result.steps)return fail(w,e,TJC_SYNTAX,TJC_TEXT);
   if(next!=cards)return fail(w,e,TJC_END,TJC_TEXT);break;}
  rc=substitute(w,e);if(rc)return rc;
  if(strcmp(w->operation,"DD"))w->last_library=0U;
  if(!strcmp(w->operation,"JOB"))rc=job(w,e);
  else if(!w->started)rc=fail(w,e,TJC_SYNTAX,0U);
  else if(!strcmp(w->operation,"SET")){if(w->name[0])rc=fail(w,e,TJC_SYNTAX,0U);else rc=set_symbol(w,e);}
  else if(!strcmp(w->operation,"EXEC"))rc=exec_step(w,e);
  else if(!strcmp(w->operation,"DD"))rc=dd(w,deck,cards,&next,e);
  else rc=fail(w,e,TJC_FEATURE,0U);
  if(rc)return rc;
 }
 if(!w->ended)return fail(w,e,TJC_END,TJC_TEXT);
 /* No callbacks were made while parsing: a bad final card starts no queries,
  * let alone a program. Clear stale source maps before resolver diagnostics. */
 for(i=0U;i<TJC_TEXT;++i)w->expanded_card[i]=0U;
 rc=resolve(w,site,e);if(rc)return rc;memcpy(out,&w->result,sizeof *out);return 0;
}
