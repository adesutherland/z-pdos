/* SPDX-License-Identifier: MIT
 * U computes model layout and logical field requests. K performs encoding,
 * device I/O, native text fanout and ownership. No U console below 16 MiB. */
#include "twospace_ui.h"
#include "twospace_console.h"
static unsigned char ui_cap[64],ui_title[132];
static unsigned int ui_title_length,ui_open;
static void put(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static unsigned int word(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
static void zero(unsigned char *p,unsigned int n)
{unsigned int i;for(i=0U;i<n;++i)p[i]=0U;}
static void pointer(unsigned char *p,const void *address)
{put(p,0U);put(p+4U,(unsigned int)(unsigned long)address);}
unsigned int TUICAPS(unsigned char *cap)
{zero(cap,64U);put(cap,1U);put(cap+4U,64U);return TUIIO(64U,cap,TSA_IO_CAPABILITIES);}
unsigned int TUIOPEN(const unsigned char *title,unsigned int length)
{
    unsigned char request[128];unsigned int rc,i,rows,cols;
    static const unsigned char footer[]={0xc5,0x95,0xa3,0x85,0x99,0x40,0xa3,0x96,0x40,0xa2,0xa4,0x82,0x94,0x89,0xa3};
    if(sizeof(void *)!=4U||!title||length>132U)return 20U;
    rc=TUICAPS(ui_cap);if(rc)return rc;
    for(i=0U;i<length;++i)ui_title[i]=title[i];ui_title_length=length;
    if(word(ui_cap+8U)==TSA_DEVICE_LINE){ui_open=1U;return 0U;}
    rows=word(ui_cap+16U);cols=word(ui_cap+20U);
    if(rows<8U||cols<20U)return 20U;
    zero(request,32U);put(request,1U);put(request+4U,32U);
    put(request+24U,word(ui_cap+60U));
    rc=TUIIO(32U,request,TSA_IO_SCREEN_ACQUIRE);if(rc)return rc;
    zero(request,sizeof request);put(request,1U);put(request+4U,sizeof request);
    put(request+12U,4U);put(request+16U,rows-4U);put(request+20U,1U);
    put(request+24U,word(ui_cap+60U));
    /* Header, output origin, footer, editable entry. The K line model
     * retains the one ordered output stream between U layout requests. */
    put(request+32U+8U,TTC_BRIGHT);put(request+32U+12U,length<cols?length:cols-1U);
    pointer(request+32U+16U,ui_title);
    put(request+56U,1U);put(request+56U+8U,TTC_PROTECTED);
    put(request+80U,rows-5U);put(request+80U+8U,TTC_PROTECTED);
    put(request+80U+12U,sizeof footer);pointer(request+80U+16U,footer);
    put(request+104U,rows-4U);put(request+104U+4U,1U);put(request+104U+8U,TTC_INPUT);
    rc=TUIIO(sizeof request,request,TSA_IO_SCREEN_WRITE);if(rc)return rc;
    ui_open=1U;return 0U;
}
static unsigned int line(unsigned char *text,unsigned int capacity,unsigned int *length,unsigned int op)
{
    unsigned char request[32];unsigned int rc;
    if(!ui_open||!text||capacity>256U)return 8U;
    zero(request,32U);put(request,1U);put(request+4U,32U);put(request+12U,capacity);
    pointer(request+16U,text);rc=TUIIO(32U,request,op);
    if(!rc&&length)*length=word(request+24U);return rc;
}
unsigned int TUILINE(const unsigned char *text,unsigned int length)
{return line((unsigned char *)text,length,0,TSA_IO_LINE_WRITE);}
unsigned int TUIREAD(unsigned char *text,unsigned int capacity,unsigned int *length)
{return line(text,capacity,length,TSA_IO_LINE_READ);}
unsigned int TUIMON(unsigned char *status)
{zero(status,32U);put(status,1U);put(status+4U,32U);return TUIIO(32U,status,TSA_IO_MONITOR_STATUS);}
unsigned int TUIHAND(unsigned int source)
{
    unsigned char request[32];zero(request,32U);put(request,1U);put(request+4U,32U);
    put(request+8U,source);return TUIIO(32U,request,TSA_IO_INPUT_HANDOFF);
}
unsigned int TUICLOSE(void)
{
    unsigned char request[32];unsigned int rc;
    if(!ui_open)return 8U;
    if(word(ui_cap+8U)==TSA_DEVICE_LINE){ui_open=0U;return 0U;}
    zero(request,32U);put(request,1U);put(request+4U,32U);
    put(request+24U,word(ui_cap+60U));rc=TUIIO(32U,request,TSA_IO_SCREEN_RELEASE);
    if(!rc)ui_open=0U;return rc;
}

unsigned int TUIRESULT(unsigned char *result)
{zero(result,32U);put(result,1U);put(result+4U,32U);return TUIIO(32U,result,TSA_IO_RESULT);}
