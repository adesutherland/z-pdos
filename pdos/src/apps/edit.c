/* SPDX-License-Identifier: MIT; small U31 C fullscreen/line record editor. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "twospace_ui.h"
#include "twospace_recordio.h"
#include "edit_session.h"
#include "edit_screen.h"
#include "edit_text.h"
static void put(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static void init(unsigned char *r,unsigned int h,unsigned int g)
{memset(r,0,64U);put(r,1U);put(r+4U,64U);put(r+8U,h);put(r+12U,g);}
static void pointer(unsigned char *p,const void *v)
{put(p,0U);put(p+4U,(unsigned int)(unsigned long)v);}
static void out(const char *p)
{TUILINE((const unsigned char *)p,(unsigned int)strlen(p));}
static unsigned int files(unsigned int action,const char *name,unsigned char *data,unsigned int cap,unsigned char *info)
{
 unsigned int rc,i;unsigned char old[TRI_BYTES];
 if(action==TEF_NEW&&(TEDWORD(info+36U)&TED_LEGACY)){
  rc=TRIFILE(TRI_SAVE_EMPTY,name,data,cap,old);
 }else{
  rc=TEFFILE(action,name,data,cap,info);
  if(rc!=20U||action!=TEF_LOAD||strchr(name,'('))return rc;
  rc=TRIFILE(cap?TRI_LOAD:TRI_PROBE,name,data,cap,old);
 }
 if(!rc){memset(info,0,TEF_BYTES);for(i=16U;i<36U;++i)info[i]=old[i];put(info+36U,TEF_EXISTS|TED_LEGACY);}
 return rc;
}
static void show(const TEDSTATE *s)
{
 unsigned int i,n;unsigned char native[EDC_WIDTH];char b[80];
 for(i=s->first;i&&i<=s->last&&i<=s->text->count;++i){
  sprintf(b,"%u (%u UTF8 bytes)",i,s->text->line[i-1U].length);out(b);
  if(!TEDEBC(s->text->line[i-1U].text,s->text->line[i-1U].length,native,sizeof native,&n))TUILINE(native,n);
 }
}
static int line(TEDSTATE *s,const unsigned char *cap)
{
 unsigned char input[257];char command[257],b[140];unsigned int n,rc,limit;int action;
 limit=TEDWORD(cap+TSA_CAP_LINE_LIMIT);if(!limit||limit>256U)limit=256U;
 for(;;){
  if(*s->notice)out(s->notice);
  sprintf(b,"EDIT: %u records / current %u / %s",s->text->count,s->text->current,TEDDIRTY(s)?"UNSAVED":"saved");out(b);
  out(s->input_mode?"INPUT> . ends; .. inserts a dot":"EDIT>");
  rc=TUIREAD(input,limit,&n);if(rc)return (int)rc;
  if(TEDCDEC(input,n,command)){out("EDIT: unsupported command characters");continue;}command[n]=0;
  action=TEDCMD(s,command);if(action==1)return 0;if(action==2)show(s);
  if(action==3){out("PRINT first last / TOP / BOTTOM / FIND literal / INPUT after");out("INSERT after text / DELETE first last / CHANGE line /old/new/ / UNDO");out("SAVE / SAVE AS dataset(member) / END / QUIT / CANCEL (discard)");}
 }
}
static int screen(TEDSTATE *s)
{
 unsigned char r[64],cap[128],*record,*stream,*buffer;T27GEOMETRY geometry;
 unsigned int h=0U,g=0U,rc,n,kind,aid=0U,cursor=0xffffffffU,close_rc,input_mode,buffered;char command[257];int result=0,action;
 record=(unsigned char *)malloc(TES_MAX);stream=(unsigned char *)malloc(TES_MAX);
 buffer=(unsigned char *)malloc(TES_MAX);
 if(!record||!stream||!buffer){result=4;goto done;}
 init(r,0U,0U);rc=TUIIO(64U,r,TSA_IO_DEVICE_OPEN);if(rc){result=(int)rc;goto done;}h=TEDWORD(r+8U);g=TEDWORD(r+12U);
 /* OPEN may enqueue the device query; acknowledge immutable initial events. */
 for(;;){init(r,h,g);put(r+28U,TES_MAX);pointer(r+32U,record);rc=TUIIO(64U,r,TSA_IO_DEVICE_EVENT);
  if(rc==16U)break;if(rc){result=(int)rc;goto done;}put(r+16U,0U);rc=TUIIO(64U,r,TSA_IO_DEVICE_ACK);if(rc){result=(int)rc;goto done;}}
 for(;;){
  init(r,h,g);put(r+28U,sizeof cap);pointer(r+32U,cap);rc=TUIIO(64U,r,TSA_IO_DEVICE_CAPS);
  if(rc){result=(int)rc;break;}g=TEDWORD(r+12U);
  if(!(TEDWORD(cap+48U)&TSA_FEATURE_READ_BUFFER)){out("EDIT: use LINE on this terminal; exact field positions require Read Buffer");result=20;break;}
  if(T27GEOM(&geometry,TEDWORD(cap+16U),TEDWORD(cap+20U),TEDWORD(cap+40U))||
     TESPAINT(s,&geometry,stream,TES_MAX,&n,cursor)){result=20;break;}
  s->rows=geometry.rows-5U;
  init(r,h,g);put(r+16U,1U);put(r+20U,0xf1U);put(r+28U,n);pointer(r+32U,stream);
  rc=TUIIO(64U,r,TSA_IO_DEVICE_WRITE);if(rc){result=(int)rc;break;}
  init(r,h,g);put(r+20U,0xf6U);put(r+28U,TES_MAX);pointer(r+32U,record);rc=TUIIO(64U,r,TSA_IO_DEVICE_READ);
  if(rc){result=(int)rc;break;}n=TEDWORD(r+48U);kind=TEDWORD(r+52U);
  put(r+16U,0U);close_rc=TUIIO(64U,r,TSA_IO_DEVICE_ACK);if(close_rc){result=(int)close_rc;break;}
  /* Read Modified suppresses interior nulls. Read Buffer preserves their
   * positions; only the immutable modified-field list is applied. */
  if(kind==1U&&n>3U){
   init(r,h,g);put(r+20U,0xf2U);put(r+28U,TES_MAX);pointer(r+32U,buffer);rc=TUIIO(64U,r,TSA_IO_DEVICE_READ);
   if(rc){result=(int)rc;break;}buffered=TEDWORD(r+48U);
   rc=(unsigned int)TESFULL(&geometry,record,n,buffer,buffered,stream,TES_MAX,&n);
   put(r+16U,0U);close_rc=TUIIO(64U,r,TSA_IO_DEVICE_ACK);if(close_rc){result=(int)close_rc;break;}
   if(rc){strcpy(s->notice,"Page refused; complete field positions could not be verified");continue;}
   memcpy(record,stream,n);
  }
  if(kind==1U){rc=(unsigned int)TESINPUT(s,&geometry,record,n,command,&aid,&cursor);
   /* TESINPUT retains rejected fields and supplies the recovery message. */
  }else rc=20U;
  if(rc)continue;
  if(aid!=0x7dU){
   if(aid==0xf1U)strcpy(command,"HELP");else if(aid==0xf2U)strcpy(command,"SAVE");
   else if(aid==0xf3U)strcpy(command,"QUIT");else if(aid==0xf7U)strcpy(command,"UP");
   else if(aid==0xf8U)strcpy(command,"DOWN");else if(aid==0xf9U)strcpy(command,"UNDO");
   else if(aid==0x7aU)strcpy(command,"LEFT");else if(aid==0x7bU)strcpy(command,"RIGHT");
  }
  input_mode=s->input_mode;
  if(aid!=0x7dU)s->input_mode=0U;
  action=TEDCMD(s,command);if(action==1)break;
  if(aid!=0x7dU){s->input_mode=input_mode;if(aid==0xf9U)s->input_undo=0U;}
  if(action==3)cursor=(geometry.rows-3U)*geometry.columns+7U;
  if(action==2){s->top=s->first?s->first:1U;strcpy(s->notice,"PRINT range positioned at its first record");}
 }
 done:
 if(h){init(r,h,g);close_rc=TUIIO(64U,r,TSA_IO_DEVICE_CLOSE);if(!result)result=(int)close_rc;}
 free(record);free(stream);free(buffer);return result;
}
int main(int argc,char **argv)
{
 TEDSTATE s;unsigned char cap[64];int rc=0,force_line=0;const char *source=0;
 if(argc>1&&(!strcmp(argv[1],"HELP")||!strcmp(argv[1],"help"))){
  printf("EDIT dataset(member) [LINE]\nType source; I/D prefixes insert/delete. SAVE writes; END saves/exits.\nINPUT after / . ends input; PRINT / UNDO / HELP; CANCEL discards.\n");return 0;}
 if(argc>3){printf("EDIT [dataset(member)] [LINE]\n");return 8;}
 if(argc>1){if(!strcmp(argv[1],"LINE"))force_line=1;else source=argv[1];}
 if(argc>2){if(strcmp(argv[2],"LINE")){printf("EDIT [dataset(member)] [LINE]\n");return 8;}force_line=1;}
 if(TEDINIT(&s,files))return 4;
 if(TUIOPEN((const unsigned char *)"EDIT / record editor",sizeof "EDIT / record editor"-1U)||TUICAPS(cap)){rc=8;goto done;}
 if(source&&TEDOPEN(&s,source)){out(s.notice);rc=8;goto done;}
 if(force_line||TEDWORD(cap+TSA_CAP_DEVICE_CLASS)==TSA_DEVICE_LINE)rc=line(&s,cap);else rc=screen(&s);
 if(rc){out("EDIT: terminal failed; completed or uncertain saves may be on disk");if(TEDDIRTY(&s))out("EDIT: unsaved buffer could not be retained after application exit");}
 else out("EDIT: returned to console");
 done:
 TEDFREE(&s);return rc;
}
