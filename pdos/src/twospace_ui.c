/* SPDX-License-Identifier: MIT
 * U computes model layout and logical field requests. K performs encoding,
 * device I/O, native text fanout and ownership. No U console below 16 MiB. */
#include "twospace_ui.h"
#include "twospace_console.h"
static unsigned char ui_cap[64];
static unsigned int ui_open;
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
    unsigned char request[32];unsigned int rc;
    if(sizeof(void *)!=4U||!title||!length||length>132U)return 20U;
    rc=TUICAPS(ui_cap);if(rc)return rc;
    zero(request,32U);put(request,1U);put(request+4U,32U);put(request+12U,length);pointer(request+16U,title);
    rc=TUIIO(32U,request,TSA_IO_WORKBENCH);if(!rc)ui_open=1U;return rc;
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
    put(request+8U,source);return TUIIO(32U,request,TSA_IO_UI_INPUT);
}
unsigned int TUICLOSE(void)
{unsigned char r[32];unsigned int rc;if(!ui_open)return 8U;zero(r,32U);put(r,1U);put(r+4U,32U);put(r+8U,1U);rc=TUIIO(32U,r,TSA_IO_WORKBENCH);if(!rc)ui_open=0U;return rc;}
unsigned int TUIKEY(unsigned char *text,unsigned int capacity,unsigned int *length,unsigned int *aid)
{
    unsigned char r[32];unsigned int rc;
    if(!ui_open||!text||!capacity||capacity>256U||!length||!aid)return 8U;
    zero(r,32U);put(r,1U);put(r+4U,32U);put(r+12U,capacity);pointer(r+16U,text);
    rc=TUIIO(32U,r,TSA_IO_LINE_KEY);if(!rc){*length=word(r+24U);*aid=word(r+28U);}return rc;
}
static unsigned int control(unsigned int op,unsigned int flags,const unsigned char *text,unsigned int bytes,unsigned int extra)
{
    unsigned char r[32];zero(r,32U);put(r,1U);put(r+4U,32U);put(r+8U,flags);put(r+12U,bytes);
    if(text)pointer(r+16U,text);put(r+24U,extra);return TUIIO(32U,r,op);
}
unsigned int TUISET(const unsigned char *text,unsigned int bytes,unsigned int cursor)
{
    unsigned char r[32],cap[64];unsigned int rc;
    if(bytes>256U||cursor>bytes)return 8U;rc=TUICAPS(cap);if(rc)return rc;
    zero(r,32U);put(r,1U);put(r+4U,32U);put(r+8U,cursor);put(r+12U,bytes);pointer(r+16U,text);put(r+28U,word(cap+60U));
    return TUIIO(32U,r,TSA_IO_LINE_SET);
}
unsigned int TUIJOB(const unsigned char *text,unsigned int bytes)
{return control(TSA_IO_JOB_BEGIN,0U,text,bytes,0U);}
unsigned int TUICTRL(unsigned int enabled)
{return control(TSA_IO_UI_CONTROL,enabled,0,0U,0U);}
unsigned int TUIJOIN(unsigned int pending,unsigned int bytes)
{return control(TSA_IO_UI_STATUS,2U,0,bytes,pending);}
unsigned int TUISHELL(void)
{return control(TSA_IO_JOB_BEGIN,3U,0,0U,0U);}
unsigned int TUIEXT(void)
{return control(TSA_IO_JOB_BEGIN,2U,0,0U,0U);}
unsigned int TUIEND(unsigned int os,unsigned int valid,unsigned int rc)
{return control(TSA_IO_JOB_END,valid,0,rc,os);}
unsigned int TUIVOL(const unsigned char *text,unsigned int bytes)
{return control(TSA_IO_UI_STATUS,1U,text,bytes,0U);}
unsigned int TUIPANEL(unsigned int operation,unsigned char *request)
{return TUIIO(64U,request,operation);}

unsigned int TUIRESULT(unsigned char *result)
{zero(result,32U);put(result,1U);put(result+4U,32U);return TUIIO(32U,result,TSA_IO_RESULT);}
