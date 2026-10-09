/* SPDX-License-Identifier: MIT; layout policy shared by U and the K compositor. */
#include "twospace_workbench.h"
#include "twospace_display.h"
int TWBLAYOUT(unsigned int rows,unsigned int cols,TWBPLAN *p)
{
    unsigned int dock,keys;
    if(!p||rows<24U||rows>256U||cols<80U||cols>TWF_MAX_CELLS/rows)return -1;
    dock=(260U+cols-1U)/cols;keys=cols<120U?2U:1U;
    p->wide=cols>=100U;p->input=(rows-1U-keys-dock)*cols+4U;
    p->heading=rows-2U-keys-dock;p->status=rows-1U-keys;p->keys=rows-keys;p->body=3U;
    if(p->wide){p->position=p->heading-1U;p->body_rows=p->position-p->body;p->shell_column=cols-43U;p->shell_row=p->body;p->shell_rows=p->body_rows;p->shell_width=42U;p->body_width=cols-47U;}
    else{p->position=p->heading-4U;p->body_rows=p->position-p->body;p->shell_column=1U;p->shell_row=p->position+2U;p->shell_rows=2U;p->shell_width=cols-2U;p->body_width=cols-2U;}
    return p->body_rows&&p->body_rows<=TPH_VIEW_ROWS?0:-1;
}
typedef struct {unsigned char data[256];unsigned int bytes;} LABEL;
static int label(LABEL *l,const char *s)
{unsigned int n;if(TWDLabel(s,l->data+l->bytes,sizeof l->data-l->bytes,&n))return -1;l->bytes+=n;return 0;}
static int number(LABEL *l,unsigned int n)
{unsigned char digits[10];unsigned int i=0U;do{digits[i++]=(unsigned char)(0xf0U+n%10U);n/=10U;}while(n);if(i>sizeof l->data-l->bytes)return -1;while(i)l->data[l->bytes++]=digits[--i];return 0;}
static int raw(LABEL *l,const unsigned char *p,unsigned int n)
{unsigned int i;if((n&&!p)||n>sizeof l->data-l->bytes)return -1;for(i=0U;i<n;++i){if(p[i]<0x40U||p[i]==0xffU)return -1;l->data[l->bytes++]=p[i];}return 0;}
static int line(TWFSURFACE *s,unsigned int row,unsigned int col,unsigned int width,LABEL *l,unsigned int role)
{
    unsigned int n=l->bytes;
    if(n>width){n=width;if(n>=3U)l->data[n-1U]=l->data[n-2U]=l->data[n-3U]=0x4bU;}
    return TWFTEXT(s,row,col,width,l->data,n,role);
}
static int words(TWFSURFACE *s,unsigned int row,unsigned int col,unsigned int width,const char *text,unsigned int role)
{LABEL l;l.bytes=0U;if(label(&l,text))return -1;return line(s,row,col,width,&l,role);}
static int result(LABEL *l,const TWBCONTEXT *c)
{
    if(!c->phase)return label(l,"No completed call");
    if(c->phase!=TWB_DONE)return label(l,"OS=PENDING RC=PENDING");
    if(label(l,"OS=")||number(l,c->os)||label(l," RC="))return -1;
    if(!c->rc_valid)return label(l,"unavailable");
    if(c->rc&0x80000000U){if(label(l,"-"))return -1;return number(l,(~c->rc)+1U);}
    return number(l,c->rc);
}
int TWBCOMPOSE(TWFSURFACE *s,const TWBPLAN *p,const TWBCONTEXT *c,const TPHSTATE *history,TPHVIEW *program,TPHVIEW *shell,unsigned char *scratch,unsigned int capacity)
{
    unsigned int i,n,row,cols,role,prompt;LABEL l;T27GEOMETRY g;unsigned char *text,*kind,*roles;
    if(!s||!p||!c||!history||!program||!shell||!scratch||c->draft_bytes>256U||(c->draft_bytes&&!c->draft)||
       (c->cursor_valid&&c->cursor>=s->geometry.cells))return -1;
    g=s->geometry;text=s->text;kind=s->kind;roles=s->role;cols=g.columns;
    prompt=c->waiting||c->phase==TWB_DONE||!c->phase;
    if(TWFINIT(s,&g,text,kind,roles)||TWFINPUT(s,p->input,256U))return -1;
    for(i=0U;i<c->draft_bytes;++i)s->text[p->input+i]=c->draft[i];
    s->cursor=c->cursor_valid?c->cursor:p->input;s->cursor_valid=1U;
    s->role[0U]=TWF_HEADER;
    l.bytes=0U;if(raw(&l,c->title,c->title_bytes)||line(s,0U,1U,cols-2U,&l,TWF_HEADER))return -1;
    l.bytes=0U;if(label(&l,"Volume ")||raw(&l,c->volume,c->volume_bytes)||label(&l,"   Command #")||number(&l,c->job)||label(&l,"   ")||result(&l,c)||line(s,1U,1U,cols-2U,&l,c->os?TWF_ERROR:TWF_NORMAL))return -1;
    l.bytes=0U;if(label(&l,"OUTPUT / ")||raw(&l,c->name,c->name_bytes)||label(&l,c->phase==TWB_DONE?"  COMPLETED  ":c->phase==TWB_ACTIVE?"  ACTIVE  ":c->phase==TWB_DISPATCH?"  STARTING  ":"  READY  ")||result(&l,c))return -1;
    s->role[2U*cols]=c->focus?TWF_FOCUS:TWF_HEADER;
    if(line(s,2U,1U,p->body_width,&l,c->os?TWF_ERROR:c->focus?TWF_FOCUS:TWF_HEADER))return -1;
    if(TPHRENDER(history,program,p->body_width,p->body_rows,scratch,capacity,&n))return -1;
    for(i=0U;i<p->body_rows;++i)if(TWFTEXT(s,p->body+i,1U,p->body_width,scratch+i*p->body_width,p->body_width,TWF_NORMAL))return -1;
    if(p->wide){
        for(row=2U;row<p->position;++row){s->kind[row*cols+p->shell_column-1U]=TWF_ATTRIBUTE;s->role[row*cols+p->shell_column-1U]=row==2U&&!c->focus?TWF_FOCUS:TWF_SHELL;}
        if(words(s,2U,p->shell_column,p->shell_width,"SHELL / PCOMM",!c->focus?TWF_FOCUS:TWF_HEADER))return -1;
    }else{s->role[(p->position+1U)*cols]=!c->focus?TWF_FOCUS:TWF_HEADER;if(words(s,p->position+1U,1U,p->shell_width,"SHELL / RECENT COMMAND",!c->focus?TWF_FOCUS:TWF_HEADER))return -1;}
    if((!p->wide&&shell->follow?TPHCOMPACT(history,shell,p->shell_width,p->shell_rows,scratch,capacity,&n):TPHRENDER(history,shell,p->shell_width,p->shell_rows,scratch,capacity,&n)))return -1;
    for(i=0U;i<p->shell_rows;++i)if(TWFTEXT(s,p->shell_row+i,p->shell_column,p->shell_width,scratch+i*p->shell_width,p->shell_width,TWF_SHELL))return -1;
    l.bytes=0U;if(label(&l,c->help?"Help: PF1 Close":program->follow?"Output: Latest":"Output: History")||label(&l,"   Records ")||number(&l,history->count)||label(&l,"   Trimmed ")||number(&l,history->evicted)||line(s,p->position,1U,p->body_width,&l,program->evicted?TWF_ACTION:TWF_HEADER))return -1;
    s->role[p->heading*cols]=TWF_HEADER;
    if(words(s,p->heading,1U,cols-2U,!prompt?c->phase==TWB_DISPATCH?"PROGRAM STARTING / Input at the next prompt":"PROGRAM RUNNING / Input at the next prompt":c->input_program?"PROGRAM INPUT / Enter replies":"COMMAND / PCOMM / Enter submits",TWF_HEADER)||words(s,p->input/cols,1U,2U,">",TWF_ACTION))return -1;
    if(!c->input_program&&c->joined){l.bytes=0U;if(label(&l,"CONTINUE COMMAND / ")||number(&l,c->joined-1U)||label(&l," of 198 characters / Clear cancels")||line(s,p->heading,1U,cols-2U,&l,TWF_ACTION))return -1;}
    l.bytes=0U;if(label(&l,c->phase==TWB_DISPATCH?"Starting":!prompt?"Running":c->input_program?"Program input":"Ready")||label(&l,c->focus?"  Focus: OUTPUT":"  Focus: SHELL")||label(&l,c->source?"  Input: MONITOR/":"  Input: PRIMARY/"))return -1;
    if(c->input_name_bytes){if(raw(&l,c->input_name,c->input_name_bytes))return -1;}else if(label(&l,c->input_program?"PROGRAM":"PCOMM"))return -1;
    if(label(&l,c->help?"  Help":(c->focus?program->follow:shell->follow)?"  Latest":"  History"))return -1;
    if(c->capture_gap&&label(&l,"  CAPTURE GAP"))return -1;
    s->role[p->status*cols]=TWF_HEADER;if(line(s,p->status,1U,cols-2U,&l,c->capture_gap?TWF_ACTION:TWF_NORMAL))return -1;
    if(c->source&&prompt){
        if(words(s,p->keys,1U,cols-2U,"MONITOR input selected. Clear returns to primary input.",TWF_ACTION))return -1;
        if(p->keys+1U<g.rows&&words(s,p->keys+1U,1U,cols-2U,c->input_program?"The program owns this reply.":"Use CONSOLE INPUT PRIMARY to return to the screen.",TWF_NORMAL))return -1;
    }else if(!prompt){
        if(words(s,p->keys,1U,cols-2U,"Input actions become available at the next prompt.",TWF_ACTION))return -1;
        if(p->keys+1U<g.rows&&words(s,p->keys+1U,1U,cols-2U,"Execution remains synchronous; status reflects actual calls.",TWF_NORMAL))return -1;
    }else if(p->keys+1U<g.rows){
        l.bytes=0U;if(label(&l,c->input_program?"Enter Reply  PF1 ":"Enter Run  PF1 ")||label(&l,c->help?"Close":"Help")||label(&l,c->input_program?"  PF2 Edit  PF7 Up  PF8 Down":"  PF2 Edit  PF5 Older  PF6 Newer  PF7 Up  PF8 Down")||line(s,p->keys,1U,cols-2U,&l,TWF_ACTION)||
           words(s,p->keys+1U,1U,cols-2U,"PF9 Latest  PF10 Output/Shell",TWF_ACTION))return -1;
        s->role[(p->keys+1U)*cols]=TWF_ACTION;
    }else{l.bytes=0U;if(label(&l,c->input_program?"Enter Reply  PF1 ":"Enter Run  PF1 ")||label(&l,c->help?"Close":"Help")||label(&l,c->input_program?"  PF2 Edit  PF7 Up  PF8 Down  PF9 Latest  PF10 Output/Shell":"  PF2 Edit  PF5 Older  PF6 Newer  PF7 Up  PF8 Down  PF9 Latest  PF10 Output/Shell")||line(s,p->keys,1U,cols-2U,&l,TWF_ACTION))return -1;}
    s->role[p->keys*cols]=TWF_ACTION;
    if(c->help){
        static const char *help[9]={"WORKBENCH HELP","Enter submits a command or answers the active program.","PF1 closes help. PF2 returns to the input editor.","PF7/PF8 page the focused view. PF9 returns to Latest.","PF10 toggles output and shell focus.","PF5/PF6 recall PCOMM commands at the shell prompt.","CONSOLE INPUT PRIMARY or MONITOR selects shell input.","Colour follows advertised terminal capabilities.","Teletype output stays ordered and contains no screen chrome."};
        for(i=0U;i<p->body_rows;++i){role=i?TWF_NORMAL:TWF_HEADER;s->role[(p->body+i)*cols]=(unsigned char)role;if(words(s,p->body+i,1U,p->body_width,i<9U?help[i]:"",role))return -1;}
    }
    return 0;
}
