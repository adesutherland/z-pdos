/* SPDX-License-Identifier: MIT; owned panel/key-event C90 example. */
#include <stdio.h>
#include <string.h>
#include "twospace_ui.h"
static void put(unsigned char *p,unsigned int v)
{p[0]=(unsigned char)(v>>24);p[1]=(unsigned char)(v>>16);
 p[2]=(unsigned char)(v>>8);p[3]=(unsigned char)v;}
static unsigned int word(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)
        |((unsigned int)p[2]<<8)|p[3];}
int main(int argc,char **argv)
{
    unsigned char cap[64],request[64],input[256],cells[50];
    unsigned int handle=0U,n,aid,rc;char message[100];
    const char *title="C / owned panel example";
    (void)argc;(void)argv;
    if(TUIOPEN((const unsigned char *)title,strlen(title))
       ||TUICAPS(cap))return 8;
    /* Mirror the owned panel's rows into semantic output as well. Keep
     * the instruction below the panel so it remains visible on screen
     * and is equally useful to the plain line client. */
    TUILINE((const unsigned char *)title,strlen(title));
    TUILINE((const unsigned char *)"C owns the presentation.",sizeof "C owns the presentation."-1U);
    TUILINE((const unsigned char *)"K owns events and I/O.",sizeof "K owns events and I/O."-1U);
    TUILINE((const unsigned char *)"Enter text / Q or PF3 returns",sizeof "Enter text / Q or PF3 returns"-1U);
    if(word(cap+8)==TSA_DEVICE_3270){
        memset(request,0,sizeof request);put(request,1U);
        put(request+4,64U);put(request+12,word(cap+60));
        put(request+20,3U);put(request+24,1U);
        put(request+28,3U);put(request+32,25U);
        put(request+40,strlen(title));
        put(request+48,(unsigned int)(unsigned long)title);
        if(TUIPANEL(TSA_IO_PANEL_CREATE,request))return 8;
        handle=word(request+8);memset(cells,' ',sizeof cells);
        memcpy(cells,"C owns the presentation.",24U);
        memcpy(cells+25,"K owns events and I/O.",sizeof "K owns events and I/O."-1U);
        put(request+52,sizeof cells);
        put(request+60,(unsigned int)(unsigned long)cells);
        if(TUIPANEL(TSA_IO_PANEL_TEXT,request))return 8;
        put(request+8,0U);
        if(TUIPANEL(TSA_IO_PANEL_COMMIT,request))return 8;
    }
    for(;;){rc=TUIKEY(input,sizeof input,&n,&aid);if(rc)return (int)rc;
        sprintf(message,"PANEL: AID=%02X, submitted=%u bytes",aid,n);
        TUILINE((const unsigned char *)message,strlen(message));
        if(aid==0xf3U||(n==1U&&(input[0]=='Q'||input[0]=='q')))break;
    }
    if(handle){put(request+8,handle);
        if(TUIPANEL(TSA_IO_PANEL_CLOSE,request))return 8;}
    return 0;
}
