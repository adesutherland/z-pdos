/* SPDX-License-Identifier: MIT; shared line/screen controller and field proof. */
#include <assert.h>
#include <string.h>
#include "apps/edit_screen.h"
#include "apps/edit_text.h"
static unsigned char stored[1024],version=1U;static unsigned int stored_n,fail;
static EDCSTATE prior;
static void put(unsigned char *p,unsigned int n){p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static unsigned int file(unsigned int action,const char *name,unsigned char *p,unsigned int n,unsigned char *info)
{
 (void)name;if(fail)return fail;
 if(action!=TEF_LOAD){if(info[104U]!=version)return 16U;assert(n<=sizeof stored);memcpy(stored,p,n);stored_n=n;++version;}
 else if(n){assert(n>=stored_n);memcpy(p,stored,stored_n);}
 memset(info,0,TEF_BYTES);put(info+16U,stored_n);put(info+20U,0x90U);put(info+24U,80U);put(info+28U,80U);put(info+32U,stored_n/82U);put(info+36U,TEF_EXISTS);info[104U]=version;return 0;
}
static int cmd(TEDSTATE *s,const char *command){char b[257];assert(strlen(command)<sizeof b);strcpy(b,command);return TEDCMD(s,b);}
static unsigned int start(const T27GEOMETRY *g,unsigned char *p,unsigned int aid,unsigned int cursor)
{p[0]=(unsigned char)aid;assert(!T27ADDR(g,cursor,p+1U));return 3U;}
static unsigned int field(const T27GEOMETRY *g,unsigned char *p,unsigned int at,unsigned int addr,const char *text)
{unsigned int n=(unsigned int)strlen(text);p[at++]=0x11U;assert(!T27ADDR(g,addr,p+at));at+=2U;assert(!TEDCENC(text,n,p+at));return at+n;}
int main(void)
{
 TEDSTATE s;T27GEOMETRY g;unsigned char raw[2048],screen[TES_MAX],full[TES_MAX],exact[TES_MAX],native[256],utf8[512],round[256];
 unsigned int i,n,k,m,j,at,cell,aid,cursor,models[3][2]={{24U,80U},{43U,80U},{27U,132U}};char command[257];
 assert(!TEDINIT(&s,file));
 for(i=0U;i<256U;++i){native[0]=(unsigned char)i;
  if(!TEDUTF8(native,1U,utf8,sizeof utf8,&n)){assert(!TEDEBC(utf8,n,round,sizeof round,&k)&&k==1U&&round[0]==native[0]);}}
 assert(TEDEBC((unsigned char *)"\300\240",2U,native,sizeof native,&n));
 assert(TEDEBC((unsigned char *)"\342\202\254",3U,native,sizeof native,&n));
 assert(TEDUTF8((unsigned char *)"\000",1U,utf8,sizeof utf8,&n));
 stored[0]=0;stored[1]=80;memset(stored+2U,0x40U,80U);assert(!TEDCENC("say 'before'",12U,stored+2U));stored_n=82U;
 assert(!TEDOPEN(&s,"SOURCE(A)")&&!TEDDIRTY(&s));
 assert(!cmd(&s,"CHANGE 1 /before/after/"));assert(TEDDIRTY(&s));prior=*s.text;
 fail=12U;assert(TEDSAVE(&s,0)==12&&TEDDIRTY(&s)&&EDCEQUAL(s.text,&prior));fail=0U;
 ++version;assert(TEDSAVE(&s,0)==16&&TEDDIRTY(&s));assert(!cmd(&s,"QUIT"));
 assert(!cmd(&s,"UNDO")&&!TEDDIRTY(&s));assert(!cmd(&s,"UNDO")&&TEDDIRTY(&s));
 assert(!TEDOPEN(&s,"SOURCE(A)"));assert(!cmd(&s,"CHANGE 1 /before/after/"));assert(!TEDSAVE(&s,0)&&!TEDDIRTY(&s));
 assert(!TEDOPEN(&s,"SOURCE(A)"));assert(!EDCFIND(&s.text->line[0],(unsigned char *)"after",5U,&n));
 assert(!cmd(&s,"INPUT 1"));assert(!cmd(&s,"two"));assert(!cmd(&s,".."));assert(!cmd(&s,"."));assert(s.text->count==3U);
 assert(!cmd(&s,"UNDO")&&s.text->count==1U);assert(!cmd(&s,"UNDO")&&s.text->count==3U);
 for(i=0U;i<3U;++i){
  assert(!T27GEOM(&g,models[i][0],models[i][1],T27_CODED12));s.rows=g.rows-5U;s.top=1U;s.left=0U;s.help=0U;
  assert(!TESPAINT(&s,&g,screen,sizeof screen,&n,0xffffffffU));assert(!T27WRITE(&g,screen,n));
  n=start(&g,raw,0x7dU,(g.rows-3U)*g.columns+7U);
  n=field(&g,raw,n,2U*g.columns+10U,"say 'screen'");
  n=field(&g,raw,n,(g.rows-3U)*g.columns+7U,"SAVE");
  assert(!TESINPUT(&s,&g,raw,n,command,&aid,&cursor)&&!strcmp(command,"SAVE")&&aid==0x7dU);
  assert(!EDCFIND(&s.text->line[0],(unsigned char *)"screen",6U,&k));
  prior=*s.text;
  n=field(&g,raw,n,2U*g.columns+10U,"duplicate");
  assert(TESINPUT(&s,&g,raw,n,command,&aid,&cursor)&&EDCEQUAL(s.text,&prior));
  n=start(&g,raw,0x7dU,2U*g.columns+10U);n=field(&g,raw,n,2U*g.columns+10U,"would change");
  n=field(&g,raw,n,1U,"protected");assert(TESINPUT(&s,&g,raw,n,command,&aid,&cursor)&&EDCEQUAL(s.text,&prior));
  n=start(&g,raw,0x7dU,2U*g.columns+10U);n=field(&g,raw,n,2U*g.columns+1U,"I");
  m=s.text->count;assert(!TESINPUT(&s,&g,raw,n,command,&aid,&cursor)&&s.text->count==m+1U);
  assert(!cmd(&s,"UNDO")&&s.text->count==m);
 }
 /* New source: a blank second record needs no guard or seed member. */
 memset(s.text,0,sizeof *s.text);s.top=1U;s.left=0U;s.help=0U;s.draft_bytes=0U;
 assert(!T27GEOM(&g,24U,80U,T27_CODED12));
 n=start(&g,raw,0x7dU,330U);n=field(&g,raw,n,170U,"options levelb");n=field(&g,raw,n,330U,"say 'hello'");
 assert(!TESINPUT(&s,&g,raw,n,command,&aid,&cursor)&&s.text->count==3U&&s.text->line[1].length==0U);
 /* Valid typed changes survive an invalid prefix, including the SAVE field.
  * A corrected prefix applies the page once; UNDO restores all three rows. */
 prior=*s.text;n=start(&g,raw,0x7dU,161U);n=field(&g,raw,n,170U,"options levelb /* revised */");
 n=field(&g,raw,n,161U,"X");n=field(&g,raw,n,1687U,"SAVE");
 assert(TESINPUT(&s,&g,raw,n,command,&aid,&cursor)&&s.draft_bytes&&EDCEQUAL(s.text,&prior));
 assert(!TESPAINT(&s,&g,screen,sizeof screen,&n,161U)&&!T27WRITE(&g,screen,n));
 n=start(&g,raw,0x7dU,161U);n=field(&g,raw,n,161U," ");
 assert(!TESINPUT(&s,&g,raw,n,command,&aid,&cursor)&&!s.draft_bytes&&!strcmp(command,"SAVE"));
 assert(!EDCFIND(&s.text->line[0],(unsigned char *)"revised",7U,&k));assert(!cmd(&s,"UNDO")&&EDCEQUAL(s.text,&prior));
 n=start(&g,raw,0x7dU,161U);n=field(&g,raw,n,161U,"X");assert(TESINPUT(&s,&g,raw,n,command,&aid,&cursor));
 assert(!T27GEOM(&g,27U,132U,T27_CODED12));n=start(&g,raw,0x7dU,2U*g.columns+10U);
 assert(TESINPUT(&s,&g,raw,n,command,&aid,&cursor)==16&&s.draft_bytes);
 n=field(&g,raw,n,(g.rows-3U)*g.columns+7U,"CANCEL");assert(!TESINPUT(&s,&g,raw,n,command,&aid,&cursor)&&!strcmp(command,"CANCEL")&&!s.draft_bytes);
 /* Command-field text never overrides the actual PF action. */
 n=start(&g,raw,0xf2U,(g.rows-3U)*g.columns+7U);n=field(&g,raw,n,(g.rows-3U)*g.columns+7U,"CANCEL");
 assert(!TESINPUT(&s,&g,raw,n,command,&aid,&cursor)&&aid==0xf2U&&EDCEQUAL(s.text,&prior));
 raw[0]=0xf1U;assert(!TESINPUT(&s,&g,raw,n,command,&aid,&cursor)&&aid==0xf1U&&EDCEQUAL(s.text,&prior));
 assert(!cmd(&s,"INPUT 0"));n=start(&g,raw,0x7dU,(g.rows-3U)*g.columns+7U);n=field(&g,raw,n,(g.rows-3U)*g.columns+7U,"CANCEL");
 assert(!TESINPUT(&s,&g,raw,n,command,&aid,&cursor)&&s.input_mode&&!strcmp(command,"CANCEL"));
 assert(!TEDCMD(&s,command)&&s.input_mode&&s.text->line[0].length==6U);assert(!cmd(&s,"."));
 /* An off-screen tail is preserved at the same byte columns. */
 s.left=40U;s.top=1U;memset(native,0xc1U,sizeof native);assert(!TEDUTF8(native,200U,s.text->line[0].text,256U,&n));s.text->line[0].length=n;
 assert(!T27GEOM(&g,24U,80U,T27_CODED12));n=start(&g,raw,0x7dU,170U);n=field(&g,raw,n,170U,"B");
 assert(!TESINPUT(&s,&g,raw,n,command,&aid,&cursor));
 assert(!TEDEBC(s.text->line[0].text,s.text->line[0].length,native,sizeof native,&n)&&n==200U);
 assert(native[39U]==0xc1U&&native[40U]==0xc2U&&native[41U]==0x40U&&native[109U]==0xc1U);
 /* The provider suppresses interior nulls in Read Modified. Verify exact
  * positions recovered from a complete field-mode Read Buffer response. */
 s.left=0U;s.text->line[0].length=1U;s.text->line[0].text[0]=0x41U;
 assert(!TESPAINT(&s,&g,screen,sizeof screen,&n,0xffffffffU));
 at=start(&g,full,0x60U,180U);cell=0U;
 for(j=4U;j<n;++j){
  if(screen[j]==0x13U)continue;
  if(screen[j]==0x1dU){full[at++]=screen[j++];full[at++]=screen[j];++cell;}
  else{full[at++]=cell==180U?0xc2U:screen[j];++cell;}
 }
 assert(cell==g.cells);n=start(&g,raw,0x7dU,180U);n=field(&g,raw,n,170U,"AB");
 assert(!TESFULL(&g,raw,n,full,at,exact,sizeof exact,&m));
 assert(!TESINPUT(&s,&g,exact,m,command,&aid,&cursor));
 assert(s.text->line[0].length==11U&&s.text->line[0].text[10U]==0x42U);
 for(i=1U;i<10U;++i)assert(s.text->line[0].text[i]==0x20U);
 prior=*s.text;assert(TESFULL(&g,raw,n,full,at-1U,exact,sizeof exact,&m)&&EDCEQUAL(s.text,&prior));
 assert(cmd(&s,"CANCEL")==1);TEDFREE(&s);return 0;
}
