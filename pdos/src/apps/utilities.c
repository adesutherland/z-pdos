/* SPDX-License-Identifier: MIT; small record-aware U utilities. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "twospace_ui.h"
#include "twospace_recordio.h"
#include "utilities.h"
static unsigned int word(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
static unsigned int half(const unsigned char *p)
{return ((unsigned int)p[0]<<8)|p[1];}
static void line(const char *p)
{TUILINE((const unsigned char *)p,(unsigned int)strlen(p));}
int UTLRUN(unsigned int kind,int argc,char **argv)
{
    unsigned char *a,*b,info[TRI_BYTES];unsigned int bytes,other=0U,at=0U,bt=0U,n,m,i,j,k,num=0U,found=0U,rc;
    char text[256];int result=0;
    if((kind==1U||kind==3U)?argc!=3:argc!=2){printf("%s\n",kind==1U?"FIND literal dataset":kind==2U?"HEX dataset":"CMP dataset dataset");return 8;}
    a=(unsigned char *)malloc(TRI_LIMIT);b=(unsigned char *)malloc(TRI_LIMIT);
    if(!a||!b){free(a);free(b);return 4;}
    if(TUIOPEN((const unsigned char *)"Record utilities",16U)){result=8;goto done;}
    rc=TRIFILE(TRI_LOAD,argv[kind==1U?2:1],a,TRI_LIMIT,info);bytes=word(info+16U);
    if(rc){sprintf(text,"Utility: source refused (%u); bounded PS FB/VB required",rc);line(text);result=8;goto done;}
    if(kind==3U){rc=TRIFILE(TRI_LOAD,argv[2],b,TRI_LIMIT,info);other=word(info+16U);
        if(rc){sprintf(text,"CMP: second source refused (%u)",rc);line(text);result=8;goto done;}}
    while(at<bytes){
        n=half(a+at);at+=2U;++num;
        if(kind==1U){m=(unsigned int)strlen(argv[1]);
            for(i=0U;m&&m<=n&&i<=n-m;++i)if(!memcmp(a+at+i,argv[1],m)){
                sprintf(text,"FIND: record %u column %u",num,i+1U);line(text);
                for(j=0U;j<n;j+=k){k=n-j;if(k>256U)k=256U;TUILINE(a+at+j,k);}++found;break;}
        }else if(kind==2U){
            sprintf(text,"HEX: record %u, %u bytes",num,n);line(text);
            for(i=0U;i<n;i+=16U){sprintf(text,"%04X:",i);j=(unsigned int)strlen(text);
                for(k=i;k<n&&k<i+16U;++k){sprintf(text+j," %02X",a[at+k]);j+=3U;}line(text);}
        }else{
            if(bt>=other){result=4;break;}m=half(b+bt);bt+=2U;
            if(n!=m||memcmp(a+at,b+bt,n)){result=4;break;}bt+=m;
        }
        at+=n;
    }
    if(kind==1U){sprintf(text,"FIND: %u matching records",found);line(text);result=found?0:4;}
    if(kind==3U){if(bt!=other)result=4;
        if(result)sprintf(text,"CMP: files differ at record %u",num?num:1U);else strcpy(text,"CMP: logical records identical");line(text);}
done:
    free(a);free(b);return result;
}
