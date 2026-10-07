/* SPDX-License-Identifier: MIT; unchanged full native workload loader comparison. */
#include "twospace_tso.h"
#include "pdosutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int int_rdblock(int a,int b,int c,int d,void *e,int f,int g)
{(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;abort();return -1;}
int main(int argc,char **argv)
{
    unsigned char *raw,*image;char *legacy;FILE *f;size_t bytes;
    unsigned int base,i,mode;int length,entry,amode,rmode;TSTINFO info;
    if(argc!=3)return 2;mode=(unsigned int)atoi(argv[2]);
    raw=(unsigned char *)malloc(TST_MAX_RAW+1U);
    image=(unsigned char *)malloc(TST_MAX_IMAGE);
    legacy=(char *)malloc(TST_MAX_RAW>TST_MAX_IMAGE?TST_MAX_RAW:TST_MAX_IMAGE);
    if(!raw||!image||!legacy)return 2;
    f=fopen(argv[1],"rb");if(!f)return 2;
    bytes=fread(raw,1,TST_MAX_RAW+1U,f);
    if(!bytes||bytes>TST_MAX_RAW||ferror(f)||!feof(f)||fclose(f))return 2;
    for(i=0;i<2U;++i){
        base=mode==24U?0x20000U+i*0x200000U:0x20000000U+i*0x2000000U;
        if((mode==24U?TSTIMAGE24(raw,(unsigned int)bytes,base,image,TST_MAX_IMAGE,&info):
            mode==31U?TSTIMAGE31(raw,(unsigned int)bytes,base,image,TST_MAX_IMAGE,&info):
            TSTIMAGE64ANY(raw,(unsigned int)bytes,base,image,TST_MAX_IMAGE,&info))!=TST_OK)return 1;
        memcpy(legacy,raw,bytes);length=(int)bytes;
        if(fixPEMode(legacy,&length,&entry,(int)base,TST_MAX_RAW,&amode,&rmode)||
           length!=(int)info.image_bytes||amode!=(mode==24U?0:mode==31U?2:1)||
           memcmp(image,legacy,info.image_bytes))return 1;
        printf("native %u: %lu record bytes, %u image bytes at %08x match released loader\n",mode,(unsigned long)bytes,info.image_bytes,base);
    }
    free(raw);free(image);free(legacy);return 0;
}
