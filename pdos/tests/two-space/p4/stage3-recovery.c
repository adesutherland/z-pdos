/* SPDX-License-Identifier: MIT; no monitor reply is required for primary return. */
#include <string.h>
#include "twospace_ui.h"
static unsigned int word(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
static int line(const char *text)
{return (int)TUILINE((const unsigned char *)text,(unsigned int)strlen(text));}
static void put(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static void init(unsigned char *p){memset(p,0,64U);put(p,1U);put(p+4U,64U);}
int main(int argc,char **argv)
{
    unsigned char cap[64],input[256],request[64];unsigned int n,handle,generation;
    (void)argc;(void)argv;
    if(TUIOPEN((const unsigned char *)"INPUT RECOVERY",14U)||TUICAPS(cap))return 90;
    if(!(word(cap+48U)&TSA_FEATURE_INPUT_HANDOFF)){
        if(TUIHAND(1U)!=20U)return 91;
        init(request);put(request+20U,10U);if(TUIIO(64U,request,TSA_IO_DEVICE_OPEN))return 91;
        put(request+20U,0xf6U);if(TUIIO(64U,request,TSA_IO_DEVICE_READ)!=20U||TUIIO(64U,request,TSA_IO_DEVICE_CLOSE)||line("CAPTURE-ONLY MONITOR REFUSED"))return 91;
        return 0;
    }
    if(line("INTERACTIVE MONITOR CHECK")||TUIHAND(1U)||line("MONITOR ESCAPE READY")||TUIREAD(input,sizeof input,&n)||n!=13U||memcmp(input,"PRIMARY-REPLY",13U))return 92;
    if(TUIHAND(1U)!=16U||line("PRIMARY RETURN / MONITOR QUARANTINE PASS")||
       TUIREAD(input,sizeof input,&n)||n!=4U||memcmp(input,"NEXT",4U))return 93;
    if(line("RECONNECT MONITOR")||TUIREAD(input,sizeof input,&n)||n!=8U||memcmp(input,"CONTINUE",8U))return 94;
    if(TUIHAND(1U)||line("SECOND MONITOR READY")||TUIREAD(input,sizeof input,&n)||n!=11U||memcmp(input,"MONITOR-NEW",11U))return 95;
    if(TUIHAND(0U))return 96;
    init(request);put(request+20U,10U);if(TUIIO(64U,request,TSA_IO_DEVICE_OPEN))return 96;
    handle=word(request+8U);generation=word(request+12U);
    if(line("RAW MONITOR ESCAPE READY"))return 96;
    put(request+8U,handle);put(request+12U,generation);put(request+20U,0xf6U);put(request+28U,sizeof input);put(request+36U,(unsigned int)(unsigned long)input);
    if(TUIIO(64U,request,TSA_IO_DEVICE_READ)!=16U||TUIIO(64U,request,TSA_IO_DEVICE_CLOSE)||line("RAW CANCEL READY")||
       TUIREAD(input,sizeof input,&n)||n!=9U||memcmp(input,"RAW-REPLY",9U)||line("RECOVERY CLIENT PASS"))return 96;
    return 0;
}
