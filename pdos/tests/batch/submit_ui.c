/* SPDX-License-Identifier: MIT; native presentation uses explicit IBM1047. */
#include <stdio.h>
#include <string.h>
#include "twospace_submit.h"
#include "twospace_ui.h"
static unsigned char wire[2048];
static unsigned int used,services,lines;
unsigned int TUIIO(unsigned int n,void *p,unsigned int op)
{(void)n;(void)p;(void)op;++services;return 20U;}
unsigned int TUIREAD(unsigned char *p,unsigned int cap,unsigned int *n)
{(void)p;(void)cap;(void)n;++services;return 20U;}
unsigned int TUILINE(const unsigned char *p,unsigned int n)
{if(!n||n>130U||n>sizeof wire-used)return 20U;memcpy(wire+used,p,n);used+=n;++lines;return 0U;}
int main(void)
{
 static const unsigned char prefix[]={0xe2U,0xe4U,0xc2U,0xd4U,0xc9U,0xe3U};
 unsigned int i,left=0U,right=0U;
 if(TJFENTRY("HELP BATCH")||services||lines!=5U||used<sizeof prefix||memcmp(wire,prefix,sizeof prefix)){
  puts("FAIL: HELP wire is not IBM1047 or performed an unexpected service");return 1;}
 for(i=0U;i<used;++i){if(wire[i]==0xadU)++left;if(wire[i]==0xbdU)++right;}
 if(left!=6U||right!=6U){puts("FAIL: optional brackets are not IBM1047 AD/BD");return 1;}
 puts("PD033 native HELP IBM1047 wire/punctuation and no-service control PASS");return 0;
}
