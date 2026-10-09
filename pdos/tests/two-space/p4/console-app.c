/* SPDX-License-Identifier: MIT; opt-in U31 client of the reusable C console. */
#include <stdio.h>
#include <string.h>
#include "twospace_ui.h"
#include "twospace_console.h"
#include "twospace_3270.h"
extern int P4LINK(void);
extern int P4WIDE(void);
extern void *P4LOW(void);
extern unsigned int P4PUT(unsigned int,void *);
extern unsigned int P4GET(unsigned int,void *);
extern unsigned int P4READ(unsigned int,void *,unsigned int *);
#ifndef P4_TEST_MODE
#define P4_TEST_MODE 0
#endif
static unsigned int word(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
int main(int argc,char **argv)
{
    unsigned char caps[64],status[32],input[256],line[256],saved[64],request[32];unsigned int n,i,j,bytes;
    (void)argc;(void)argv;
    if(TUIOPEN((const unsigned char *)"Console qualification",21U)||TUICAPS(caps))return 40;
    memcpy(saved,caps,64U);
    if(TUIIO(63U,caps,TSA_IO_CAPABILITIES)!=8U||memcmp(saved,caps,64U))return 56;
    if(word(caps+8U)==TSA_DEVICE_LINE){
        memset(request,0,32U);request[3U]=1U;request[7U]=32U;
        if(TUIIO(32U,request,TSA_IO_SCREEN_ACQUIRE)!=20U)return 57;
    }
    for(i=0U;i<80U;++i){
        sprintf((char *)line,"LINE %03u: 0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ -- ordered output",i);
        if(TUILINE(line,(unsigned int)strlen((char *)line)))return 41;
    }
    if(P4LINK()!=0||P4WIDE()!=0)return 42;
    j=word(caps+52U);
    sprintf((char *)line,"CONSOLE INPUT 1: %u Q characters",j);
    if(TUILINE(line,(unsigned int)strlen((char *)line)))return 43;
    if(TUIREAD(input,j,&n)||n!=j)return 44;
    for(i=0U;i<n;++i)if(input[i]!=(unsigned char)'Q')return 45;
    if(TUILINE((const unsigned char *)"CONSOLE INPUT 2: empty Enter",sizeof "CONSOLE INPUT 2: empty Enter"-1U)||TUIREAD(input,256U,&n)||n)return 46;
    if(TUILINE((const unsigned char *)"CONSOLE INPUT 3: padded text",sizeof "CONSOLE INPUT 3: padded text"-1U)||TUIREAD(input,256U,&n)||
       n!=10U||memcmp(input,"  padded  ",10U))return 47;
    if(P4_TEST_MODE==1){
        if(TUILINE((const unsigned char *)"MONITOR INPUT: handoff",22U)||TUIHAND(1U)||
           TUIREAD(input,256U,&n)||n!=14U||memcmp(input,"MONITOR-ACCEPT",14U))return 48;
    }
    if(P4_TEST_MODE==2){
        unsigned char *raw=(unsigned char *)P4LOW();TTCCAP cap;unsigned char address[2];
        if(!raw||word(caps+8U)!=1U)return 49;
        if(TTCCONFIG(&cap,1U,9U,word(caps+16U)==43U?4U:word(caps+20U)==132U?5U:word(caps+16U)==32U?3U:2U,0U))return 50;
        raw[0]=0x27U;raw[1]=0x7eU;raw[2]=0xc3U;raw[3]=0x11U;
        TTCADDR(&cap,cap.rows*cap.columns-1U,address);raw[4]=address[0];raw[5]=address[1];
        raw[6]=0x1dU;raw[7]=0xf0U;
        memcpy(raw+8U,"NATIVE FULL SCREEN",18U);j=26U;
        raw[j++]=0x11U;TTCADDR(&cap,(cap.rows-4U)*cap.columns,raw+j);j+=2U;
        raw[j++]=0x1dU;raw[j++]=0U;raw[j++]=0x13U;
        if(P4PUT(j,raw)||P4GET(4096U,raw))return 51;
        /* One-character modified fields make a record larger than 4096 bytes
         * on model 5. Enter supplies attention; time never decides readiness. */
        raw[0]=0x27U;raw[1]=0x7eU;raw[2]=0xc3U;raw[3]=0x11U;
        TTCADDR(&cap,cap.rows*cap.columns-1U,raw+4U);
        raw[6]=0x1dU;raw[7]=0xf0U;
        memcpy(raw+8U,"NATIVE RAW RECORD",17U);j=25U;
        raw[j++]=0x11U;TTCADDR(&cap,cap.columns,raw+j);j+=2U;
        bytes=(cap.rows-6U)*cap.columns/2U;
        for(i=0U;i<bytes;++i){raw[j++]=0x1dU;raw[j++]=0x01U;raw[j++]=(unsigned char)'Q';}
        raw[j++]=0x1dU;raw[j++]=0xf0U;
        raw[j++]=0x11U;TTCADDR(&cap,cap.columns+1U,raw+j);j+=2U;raw[j++]=0x13U;
        if(P4PUT(j,raw)||P4READ(TSA_SCREEN_BYTES,raw,&n)||n!=3U+4U*bytes||raw[0]!=0x7dU)return 61;
        {
            unsigned int address;T27GEOMETRY geometry;
            if(T27GEOM(&geometry,cap.rows,cap.columns,T27_CODED12))return 62;
            for(i=0U;i<bytes;++i)
                if(raw[3U+4U*i]!=0x11U||raw[6U+4U*i]!=(unsigned char)'Q'||
                   T27DECD(&geometry,raw+4U+4U*i,&address)||address!=cap.columns+1U+2U*i)return 62;
        }
    }
    if(P4_TEST_MODE==3){
        if(TUILINE((const unsigned char *)"DISCONNECT MONITOR",18U)||TUIREAD(input,256U,&n))return 58;
        if(TUILINE((const unsigned char *)"DURING GAP",10U)||
           TUILINE((const unsigned char *)"RECONNECT MONITOR",17U)||TUIREAD(input,256U,&n))return 59;
        if(TUILINE((const unsigned char *)"AFTER GAP",9U))return 60;
    }
    if(TUIMON(status))return 52;
    sprintf((char *)line,"CONSOLE PASS: ROWS=%u COLS=%u GAPS=%u",word(caps+16U),word(caps+20U),word(status+16U));
    /* Raw screen mode remains a non-text lease until release. */
    if(P4_TEST_MODE==2&&TUICLOSE())return 53;
    bytes=(unsigned int)strlen((char *)line);
    if(P4_TEST_MODE==2){if(TUIOPEN((const unsigned char *)"Console qualification",21U))return 54;}
    if(P4_TEST_MODE==2){
        sprintf((char *)line,"RAW RECORD PASS: BYTES=%u",n);
        if(TUILINE(line,(unsigned int)strlen((char *)line)))return 63;
        sprintf((char *)line,"CONSOLE PASS: ROWS=%u COLS=%u GAPS=%u",word(caps+16U),word(caps+20U),word(status+16U));
        bytes=(unsigned int)strlen((char *)line);
    }
    if(TUILINE(line,bytes)||TUICLOSE())return 55;
    return 0;
}
