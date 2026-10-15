/* SPDX-License-Identifier: MIT; native line binding and editor/key cleanup. */
#define main console_protocol_test
#include "submit.c"
#undef main
#include "twospace_batchui.h"
#include "twospace_ui.h"
#include "twospace_workbench.h"
static unsigned int key_at,key_count,form_keys,shell_keys,wire_lines,read_failure,cap_failure,key_failure;
static const char *keys[20];
unsigned int TUICAPS(unsigned char *p)
{if(cap_failure)return 12U;memset(p,0,64U);TJBPUT(p+8U,TSA_DEVICE_LINE);TJBPUT(p+60U,1U);return 0;}
unsigned int TUICTRL(unsigned int flags)
{if(flags==TSA_UI_KEYS_FORM){if(key_failure)return 12U;++form_keys;}else if(flags==TSA_UI_KEYS_SHELL)++shell_keys;else assert(0);return 0;}
unsigned int TUILINE(const unsigned char *p,unsigned int n)
{unsigned int i;assert(n&&n<=132U);for(i=0U;i<n;++i)assert(p[i]>=0x40U&&p[i]!=0xffU);++wire_lines;return 0;}
unsigned int TUIKEY(unsigned char *p,unsigned int cap,unsigned int *n,unsigned int *aid)
{assert(key_at<key_count);if(read_failure&&key_at==key_count-1U){++key_at;return 12U;}*n=(unsigned int)strlen(keys[key_at++]);assert(*n<=cap);assert(!TJCENC(keys[key_at-1U],*n,p));*aid=0x7dU;return 0;}
unsigned int TUISET(const unsigned char *p,unsigned int n,unsigned int cursor)
{(void)p;(void)n;(void)cursor;assert(0);return 20U;}
unsigned int TUIPANEL(unsigned int op,unsigned char *p)
{(void)op;(void)p;assert(0);return 20U;}
int main(void)
{
 TJFOPS ops;TWBPLAN layout;unsigned int i,rows[]={24U,32U,43U,27U},cols[]={80U,80U,80U,132U};
 for(i=0U;i<4U;++i){assert(!TWBLAYOUT(rows[i],cols[i],&layout));assert(layout.body_rows>=3U&&layout.body+layout.body_rows==layout.position&&layout.body_width>=78U);}
 setup();memset(&ops,0,sizeof ops);ops.load=load;ops.service=service;ops.input=console;ops.line=message;ops.record=record;
 keys[0]="DECK APP.JCL(CASE)";keys[1]="RUNHOLD YES";keys[2]="PREVIEW";keys[3]="";
 keys[4]="SUBMIT";keys[5]="SUBMIT";keys[6]="EXIT";key_count=7U;
 assert(!TBFENTRY(&state,&ops,"")&&key_at==key_count&&submitted==1U&&!runs&&form_keys==1U&&shell_keys==1U&&wire_lines);
 /* Leaving/re-entering the forms is not a new submission token. */
 key_at=0U;keys[0]="SUBMIT";keys[1]="PREVIEW";keys[2]="EXIT";key_count=3U;
 assert(!TBFENTRY(&state,&ops,"JOBS")&&submitted==1U&&form_keys==2U&&shell_keys==2U);
 /* An input error returns the real failure and still restores key ownership. */
 key_at=0U;keys[0]="NEW";keys[1]="PREVIEW";keys[2]="";key_count=3U;read_failure=1U;
 assert(TBFENTRY(&state,&ops,"")==12&&submitted==1U&&!state.result.valid&&form_keys==3U&&shell_keys==3U);
 cap_failure=1U;assert(TBFENTRY(&state,&ops,"PREVIEW")==12&&!state.result.valid&&form_keys==3U&&shell_keys==3U);
 cap_failure=0U;key_failure=1U;assert(TBFENTRY(&state,&ops,"PREVIEW")==12&&!state.result.valid&&form_keys==3U&&shell_keys==3U);
 puts("PD033 native line forms/re-entry/error cleanup and model layout controls PASS");return 0;
}
