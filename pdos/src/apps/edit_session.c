/* SPDX-License-Identifier: MIT; original C90 controller over EDIT's model. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "edit_session.h"
#include "edit_text.h"
unsigned int TEDWORD(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
int TEDINIT(TEDSTATE *s,TEDFILE file)
{
 memset(s,0,sizeof *s);s->file=file;s->top=1U;s->rows=19U;
 s->text=(EDCSTATE *)calloc(1,sizeof *s->text);s->undo=(EDCSTATE *)calloc(1,sizeof *s->undo);
 s->candidate=(EDCSTATE *)calloc(1,sizeof *s->candidate);s->saved=(EDCSTATE *)calloc(1,sizeof *s->saved);
 s->packed=(unsigned char *)malloc(TEF_LIMIT);
 s->draft=(unsigned char *)malloc(40000U);s->page=(unsigned char *)malloc(40000U);
 if(!s->text||!s->undo||!s->candidate||!s->saved||!s->packed||!s->draft||!s->page){TEDFREE(s);return 4;}
 strcpy(s->notice,"HELP lists commands; I/D in prefix; SAVE writes; CANCEL discards");return 0;
}
void TEDFREE(TEDSTATE *s)
{free(s->text);free(s->undo);free(s->candidate);free(s->saved);free(s->packed);free(s->draft);free(s->page);memset(s,0,sizeof *s);}
int TEDDIRTY(const TEDSTATE *s){return !EDCEQUAL(s->text,s->saved);}
void TEDTAKE(TEDSTATE *s)
{if(!EDCEQUAL(s->text,s->candidate)){*s->undo=*s->text;s->has_undo=1U;*s->text=*s->candidate;}}
static int decode(EDCSTATE *s)
{
 unsigned int i,n;unsigned char b[EDC_WIDTH];
 for(i=0U;i<s->count;++i){if(TEDUTF8(s->line[i].text,s->line[i].length,b,sizeof b,&n))return 20;
  memcpy(s->line[i].text,b,n);s->line[i].length=n;}return 0;
}
int TEDOPEN(TEDSTATE *s,const char *name)
{
 unsigned char info[TEF_BYTES];unsigned int rc;
 if(!name||strlen(name)>=sizeof s->source)return 8;
 memset(info,0,sizeof info);rc=s->file(TEF_LOAD,name,s->packed,TEF_LIMIT,info);
 if(!rc)rc=EDCLOAD(s->candidate,s->packed,TEDWORD(info+16U));
 if(!rc)rc=(unsigned int)decode(s->candidate);
 if(rc){sprintf(s->notice,"Open refused (%u); supported printable IBM1047 FB/VB text required",rc);return (int)rc;}
 *s->text=*s->candidate;*s->saved=*s->text;memcpy(s->info,info,sizeof info);strcpy(s->source,name);
 s->top=1U;s->left=s->has_undo=s->input_mode=s->draft_bytes=0U;
 strcpy(s->notice,(TEDWORD(info+36U)&TEF_EXISTS)?"Source opened; SAVE checks for intervening changes":"New member; SAVE creates it in the existing PDS");return 0;
}
int TEDSAVE(TEDSTATE *s,const char *target)
{
 unsigned char info[TEF_BYTES],b[EDC_WIDTH];unsigned int rc=0U,n,i,bytes=0U;
 int separate=target&&*target;
 if(!separate){target=s->source;if(!*target){strcpy(s->notice,"Use SAVE AS dataset(member) for the new buffer");return 8;}
  if(TEDWORD(s->info+36U)&TED_LEGACY){strcpy(s->notice,"Exchange source: use SAVE AS a separate empty dataset");return 20;}
  memcpy(info,s->info,sizeof info);
 }else{
  if(strlen(target)>=sizeof s->source||!strcmp(target,s->source)){strcpy(s->notice,"SAVE AS needs a separate target; SAVE uses the opened source");return 8;}
  memset(info,0,sizeof info);rc=s->file(TEF_LOAD,target,0,0U,info);
 }
 *s->candidate=*s->text;
 for(i=0U;!rc&&i<s->candidate->count;++i){
  rc=(unsigned int)TEDEBC(s->text->line[i].text,s->text->line[i].length,b,sizeof b,&n);
  if(!rc){memcpy(s->candidate->line[i].text,b,n);s->candidate->line[i].length=n;}
 }
 if(!rc)rc=EDCPKPAD(s->candidate,s->packed,TEF_LIMIT,TEDWORD(info+20U),TEDWORD(info+24U),0x40U,&bytes);
 if(!rc)rc=s->file(separate?TEF_NEW:TEF_SAVE,target,s->packed,bytes,info);
 if(rc){
  if(rc==4U)strcpy(s->notice,"Save refused: storage capacity exhausted; buffer retained");
  else if(rc==8U)strcpy(s->notice,"Save refused: records exceed target format/size; buffer retained");
  else if(rc==12U)strcpy(s->notice,"Save uncertain: retry SAVE to reconcile, or reopen separately; buffer retained");
  else if(rc==16U)strcpy(s->notice,"Save refused: target changed or is busy; buffer retained");
  else if(rc==20U)strcpy(s->notice,"Save refused: unsupported target or character; buffer retained");
  else sprintf(s->notice,"Save failed (%u); buffer retained",rc);
  return (int)rc;}
 memcpy(s->info,info,sizeof info);if(separate)strcpy(s->source,target);*s->saved=*s->text;
 sprintf(s->notice,"Saved %u records to %s",TEDWORD(info+32U),target);return 0;
}
static int number(char **p,unsigned int *n)
{unsigned int v=0U,got=0U;while(**p==' ')++*p;
 while(**p>='0'&&**p<='9'){if(v>EDC_LINES)return 0;v=v*10U+(unsigned int)(*(*p)++-'0');got=1U;}
 if(!got||v>EDC_LINES)return 0;*n=v;return 1;}
static int literal(const char *p,unsigned char *b,unsigned int *n)
{unsigned char native[EDC_WIDTH];unsigned int len=(unsigned int)strlen(p);
 if(len>sizeof native||TEDCENC(p,len,native))return 8;return TEDUTF8(native,len,b,EDC_WIDTH,n);}
static void locate(TEDSTATE *s)
{if(s->text->current)s->top=s->text->current;}
int TEDCMD(TEDSTATE *s,char *input)
{
 char command[20],*p=input,*end,*replacement;unsigned int i=0U,n,m,first,last,at,rc=8U;
 unsigned char text[EDC_WIDTH],to[EDC_WIDTH];EDCSTATE *swap;
 s->notice[0]=0;
 if(s->input_mode){
  if(!strcmp(input,".")){s->input_mode=0U;return 0;}
  if(!strcmp(input,".."))input[1]=0;
  *s->candidate=*s->text;
  if(literal(input,text,&n)||EDCINSERT(s->candidate,s->text->current,text,n))goto refused;
  if(!s->input_undo){*s->undo=*s->text;s->has_undo=s->input_undo=1U;}
  *s->text=*s->candidate;locate(s);return 0;
 }
 while(*p==' ')++p;
 while(*p&&*p!=' '&&i<sizeof command-1U)command[i++]=(char)toupper((unsigned char)*p++);
 command[i]=0;if(*p==' ')++p;
 if(!*command)return 0;
 if(!strcmp(command,"HELP")){s->help=!s->help;strcpy(s->notice,"HELP toggles help; PF3 returns only when saved; CANCEL discards");return 3;}
 if(!strcmp(command,"CANCEL")&&!*p)return 1;
 if(!strcmp(command,"END")&&!*p)return TEDSAVE(s,0)?0:1;
 if(!strcmp(command,"QUIT")){
  if(*p&&strcmp(p,"DISCARD")&&strcmp(p,"discard"))goto refused;
  if(TEDDIRTY(s)&&!*p){strcpy(s->notice,"Unsaved changes: SAVE, END, or CANCEL to discard");return 0;}return 1;
 }
 if(!strcmp(command,"SAVE")){
  if(!*p){TEDSAVE(s,0);return 0;}
  if((p[0]!='A'&&p[0]!='a')||(p[1]!='S'&&p[1]!='s')||p[2]!=' ')goto refused;
  p+=3;while(*p==' ')++p;if(!*p||strchr(p,' '))goto refused;
  for(i=0U;p[i];++i)p[i]=(char)toupper((unsigned char)p[i]);TEDSAVE(s,p);return 0;
 }
 if(!strcmp(command,"UNDO")&&!*p){if(!s->has_undo){strcpy(s->notice,"No change to undo");return 0;}
  swap=s->text;s->text=s->undo;s->undo=swap;locate(s);return 0;}
 if(!strcmp(command,"TOP")||!strcmp(command,"BOTTOM")){
  if(*p)goto refused;s->text->current=s->text->count?(!strcmp(command,"TOP")?1U:s->text->count):0U;locate(s);return 0;}
 if(!strcmp(command,"UP")||!strcmp(command,"DOWN")){
  n=s->rows;if(*p&&!number(&p,&n))goto refused;while(*p==' ')++p;if(*p)goto refused;
  if(!strcmp(command,"UP"))s->top=s->top>n?s->top-n:1U;
  else if(s->text->count>s->top)s->top+=n>s->text->count-s->top?s->text->count-s->top:n;
  return 0;
 }
 if(!strcmp(command,"LEFT")||!strcmp(command,"RIGHT")){
  n=40U;if(*p&&!number(&p,&n))goto refused;while(*p==' ')++p;if(*p)goto refused;
  if(!strcmp(command,"LEFT"))s->left=s->left>n?s->left-n:0U;
  else s->left+=n>EDC_WIDTH-1U-s->left?EDC_WIDTH-1U-s->left:n;return 0;
 }
 if(!strcmp(command,"PRINT")){
  first=s->text->count?1U:0U;last=s->text->count;
  if(*p){if(!number(&p,&first)||!first)goto refused;last=first;while(*p==' ')++p;if(*p&&!number(&p,&last))goto refused;}
  while(*p==' ')++p;if(*p||first>last||last>s->text->count)goto refused;s->first=first;s->last=last;return 2;
 }
 if(!strcmp(command,"LOCATE")||!strcmp(command,"FIND")){
  if(literal(p,text,&n)||!n)goto refused;rc=4U;
  for(i=s->text->current;i<s->text->count;++i)if(!EDCFIND(&s->text->line[i],text,n,&at)){s->text->current=i+1U;rc=0U;break;}
  if(rc)for(i=0U;i<s->text->current&&i<s->text->count;++i)if(!EDCFIND(&s->text->line[i],text,n,&at)){s->text->current=i+1U;rc=0U;break;}
  if(rc)strcpy(s->notice,"Literal not found");else locate(s);return 0;
 }
 if(!strcmp(command,"INPUT")){
  first=s->text->current;if(*p&&!number(&p,&first))goto refused;while(*p==' ')++p;if(*p||first>s->text->count)goto refused;
  s->text->current=first;s->input_mode=1U;s->input_undo=0U;strcpy(s->notice,"INPUT: . ends; .. inserts a dot");return 0;
 }
 *s->candidate=*s->text;
 if(!strcmp(command,"INSERT")){
  if(number(&p,&first)&&*p==' '){++p;if(!literal(p,text,&n))rc=EDCINSERT(s->candidate,first,text,n);}
 }else if(!strcmp(command,"DELETE")){
  if(number(&p,&first)){last=first;while(*p==' ')++p;if(!*p||number(&p,&last)){while(*p==' ')++p;if(!*p)rc=EDCDELETE(s->candidate,first,last);}}
 }else if(!strcmp(command,"CHANGE")){
  if(number(&p,&first)){while(*p==' ')++p;if(*p=='/'){++p;end=strchr(p,'/');if(end){*end=0;replacement=end+1;end=strchr(replacement,'/');
   if(end&&!end[1]){*end=0;if(!literal(p,text,&n)&&!literal(replacement,to,&m))rc=EDCCHANGE(s->candidate,first,text,n,to,m);}}}}
 }
 if(rc)goto refused;TEDTAKE(s);locate(s);return 0;
refused:
 strcpy(s->notice,"Command refused; HELP lists syntax and limits");return 0;
}
