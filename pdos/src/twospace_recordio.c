/* SPDX-License-Identifier: MIT; U record-file client, no file policy in K. */
#include <string.h>
#include "twospace_ui.h"
#include "twospace_recordio.h"
static void put(unsigned char *p,unsigned int v)
{p[0]=(unsigned char)(v>>24);p[1]=(unsigned char)(v>>16);p[2]=(unsigned char)(v>>8);p[3]=(unsigned char)v;}
unsigned int TRIFILE(unsigned int action,const char *name,unsigned char *data,
                     unsigned int capacity,unsigned char *info)
{
    unsigned int n,i;const char *colon;
    if(!info)return 8U;memset(info,0,TRI_BYTES);
    if(!name||action>TRI_SAVE_EMPTY||capacity>TRI_LIMIT||sizeof(void *)!=4U)return 8U;
    colon=strchr(name,':');
    if(colon){if(colon-name!=6)return 8U;
        for(i=0U;i<6U;++i){unsigned char c=(unsigned char)name[i];
            if(c>='a'&&c<='i')c=(unsigned char)(c-'a'+'A');
            else if(c>='j'&&c<='r')c=(unsigned char)(c-'j'+'J');
            else if(c>='s'&&c<='z')c=(unsigned char)(c-'s'+'S');
            info[20U+i]=c;
        }put(info+36U,1U);name=colon+1;
    }
    n=(unsigned int)strlen(name);if(!n||n>44U)return 8U;
    put(info,1U);put(info+4U,TRI_BYTES);
    put(info+8U,action);put(info+12U,capacity);put(info+84U,n);
    for(i=0U;i<n;++i){unsigned char c=(unsigned char)name[i];
        /* Mainframe execution character sets have separated letter ranges. */
        if(c>='a'&&c<='i')c=(unsigned char)(c-'a'+'A');
        else if(c>='j'&&c<='r')c=(unsigned char)(c-'j'+'J');
        else if(c>='s'&&c<='z')c=(unsigned char)(c-'s'+'S');
        info[40U+i]=c;
    }
    put(info+92U,(unsigned int)(unsigned long)data);
    return TUIIO(TRI_BYTES,info,TSA_IO_RECORD_FILE);
}
