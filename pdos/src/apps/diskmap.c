/* SPDX-License-Identifier: MIT; opt-in C client of DASD status and K panels. */
#include <stdio.h>
#include <string.h>
#include "twospace_ui.h"
#include "twospace_workbench.h"
#include "twospace_dasd.h"
static unsigned int word(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
static void put(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static void init(unsigned char *p,unsigned int bytes)
{memset(p,0,bytes);put(p,1U);put(p+4U,bytes);}
static void pointer(unsigned char *p,const void *v)
{put(p,0U);put(p+4U,(unsigned int)(unsigned long)v);}
int main(int argc,char **argv)
{
    unsigned char cap[64],request[64],map[TDA_TRACKS],panel[64],input[256],cells[10U*132U];
    char rows[9][132],title[132],bar[43],volser[7];TWBPLAN plan;
    unsigned int index=0U,volumes,rc,n,i,j,total,reserved,allocated,free,used,handle=0U,height,width,screen,mapwidth,maprows,rowcount,barwidth;
    (void)argc;(void)argv;
    if(TUIOPEN((const unsigned char *)"DASD storage / DISKMAP",sizeof "DASD storage / DISKMAP"-1U)||TUICAPS(cap))return 8;
    screen=word(cap+8U)==TSA_DEVICE_3270;
    if(screen&&TWBLAYOUT(word(cap+16U),word(cap+20U),&plan))return 8;
    width=screen?plan.body_width:78U;if(width>132U)width=132U;
    mapwidth=width-6U;if(mapwidth>50U)mapwidth=50U;
    maprows=(100U+mapwidth-1U)/mapwidth;rowcount=6U+maprows;
    barwidth=width-28U;if(barwidth>40U)barwidth=40U;
    for(;;){
        init(request,64U);put(request+8U,index);put(request+12U,sizeof map);pointer(request+56U,map);
        rc=TUIIO(64U,request,TSA_IO_DASD_STATUS);
        if(rc){sprintf(title,"DISKMAP: allocation view unavailable (status%u).",rc);TUILINE((const unsigned char *)title,(unsigned int)strlen(title));return (int)rc;}
        volumes=word(request+8U);if(!volumes||volumes>4U||index>=volumes)return 12;
        memcpy(volser,request+48U,6U);volser[6U]=0;
        total=word(request+24U);reserved=word(request+28U);allocated=word(request+32U);free=word(request+36U);used=reserved+allocated;
        if(total!=TDA_TRACKS||used>total||free!=total-used)return 12;
        sprintf(title,"DASD STORAGE / %s / %04X",volser,word(request+12U));
        sprintf(rows[0],"100 cylinders x 15 tracks / %u datasets",word(request+44U));
        sprintf(rows[1],"Allocated %u / reserved %u / free %u tracks",allocated,reserved,free);
        bar[0]='(';for(i=0U;i<barwidth;++i)bar[1U+i]=i<(used*barwidth+total-1U)/total?'#':'.';bar[barwidth+1U]=')';bar[barwidth+2U]=0;
        sprintf(rows[2],"%s %u%% allocated or reserved",bar,used*100U/total);
        sprintf(rows[3],"Largest unallocated run: %u tracks",word(request+40U));
        for(i=0U;i<maprows;++i){
            unsigned int take=100U-i*mapwidth;if(take>mapwidth)take=mapwidth;
            sprintf(rows[4U+i],"%02u-%02u ",i*mapwidth,i*mapwidth+take-1U);
            for(j=0U;j<take;++j){unsigned int k,bits=0U,count=0U;
                for(k=0U;k<15U;++k){unsigned int value=map[(i*mapwidth+j)*15U+k];bits|=value;if(value)++count;}
                rows[4U+i][6U+j]=(bits&TDA_RESERVED)?'R':!count?'.':count==15U?'#':'+';
            }rows[4U+i][6U+take]=0;
        }
        strcpy(rows[4U+maprows],"R reserved  # allocated  + mixed  . free");
        strcpy(rows[5U+maprows],"Enter refresh / N next volume / Q return");
        if(TUILINE((const unsigned char *)title,(unsigned int)strlen(title)))return 12;
        for(i=0U;i<rowcount;++i)if(TUILINE((const unsigned char *)rows[i],(unsigned int)strlen(rows[i])))return 12;
        if(screen){
            height=plan.body_rows<rowcount+1U?plan.body_rows:rowcount+1U;
            memset(cells,' ',width*(height-1U));
            for(i=0U;i<height-1U;++i){n=(unsigned int)strlen(rows[i]);if(n>width)n=width;memcpy(cells+i*width,rows[i],n);}
            init(panel,64U);put(panel+8U,handle);put(panel+12U,word(cap+60U));put(panel+20U,plan.body);put(panel+24U,1U);
            put(panel+28U,height);put(panel+32U,width);put(panel+36U,0U);put(panel+40U,(unsigned int)strlen(title));pointer(panel+44U,title);
            if(TUIPANEL(handle?TSA_IO_PANEL_UPDATE:TSA_IO_PANEL_CREATE,panel))return 12;handle=word(panel+8U);
            put(panel+52U,width*(height-1U));pointer(panel+56U,cells);if(TUIPANEL(TSA_IO_PANEL_TEXT,panel))return 12;
            put(panel+8U,0U);if(TUIPANEL(TSA_IO_PANEL_COMMIT,panel))return 12;
        }
        if(TUIREAD(input,sizeof input,&n))return 12;
        if(n==1U&&(input[0U]=='Q'||input[0U]=='q'))break;
        if(n==1U&&(input[0U]=='N'||input[0U]=='n'))index=(index+1U)%volumes;
    }
    if(handle){put(panel+8U,handle);if(TUIPANEL(TSA_IO_PANEL_CLOSE,panel))return 12;}
    return 0;
}
