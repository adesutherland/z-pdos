/* SPDX-License-Identifier: MIT; native presentation uses explicit IBM1047. */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "twospace_submit.h"
#include "twospace_ui.h"
/* Exercise the actual private native adapter without a test product entry. */
#define system test_editor
#include "../../src/twospace_submit.c"
#undef system
static unsigned char wire[2048];
static unsigned int used,services,lines;
static unsigned int device,cap_error,acquire_error,release_error,phase,launches;
static const char *wanted;
unsigned int TUIIO(unsigned int n,void *p,unsigned int op)
{
 unsigned char *h=(unsigned char *)p;unsigned int i;
 ++services;assert(n==32U&&TJBWORD(h)==1U&&TJBWORD(h+4U)==32U);
 for(i=8U;i<n;++i)assert(!h[i]);
 if(op==TSA_IO_SCREEN_ACQUIRE){assert(!phase);phase=1U;return acquire_error;}
 assert(op==TSA_IO_SCREEN_RELEASE&&phase==1U);phase=2U;return release_error;
}
unsigned int TUIREAD(unsigned char *p,unsigned int cap,unsigned int *n)
{(void)p;(void)cap;(void)n;++services;return 20U;}
unsigned int TUILINE(const unsigned char *p,unsigned int n)
{if(!n||n>130U||n>sizeof wire-used)return 20U;memcpy(wire+used,p,n);used+=n;++lines;return 0U;}
unsigned int TUICAPS(unsigned char *p)
{++services;memset(p,0,64U);TJBPUT(p+TSA_CAP_DEVICE_CLASS,device);return cap_error;}
int test_editor(const char *command)
{assert(!strcmp(command,wanted));assert(phase==(device==TSA_DEVICE_3270?2U:0U));++launches;return 23;}
static void handoff(void)
{
 device=TSA_DEVICE_3270;wanted="EDIT PDOS.USER.CREXX(HELLO)";
 services=phase=launches=cap_error=acquire_error=release_error=0U;
 assert(developer_edit(0,"PDOS.USER.CREXX(HELLO)",0U)==23&&services==3U&&launches==1U);
 services=phase=launches=0U;cap_error=8U;
 assert(developer_edit(0,"PDOS.USER.CREXX(HELLO)",0U)==8&&services==1U&&!launches);
 services=phase=0U;cap_error=0U;acquire_error=16U;
 assert(developer_edit(0,"PDOS.USER.CREXX(HELLO)",0U)==16&&services==2U&&!launches);
 services=phase=0U;acquire_error=0U;release_error=12U;
 assert(developer_edit(0,"PDOS.USER.CREXX(HELLO)",0U)==12&&services==3U&&!launches);
 services=phase=0U;release_error=0U;device=TSA_DEVICE_LINE;wanted="EDIT PDOS.USER.CREXX(HELLO) LINE";
 assert(developer_edit(0,"PDOS.USER.CREXX(HELLO)",1U)==23&&services==1U&&launches==1U);
 puts("PD035 native editor handoff ordering, refusal and line controls PASS");
}
int main(void)
{
 static const unsigned char prefix[]={0xe2U,0xe4U,0xc2U,0xd4U,0xc9U,0xe3U};
 unsigned int i,left=0U,right=0U;
 if(TJFENTRY("HELP BATCH")||services||lines!=5U||used<sizeof prefix||memcmp(wire,prefix,sizeof prefix)){
  puts("FAIL: HELP wire is not IBM1047 or performed an unexpected service");return 1;}
 for(i=0U;i<used;++i){if(wire[i]==0xadU)++left;if(wire[i]==0xbdU)++right;}
 if(left!=6U||right!=6U){puts("FAIL: optional brackets are not IBM1047 AD/BD");return 1;}
 puts("PD033 native HELP IBM1047 wire/punctuation and no-service control PASS");handoff();return 0;
}
