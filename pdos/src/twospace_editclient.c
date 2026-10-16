/* SPDX-License-Identifier: MIT; checked EDIT request construction in U. */
#include <string.h>
#include <ctype.h>
#include "twospace_ui.h"
#include "twospace_editfile.h"
#include "apps/edit_text.h"
static void put(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
unsigned int TEFFILE(unsigned int action,const char *name,unsigned char *data,
                     unsigned int capacity,unsigned char info[TEF_BYTES])
{
 unsigned char h[TEF_BYTES];char text[68],*p,*colon,*member,*end;unsigned int i,n,rc;
 if(!name||!info||action>TEF_NEW||capacity>TEF_LIMIT||(capacity&&!data)||sizeof(void *)!=4U)return 8U;
 n=(unsigned int)strlen(name);if(!n||n>=sizeof text)return 8U;
 for(i=0U;i<=n;++i)text[i]=(char)toupper((unsigned char)name[i]);p=text;
 if(*p=='\''){if(n<3U||p[n-1U]!='\'')return 8U;p[n-1U]=0;++p;}
 memset(h,0,sizeof h);memset(h+40U,0x40U,58U);
 colon=strchr(p,':');if(colon){if(colon-p!=6||TEDCENC(p,6U,h+92U))return 8U;p=colon+1;}
 member=strchr(p,'(');
 if(member){end=strchr(member,')');if(!end||end[1]||end==member+1||end-member>9)return 8U;
  *member++=0;*end=0;if(TEDCENC(member,(unsigned int)strlen(member),h+84U))return 8U;}
 n=(unsigned int)strlen(p);if(!n||n>44U||TEDCENC(p,n,h+40U))return 8U;
 put(h,1U);put(h+4U,TEF_BYTES);put(h+8U,action);put(h+12U,capacity);
 if(action!=TEF_LOAD){memcpy(h+20U,info+20U,12U);memcpy(h+104U,info+104U,32U);}
 put(h+140U,(unsigned int)(unsigned long)data);
 rc=TUIIO(TEF_BYTES,h,TSA_IO_EDIT_FILE);if(!rc)memcpy(info,h,TEF_BYTES);return rc;
}
