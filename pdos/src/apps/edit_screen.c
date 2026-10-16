/* SPDX-License-Identifier: MIT; original EE-style prefix/data/command page.
 * Every submitted field is validated before the shared model changes. */
#include <stdio.h>
#include <string.h>
#include "edit_screen.h"
#include "edit_text.h"
static int geometry(const T27GEOMETRY *g)
{return g&&g->rows>=24U&&g->rows<=43U&&g->columns>=80U&&g->columns<=132U&&g->cells==g->rows*g->columns;}
static int puttext(unsigned char *p,unsigned int width,const char *text)
{unsigned int n=(unsigned int)strlen(text);if(n>width)n=width;return TEDCENC(text,n,p);}
int TESPAINT(const TEDSTATE *s,const T27GEOMETRY *g,unsigned char *out,unsigned int cap,unsigned int *used,unsigned int cursor)
{
 unsigned char row[132],native[EDC_WIDTH];char b[160];unsigned int r,c,n,at=0U,width,number,offset;
 static const char *help[]={"EDIT / small C record editor / one buffer",
  "Type over source; I inserts a blank after a row; D deletes a row.",
  "SAVE checks the opened member; SAVE AS requires a new/empty target.",
  "END saves and exits; QUIT/PF3 refuses unsaved changes; CANCEL discards.",
  "INPUT after / . ends input / INSERT after text / DELETE first last",
  "FIND literal / CHANGE line /old/new/ / UNDO / TOP / BOTTOM",
  "UP/DOWN [rows] / LEFT/RIGHT [columns] / HELP toggles this page.",
  "4096 records, 256 UTF8 bytes per record, 64KiB packed native input.",
  "IBM1047 text only; unsupported records/characters are refused.",
  "SAVE error 12 may mean uncertain publication: buffer stays available.",
  "Retry SAVE reconciles identical content; reopen to inspect externally."};
 if(!geometry(g)||!out||!used||cap<g->cells+g->rows*8U+16U)return 8;
 if(cursor>=g->cells)cursor=(g->rows-3U)*g->columns+7U;
 out[at++]=0xc3U;out[at++]=0x11U;if(T27ADDR(g,0U,out+at))return 8;at+=2U;width=g->columns-11U;
 for(r=0U;r<g->rows;++r){
  memset(row,0x40U,g->columns);
  if(!r){sprintf(b,"EDIT / %s",*s->source?s->source:"new buffer");if(puttext(row+1U,g->columns-2U,b))return 8;}
  else if(r==1U){sprintf(b,"%s / %u records / top %u / columns %u-%u",TEDDIRTY(s)?"UNSAVED":"saved",s->text->count,s->top,s->left+1U,s->left+width);if(puttext(row+1U,g->columns-2U,b))return 8;}
  else if(r<g->rows-3U){
   if(s->help){if(r-2U<sizeof help/sizeof help[0]&&puttext(row+1U,g->columns-2U,help[r-2U]))return 8;}
   else{
    number=s->top+r-2U;memset(row+10U,0,width);
    if(number<=s->text->count){sprintf(b,"%5u",number);if(TEDCENC(b,5U,row+4U))return 8;
     if(TEDEBC(s->text->line[number-1U].text,s->text->line[number-1U].length,native,sizeof native,&n))return 8;
     if(n>s->left){n-=s->left;if(n>width)n=width;memcpy(row+10U,native+s->left,n);}
    }else if(number==s->text->count+1U){if(puttext(row+4U,5U,"new"))return 8;}
   }
  }else if(r==g->rows-3U){if(puttext(row+1U,5U,s->input_mode?"INP> ":"===> "))return 8;memset(row+7U,0,g->columns-8U);}
  else if(r==g->rows-2U){if(puttext(row+1U,g->columns-2U,s->notice))return 8;}
  else if(puttext(row+1U,g->columns-2U,"PF1 Help PF2 Save PF3 Quit PF7/8 Page PF9 Undo PF10/11 Left/Right"))return 8;
  for(c=0U;c<g->columns;++c){
   offset=r*g->columns+c;
   if(offset==cursor)out[at++]=0x13U;
   if(!c||c==g->columns-1U||((r>=2U&&r<g->rows-3U&&!s->help)&&(c==3U||c==9U))||(r==g->rows-3U&&c==6U)){
    out[at++]=0x1dU;
    out[at++]=(unsigned char)(((r>=2U&&r<g->rows-3U&&!s->help&&!s->input_mode&&(c==0U||c==9U))||(r==g->rows-3U&&c==6U))?0xc0U:(r==0U||r>=g->rows-2U)?0xf8U:0xf0U);
   }else out[at++]=row[c];
  }
 }*used=at;return 0;
}
static int replace(TEDSTATE *s,unsigned int number,unsigned int width,const unsigned char *p,unsigned int n)
{
 unsigned char old[EDC_WIDTH],native[EDC_WIDTH],utf8[EDC_WIDTH];unsigned int len=0U,end,k,used;
 EDCLINE *line;
 if(number>s->candidate->count+1U)return n?8:0;
 if(number<=s->candidate->count){line=&s->candidate->line[number-1U];if(TEDEBC(line->text,line->length,old,sizeof old,&len))return 8;}
 while(n&&!p[n-1U])--n;
 if(!n&&number>s->candidate->count)return 0;
 if(s->left>EDC_WIDTH||n>EDC_WIDTH-s->left)return 8;
 end=s->left+width;
 if(len>end){if(end>EDC_WIDTH)return 8;used=len;memcpy(native,old,len);memset(native+s->left,0x40U,width);}
 else{used=s->left+n;memset(native,0x40U,used);if(len){k=len<s->left?len:s->left;memcpy(native,old,k);}}
 for(k=0U;k<n;++k)native[s->left+k]=p[k]?p[k]:0x40U;
 if(TEDUTF8(native,used,utf8,sizeof utf8,&used))return 8;
 if(number>s->candidate->count)return (int)EDCINSERT(s->candidate,s->candidate->count,utf8,used);
 line=&s->candidate->line[number-1U];memcpy(line->text,utf8,used);line->length=used;return 0;
}
int TESINPUT(TEDSTATE *s,const T27GEOMETRY *g,const unsigned char *in,unsigned int bytes,char command[257],unsigned int *aid,unsigned int *cursor)
{
 T27INPUTEVENT e;T27INPUTFIELD f[128];unsigned char prefix[38],seen[77];
 unsigned char text[256];unsigned int i,j,r,c,index,n,number,width,command_seen=0U;char op[3];
 if(!geometry(g)||!command||!aid||!cursor||T27INPUT(g,in,bytes,&e,f,128U)||e.kind!=T27_EVENT_KEY)return 8;
 if(e.aid!=0x7dU&&e.aid!=0xf1U&&e.aid!=0xf2U&&e.aid!=0xf3U&&e.aid!=0xf7U&&e.aid!=0xf8U&&e.aid!=0xf9U&&e.aid!=0x7aU&&e.aid!=0x7bU)return 20;
 memset(seen,0,sizeof seen);memset(prefix,0,sizeof prefix);command[0]=0;width=g->columns-11U;
 *s->candidate=*s->text;
 for(i=0U;i<e.fields;++i){
  r=f[i].address/g->columns;c=f[i].address%g->columns;n=f[i].length;
  if(r==g->rows-3U&&c==7U){
   if(command_seen++||n>g->columns-8U||n>256U)return 8;
   while(n&&!in[f[i].offset+n-1U])--n;
   for(j=0U;j<n;++j)text[j]=in[f[i].offset+j]?in[f[i].offset+j]:0x40U;
   if(TEDCDEC(text,n,command))return 8;command[n]=0;continue;
  }
  if(r<2U||r>=g->rows-3U||s->help||s->input_mode||(c!=1U&&c!=10U))return 8;
  index=(r-2U)*2U+(c==10U);if(seen[index]++)return 8;number=s->top+r-2U;
  if(c==1U){
   if(n>2U)return 8;op[0]=op[1]=' ';op[2]=0;
   if(n){unsigned char b[2];b[0]=in[f[i].offset]?in[f[i].offset]:0x40U;b[1]=n==2U&&in[f[i].offset+1U]?in[f[i].offset+1U]:0x40U;if(TEDCDEC(b,2U,op))return 8;}
   if(op[1]!=' '||(op[0]!=' '&&op[0]!='I'&&op[0]!='i'&&op[0]!='D'&&op[0]!='d'))return 8;
   prefix[r-2U]=(unsigned char)op[0];
  }else if(n>width||replace(s,number,width,in+f[i].offset,n))return 8;
 }
 for(r=g->rows-5U;r>0U;--r){number=s->top+r-1U;
  if(prefix[r-1U]=='I'||prefix[r-1U]=='i'){if(number>s->candidate->count||EDCINSERT(s->candidate,number,(const unsigned char *)"",0U))return 8;}
  if(prefix[r-1U]=='D'||prefix[r-1U]=='d'){if(EDCDELETE(s->candidate,number,number))return 8;}
 }
 TEDTAKE(s);*aid=e.aid;*cursor=e.cursor_valid?e.cursor:(g->rows-3U)*g->columns+7U;
 if(e.cursor_valid){r=e.cursor/g->columns;c=e.cursor%g->columns;
  if(r>=2U&&r<g->rows-3U&&(c==1U||c==2U||(c>=10U&&c<g->columns-1U))){number=s->top+r-2U;if(number<=s->text->count)s->text->current=number;}
  else *cursor=(g->rows-3U)*g->columns+7U;
 }return 0;
}
int TESFULL(const T27GEOMETRY *g,const unsigned char *modified,unsigned int bytes,
 const unsigned char *buffer,unsigned int buffered,unsigned char *out,unsigned int cap,unsigned int *used)
{
 T27INPUTEVENT e,b;T27INPUTFIELD f[128];unsigned int i,n,at=3U;
 if(!geometry(g)||!out||!used||cap<3U||bytes<3U||T27INPUT(g,modified,bytes,&e,f,128U)||e.kind!=T27_EVENT_KEY)return 8;
 memcpy(out,modified,3U);
 for(i=0U;i<e.fields;++i){
  if(at>cap||cap-at<3U)return 8;out[at++]=0x11U;
  if(T27ADDR(g,f[i].address,out+at))return 8;at+=2U;
  if(T27BUFFER(g,buffer,buffered,f[i].address,out+at,cap-at,&n,&b))return 8;at+=n;
 }
 *used=at;return 0;
}
