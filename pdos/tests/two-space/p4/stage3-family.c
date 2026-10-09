/* SPDX-License-Identifier: MIT; provider-executed reply modes and3287 text. */
#include <stdio.h>
#include <string.h>
#include "twospace_ui.h"
#include "twospace_3270.h"
#ifndef FAM_DBCS
#define FAM_DBCS 0
#endif
static unsigned int word(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
static void put(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static void init(unsigned char *p){memset(p,0,64U);put(p,1U);put(p+4U,64U);}
static void pointer(unsigned char *p,const void *v){put(p,0U);put(p+4U,(unsigned int)(unsigned long)v);}
static unsigned int send(unsigned int handle,unsigned int generation,unsigned int command,const unsigned char *bytes,unsigned int n)
{unsigned char r[64];init(r);put(r+8U,handle);put(r+12U,generation);put(r+16U,1U);put(r+20U,command);put(r+28U,n);pointer(r+32U,bytes);return TUIIO(64U,r,TSA_IO_DEVICE_WRITE);}
static unsigned int ack(unsigned char *r)
{put(r+16U,0U);return TUIIO(64U,r,TSA_IO_DEVICE_ACK);}
int main(int argc,char **argv)
{
    unsigned char r[64],saved[64],cap[128],input[256],record[8192],stream[256],attrs[]={0x41U,0x42U};
    unsigned int h,g,n,i,mode,printer;
    static const unsigned char style[]={0xc2U,0x11U,0x40U,0x40U,0x29U,3U,0xc0U,0xf8U,0x42U,0xf2U,0x41U,0xf4U,
        0x28U,0x42U,0xf1U,0xc5U,0xe7U,0xe3U,0xc5U,0xd5U,0xc4U,0xc5U,0xc4U};
    (void)argc;(void)argv;
    if(TUIOPEN((const unsigned char *)"3270 family client",18U))return 70;
    init(r);put(r+12U,1500U);put(r+56U,1U);memcpy(saved,r,64U);
    if(TUIIO(64U,r,TSA_IO_DASD_STATUS)!=8U||memcmp(saved,r,64U))return 70;
    init(r);if(TUIIO(64U,r,TSA_IO_DEVICE_OPEN))return 71;h=word(r+8U);g=word(r+12U);
    put(r+28U,sizeof cap);pointer(r+32U,cap);if(TUIIO(64U,r,TSA_IO_DEVICE_CAPS)||!(cap[96U+0x88U/8U]&(1U<<(0x88U%8U))))return 72;
    put(r+28U,sizeof record);pointer(r+32U,record);if(TUIIO(64U,r,TSA_IO_DEVICE_EVENT)||ack(r))return 73;
    if(send(h,g,0xf1U,style,sizeof style))return 74;
    for(mode=0U;mode<3U;++mode){
        if(T27REPLY(mode,mode==2U?attrs:0,mode==2U?2U:0U,stream,sizeof stream,&n)||send(h,g,0xf3U,stream,n))return 75;
        init(r);put(r+8U,h);put(r+12U,g);put(r+20U,0xf2U);put(r+28U,sizeof record);pointer(r+32U,record);
        if(TUIIO(64U,r,TSA_IO_DEVICE_READ)||word(r+52U)!=6U)return 76;n=word(r+48U);
        if(n<16U||record[3U]!=(mode?0x29U:0x1dU))return 77;
        if(mode==2U){for(i=3U;i+2U<n;++i)if(record[i]==0x28U&&record[i+1U]==0x42U&&record[i+2U]==0xf1U)break;if(i+2U==n)return 78;}
        if(ack(r))return 79;
    }
    if(FAM_DBCS){
        /* ICU78.3 independently encodes the two Unicode characters U+65E5,
         * U+672C as IBM930 SO45624566SI. Verify bytes and rendered characters. */
        static const unsigned char japanese[]={0x0eU,0x45U,0x62U,0x45U,0x66U,0x0fU};
        T27GEOMETRY geometry;unsigned int at=0U;
        if(!(cap[96U+0x91U/8U]&(1U<<(0x91U%8U)))||T27GEOM(&geometry,word(cap+16U),word(cap+20U),word(cap+40U)))return 88;
        stream[at++]=0xc2U;stream[at++]=0x11U;if(T27ADDR(&geometry,5U*geometry.columns,stream+at))return 88;at+=2U;
        stream[at++]=0x1dU;stream[at++]=0xf0U;memcpy(stream+at,japanese,sizeof japanese);at+=sizeof japanese;
        stream[at++]=0x1dU;stream[at++]=0xf0U;
        if(send(h,g,0xf1U,stream,at))return 88;
        init(r);put(r+8U,h);put(r+12U,g);put(r+20U,0xf6U);put(r+28U,sizeof record);pointer(r+32U,record);
        if(TUIIO(64U,r,TSA_IO_DEVICE_READ)||ack(r))return 88;
        init(r);put(r+8U,h);put(r+12U,g);put(r+20U,0xf2U);put(r+28U,sizeof record);pointer(r+32U,record);
        if(TUIIO(64U,r,TSA_IO_DEVICE_READ))return 89;n=word(r+48U);
        for(i=3U;i+sizeof japanese<=n;++i)if(!memcmp(record+i,japanese,sizeof japanese))break;
        if(i+sizeof japanese>n||ack(r))return 89;
    }
    if(TUIIO(64U,r,TSA_IO_DEVICE_CLOSE)||TUILINE((const unsigned char *)"EXTENDED REPLY PASS / native field mode restored",sizeof "EXTENDED REPLY PASS / native field mode restored"-1U))return 80;
    init(r);put(r+20U,11U);if(TUIIO(64U,r,TSA_IO_DEVICE_OPEN))return 81;printer=word(r+8U);g=word(r+12U);
    put(r+28U,sizeof cap);pointer(r+32U,cap);if(TUIIO(64U,r,TSA_IO_DEVICE_CAPS)||word(cap+8U)!=TSA_DEVICE_PRINTER||word(cap+72U))return 82;
    init(r);put(r+8U,printer);put(r+12U,g);put(r+16U,1U);put(r+20U,0xf1U);put(r+28U,6U);put(r+32U,1U);
    if(TUIIO(64U,r,TSA_IO_DEVICE_WRITE)!=8U)return 82;
    stream[0U]=0xc8U;stream[1U]=0x11U;
    if(send(printer,g,0xf1U,stream,2U)!=20U)return 82;
    init(r);put(r+8U,printer);if(TUIIO(64U,r,TSA_IO_DEVICE_CLOSE))return 82;
    init(r);put(r+20U,11U);if(TUIIO(64U,r,TSA_IO_DEVICE_OPEN)||word(r+8U)==printer)return 82;
    printer=word(r+8U);g=word(r+12U);
    if(T27PRINT((const unsigned char *)"z/PDOS3270 printer\025Exact checked output\025",sizeof "z/PDOS3270 printer\025Exact checked output\025"-1U,stream,sizeof stream,&n))return 83;
    if(send(printer,g,0xf1U,stream,n))return 84;
    init(r);put(r+8U,printer);put(r+12U,g);put(r+20U,0xf2U);
    if(TUIIO(64U,r,TSA_IO_DEVICE_READ)!=20U||TUIIO(64U,r,TSA_IO_DEVICE_CLOSE))return 85;
    if(TUILINE((const unsigned char *)"PRINTER PASS / explicit End Media",sizeof "PRINTER PASS / explicit End Media"-1U)||
       TUILINE((const unsigned char *)"NATIVE INPUT AFTER RAW",sizeof "NATIVE INPUT AFTER RAW"-1U)||TUIREAD(input,sizeof input,&n)||n!=3U||memcmp(input,"ABC",3U))return 86;
    if(TUILINE((const unsigned char *)"FAMILY CLIENT PASS",18U))return 87;return 0;
}
