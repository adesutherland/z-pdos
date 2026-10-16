/* SPDX-License-Identifier: MIT; host oracle control using actual EDIT paint. */
#include <stdio.h>
#include <stdlib.h>
#include "apps/edit_screen.h"
#include "apps/edit_text.h"
static unsigned int absent(unsigned int a,const char *b,unsigned char *c,unsigned int d,unsigned char *e)
{(void)a;(void)b;(void)c;(void)d;(void)e;return 20U;}
int main(int argc,char **argv)
{
 TEDSTATE s;T27GEOMETRY g;unsigned char out[TES_MAX];unsigned int n,rows,cols;
 char command[2];unsigned char accent=0x51U,letter=0xc1U;
 if(!TEDCDEC(&accent,1U,command)||TEDCDEC(&letter,1U,command)||command[0]!='A')return 7;
 if(argc!=2)return 2;rows=atoi(argv[1])==5?27U:24U;cols=rows==27U?132U:80U;
 if(TEDINIT(&s,absent)||T27GEOM(&g,rows,cols,T27_CODED12))return 3;
 if(EDCINSERT(s.text,0U,(const unsigned char *)"A",1U))return 4;
 if(TESPAINT(&s,&g,out,sizeof out,&n,(rows-3U)*cols+7U))return 5;
 if(putchar(0xf1)==EOF||fwrite(out,1U,n,stdout)!=n)return 6;
 TEDFREE(&s);return 0;
}
