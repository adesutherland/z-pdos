/* SPDX-License-Identifier: MIT; opt-in C client of DASD status and K panels. */
#include <stdio.h>
#include <stdlib.h>
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
    unsigned char cap[64],request[64],*map=0,panel[64],input[256],cells[10U*132U];
    char rows[9][132],title[132],bar[43],volser[7];TWBPLAN plan;
    unsigned int index=0U,volumes,rc,n,i,j,total,reserved,allocated,available,used,handle=0U,height,width,screen,mapwidth,maprows,rowcount,barwidth;
    unsigned int cylinders,capacity,group,count,labelwidth,maxmaprows;int result=12;
    (void)argc;(void)argv;
    if(TUIOPEN((const unsigned char *)"DASD storage / DISKMAP",sizeof "DASD storage / DISKMAP"-1U)||TUICAPS(cap))return 8;
    screen=word(cap+8U)==TSA_DEVICE_3270;
    if(screen&&TWBLAYOUT(word(cap+16U),word(cap+20U),&plan))return 8;
    width=screen?plan.body_width:78U;if(width>132U)width=132U;
    if(width<=28U||(screen&&plan.body_rows<8U))return 8;
    maxmaprows=screen?plan.body_rows-7U:3U;if(maxmaprows>3U)maxmaprows=3U;
    barwidth=width-28U;if(barwidth>40U)barwidth=40U;
    for(;;){
        init(request,64U);put(request,TDA_GEOMETRY_QUERY);put(request+8U,index);
        rc=TUIIO(64U,request,TSA_IO_DASD_STATUS);
        if(rc)goto unavailable;
        capacity=word(request+24U);
        if(!capacity||capacity>TDA_MAX_TRACKS)goto done;
        map=(unsigned char *)malloc(capacity);if(!map){result=4;goto done;}
        init(request,64U);put(request+8U,index);put(request+12U,capacity);pointer(request+56U,map);
        rc=TUIIO(64U,request,TSA_IO_DASD_STATUS);
        if(rc)goto unavailable;
        volumes=word(request+8U);if(!volumes||volumes>4U||index>=volumes)goto done;
        memcpy(volser,request+48U,6U);volser[6U]=0;
        cylinders=word(request+16U);total=word(request+24U);reserved=word(request+28U);allocated=word(request+32U);available=word(request+36U);
        if(cylinders<3U||cylinders>TDA_MAX_CYLINDERS||word(request+20U)!=TDA_HEADS||
           total!=cylinders*TDA_HEADS||total>capacity||reserved>total||allocated>total-reserved)goto done;
        used=reserved+allocated;if(available!=total-used)goto done;
        labelwidth=cylinders<=100U?6U:12U;mapwidth=width-labelwidth;if(mapwidth>50U)mapwidth=50U;
        group=(cylinders+mapwidth*maxmaprows-1U)/(mapwidth*maxmaprows);count=(cylinders+group-1U)/group;
        maprows=(count+mapwidth-1U)/mapwidth;rowcount=6U+maprows;
        sprintf(title,"DASD STORAGE / %s / %04X",volser,word(request+12U));
        sprintf(rows[0],"%u cylinders x 15 tracks / %u datasets",cylinders,word(request+44U));
        if(group>1U)sprintf(rows[0]+strlen(rows[0])," / %u cyl/cell",group);
        sprintf(rows[1],"Allocated %u / reserved %u / free %u tracks",allocated,reserved,available);
        bar[0]='(';for(i=0U;i<barwidth;++i)bar[1U+i]=i<(used*barwidth+total-1U)/total?'#':'.';bar[barwidth+1U]=')';bar[barwidth+2U]=0;
        sprintf(rows[2],"%s %u%% allocated or reserved",bar,used*100U/total);
        sprintf(rows[3],"Largest unallocated run: %u tracks",word(request+40U));
        for(i=0U;i<maprows;++i){
            unsigned int take=count-i*mapwidth,first,last;if(take>mapwidth)take=mapwidth;
            first=i*mapwidth*group;last=(i*mapwidth+take)*group;if(last>cylinders)last=cylinders;
            sprintf(rows[4U+i],cylinders<=100U?"%02u-%02u ":"%05u-%05u ",first,last-1U);
            for(j=0U;j<take;++j){unsigned int k,bits=0U,occupied=0U;
                first=(i*mapwidth+j)*group*TDA_HEADS;last=first+group*TDA_HEADS;if(last>total)last=total;
                for(k=first;k<last;++k){unsigned int value=map[k];bits|=value;if(value)++occupied;}
                rows[4U+i][labelwidth+j]=(bits&TDA_RESERVED)?'R':!occupied?'.':occupied==last-first?'#':'+';
            }rows[4U+i][labelwidth+take]=0;
        }
        free(map);map=0;
        strcpy(rows[4U+maprows],"R reserved  # allocated  + mixed  . free");
        strcpy(rows[5U+maprows],"Enter refresh / N next volume / Q return");
        if(TUILINE((const unsigned char *)title,(unsigned int)strlen(title)))goto done;
        for(i=0U;i<rowcount;++i)if(TUILINE((const unsigned char *)rows[i],(unsigned int)strlen(rows[i])))goto done;
        if(screen){
            height=plan.body_rows<rowcount+1U?plan.body_rows:rowcount+1U;
            memset(cells,' ',width*(height-1U));
            for(i=0U;i<height-1U;++i){n=(unsigned int)strlen(rows[i]);if(n>width)n=width;memcpy(cells+i*width,rows[i],n);}
            init(panel,64U);put(panel+8U,handle);put(panel+12U,word(cap+60U));put(panel+20U,plan.body);put(panel+24U,1U);
            put(panel+28U,height);put(panel+32U,width);put(panel+36U,0U);put(panel+40U,(unsigned int)strlen(title));pointer(panel+44U,title);
            if(TUIPANEL(handle?TSA_IO_PANEL_UPDATE:TSA_IO_PANEL_CREATE,panel))goto done;handle=word(panel+8U);
            put(panel+52U,width*(height-1U));pointer(panel+56U,cells);if(TUIPANEL(TSA_IO_PANEL_TEXT,panel))goto done;
            put(panel+8U,0U);if(TUIPANEL(TSA_IO_PANEL_COMMIT,panel))goto done;
        }
        if(TUIREAD(input,sizeof input,&n))goto done;
        if(n==1U&&(input[0U]=='Q'||input[0U]=='q'))break;
        if(n==1U&&(input[0U]=='N'||input[0U]=='n'))index=(index+1U)%volumes;
    }
    result=0;goto done;
unavailable:
    if(map){free(map);map=0;}
    sprintf(title,"DISKMAP: allocation view unavailable (status%u).",rc);
    TUILINE((const unsigned char *)title,(unsigned int)strlen(title));result=(int)rc;
done:
    if(map)free(map);
    if(handle){put(panel+8U,handle);if(TUIPANEL(TSA_IO_PANEL_CLOSE,panel))result=12;}
    return result;
}
