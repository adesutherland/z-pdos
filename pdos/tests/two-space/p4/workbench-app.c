/* SPDX-License-Identifier: MIT; a C panel/session client independent of PCOMM. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "twospace_ui.h"
#include "twospace_3270.h"
static void put(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static unsigned int word(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
static void pointer(unsigned char *p,const void *v)
{put(p,0U);put(p+4U,(unsigned int)(unsigned long)v);}
static void init(unsigned char *p,unsigned int bytes)
{memset(p,0,bytes);put(p,1U);put(p+4U,bytes);}
int main(int argc,char **argv)
{
    unsigned char caps[64],panel[64],session[64],devicecaps[128],line[256],input[256],*record;
    unsigned int generation,n,first,second,handle,seqhi,seqlo,i,bytes;T27GEOMETRY g;T27INPUTEVENT event;
    (void)argc;(void)argv;
    if(TUIOPEN((const unsigned char *)"z/PDOS Workbench / C panel client",sizeof "z/PDOS Workbench / C panel client"-1U)||TUICAPS(caps))return 70;
    generation=word(caps+60U);
    if(word(caps+8U)!=TSA_DEVICE_3270)return 71;
    init(session,64U);
    if(TUIIO(64U,session,TSA_IO_DEVICE_OPEN)||!(handle=word(session+8U)))return 72;
    put(session+12U,generation);put(session+28U,128U);pointer(session+32U,devicecaps);
    i=TUIIO(64U,session,TSA_IO_DEVICE_CAPS);
    sprintf((char *)line,"WB CAPS RC=%u RECORD=%u TRANSFER=%u QUAL=%u",i,word(devicecaps+72U),word(devicecaps+76U),word(devicecaps+68U));
    if(TUILINE(line,(unsigned int)strlen((char *)line)))return 100;
    if(i||word(devicecaps+72U)!=65535U||word(devicecaps+76U)!=131072U||word(devicecaps+68U))return 73;
    record=(unsigned char *)malloc(65535U);if(!record)return 74;
    put(session+28U,65535U);pointer(session+32U,record);
    if(TUIIO(64U,session,TSA_IO_DEVICE_EVENT))return 75;
    seqhi=word(session+40U);seqlo=word(session+44U);bytes=word(session+48U);
    if(!bytes||record[0U]!=0x88U)return 76;
    put(session+44U,seqlo+1U);if(TUIIO(64U,session,TSA_IO_DEVICE_ACK)!=8U)return 77;
    put(session+44U,seqlo);if(TUIIO(64U,session,TSA_IO_DEVICE_ACK))return 78;
    init(session,64U);put(session+8U,handle);put(session+12U,generation);put(session+20U,0xf2U);
    if(word(devicecaps+48U)&TSA_FEATURE_READ_BUFFER){
    if(TUIIO(64U,session,TSA_IO_DEVICE_READ))return 79;
    bytes=word(session+48U);seqhi=word(session+40U);seqlo=word(session+44U);
    if(bytes<=word(caps+16U)*word(caps+20U)||word(session+52U)!=6U)return 80;
    put(session+16U,0U);put(session+28U,65535U);pointer(session+32U,0);
    if(TUIIO(64U,session,TSA_IO_DEVICE_EVENT)!=8U||word(session+40U)!=seqhi||word(session+44U)!=seqlo)return 81;
    pointer(session+32U,record);if(TUIIO(64U,session,TSA_IO_DEVICE_EVENT)||word(session+48U)!=bytes)return 82;
    if(T27GEOM(&g,word(caps+16U),word(caps+20U),word(caps+40U))||T27BUFFER(&g,record,bytes,(word(caps+16U)-1U-(word(caps+20U)<120U?2U:1U)-(260U+word(caps+20U)-1U)/word(caps+20U))*word(caps+20U)+4U,input,256U,&n,&event)||n)return 83;
    put(session+16U,0U);if(TUIIO(64U,session,TSA_IO_DEVICE_ACK))return 84;
    }else if(TUIIO(64U,session,TSA_IO_DEVICE_READ)!=20U)return 79;
    if(TUIIO(64U,session,TSA_IO_DEVICE_CLOSE))return 84;
    if(TUIIO(64U,session,TSA_IO_DEVICE_CLOSE)!=8U)return 85;
    free(record);
#if P4_TEST_MODE == 6
    record=(unsigned char *)malloc(131072U);if(!record)return 101;memset(record,0x40U,131072U);
    init(session,64U);if(TUIIO(64U,session,TSA_IO_DEVICE_OPEN))return 102;handle=word(session+8U);
    put(session+20U,0xf1U);put(session+28U,32767U);pointer(session+32U,record);
    if(TUIIO(64U,session,TSA_IO_DEVICE_WRITE))return 103;
    put(session+24U,32766U);put(session+28U,1U);pointer(session+32U,record+32766U);
    if(TUIIO(64U,session,TSA_IO_DEVICE_WRITE)!=8U)return 104;
    put(session+16U,1U);put(session+24U,32767U);put(session+28U,98305U);pointer(session+32U,record+32767U);
    if(TUIIO(64U,session,TSA_IO_DEVICE_WRITE)!=12U)return 105;
    put(session+16U,0U);if(TUIIO(64U,session,TSA_IO_DEVICE_CLOSE))return 106;
    init(session,64U);if(TUIIO(64U,session,TSA_IO_DEVICE_OPEN))return 107;
    put(session+20U,0xf1U);put(session+28U,8191U);pointer(session+32U,record);if(TUIIO(64U,session,TSA_IO_DEVICE_WRITE))return 108;
    put(session+16U,1U);put(session+24U,8191U);put(session+28U,8193U);pointer(session+32U,record+8191U);
    if(TUIIO(64U,session,TSA_IO_DEVICE_WRITE)||word(session+48U)!=16384U)return 109;
    put(session+16U,0U);if(TUIIO(64U,session,TSA_IO_DEVICE_CLOSE))return 110;free(record);
    if(TUILINE((const unsigned char *)"WBTRANSFER PASS 16384 / 131072 rejected",sizeof "WBTRANSFER PASS 16384 / 131072 rejected"-1U))return 111;
#endif
    init(panel,64U);put(panel+12U,generation-1U);
    if(TUIPANEL(TSA_IO_PANEL_CREATE,panel)!=16U||word(panel+8U))return 86;
    put(panel+12U,generation);put(panel+20U,4U);put(panel+24U,3U);put(panel+28U,3U);put(panel+32U,25U);put(panel+36U,2U);
    put(panel+40U,9U);pointer(panel+44U,"Panel one");
    if(TUIPANEL(TSA_IO_PANEL_CREATE,panel)||!(first=word(panel+8U)))return 87;
    put(panel+52U,12U);pointer(panel+56U,"Owned C text");if(TUIPANEL(TSA_IO_PANEL_TEXT,panel))return 88;
    put(panel+8U,0U);put(panel+24U,31U);put(panel+40U,9U);pointer(panel+44U,"Panel two");
    if(TUIPANEL(TSA_IO_PANEL_CREATE,panel)||!(second=word(panel+8U)))return 89;
    put(panel+52U,14U);pointer(panel+56U,"Separate panel");if(TUIPANEL(TSA_IO_PANEL_TEXT,panel))return 90;
    put(panel+8U,0U);if(TUIPANEL(TSA_IO_PANEL_COMMIT,panel))return 91;
    if(TUILINE((const unsigned char *)"WBPANEL READY",13U)||TUIREAD(input,256U,&n)||n)return 92;
    put(panel+8U,second);if(TUIPANEL(TSA_IO_PANEL_HIDE,panel))return 93;
    put(panel+8U,first);if(TUIPANEL(TSA_IO_PANEL_CLOSE,panel))return 94;
    put(panel+8U,0U);if(TUIPANEL(TSA_IO_PANEL_COMMIT,panel))return 95;
    for(i=0U;i<200U;++i){sprintf((char *)line,"WB LINE %03u / retained output for scrollback",i);if(TUILINE(line,(unsigned int)strlen((char *)line)))return 96;}
    if(TUILINE((const unsigned char *)"WBHISTORY READY",15U)||TUIREAD(input,256U,&n)||n!=10U||memcmp(input,"draft kept",10U))return 97;
    if(TUILINE((const unsigned char *)"WBINPUT SOURCE",14U))return 98;
    init(session,32U);put(session+8U,1U);
    if(TUIIO(32U,session,TSA_IO_UI_INPUT)||TUIREAD(input,256U,&n)||n!=12U||memcmp(input,"MONITOR-LINE",12U))return 98;
    if(TUILINE((const unsigned char *)"WORKBENCH CLIENT PASS",21U)||TUICLOSE())return 99;
    return 0;
}
