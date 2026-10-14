/* SPDX-License-Identifier: MIT; actual provider preflight/failure publication.
 * Hardware callbacks below are deliberate fakes, not guest proof. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "twospace_gate.h"
#include "twospace_fixture.h"
#include "twospace_batch.h"
#include "twospace_io.h"
#include "twospace_invocation.h"
#include "twospace_console.h"
static unsigned char *host_real,user[1024];
static unsigned int host_r0_hi,host_r2_hi,host_ipl=441U;
static unsigned int io_calls,read_calls,writable=1U,fault,clear_fail;
static TSVSTACK native_invocations;static TIOPOOL channel_io;static TTCCAP tc_cap;
#undef TSF_KAPERTURE_VA
#define TSF_KAPERTURE_VA ((unsigned long)host_real)
static unsigned int cms_word(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
static void cms_put_word(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static unsigned int tc_span(TSPADDR a,unsigned int n,unsigned int direction,unsigned char *bytes,unsigned int copy)
{if(a.hi||a.lo>sizeof user||n>sizeof user-a.lo||(direction==TSG_WRITE&&!writable))return 8U;
 if(copy){if(direction==TSG_WRITE)memcpy(user+a.lo,bytes,n);else memcpy(bytes,user+a.lo,n);}return 0U;}
static unsigned int tc_find(unsigned int address,TSCSTATE *c)
{(void)c;return address==12U||address==14U?0x10000U+address:0U;}
static int channel_run(TSCSTATE *c,unsigned int ssid)
{
    unsigned char *r=TSCIRB(c);unsigned int command=c->aperture[c->region_real+TSC_CCW_OFFSET],i;
    ++io_calls;memset(r,0,64U);cms_put_word(r+4U,c->region_real+TSC_CCW_OFFSET+8U);r[8U]=12U;
    if(command==0xe4U){unsigned char id[]={0xff,0,0,0,0x35,5,0};if(ssid==0x1000eU){id[4]=0x14U;id[5]=3U;}memcpy(TSCDATA(c),id,7U);}
    else if(command==4U){r[11U]=7U;TSCDATA(c)[0]=0x40U;}
    else if(command==2U){++read_calls;for(i=0U;i<80U;++i)TSCDATA(c)[i]=(unsigned char)i;
        if(fault==1U)r[11U]=1U;
        if(fault==2U){r[8U]=13U;r[11U]=80U;}
        if(fault==3U){r[8U]=14U;r[11U]=80U;return -1;}}
    return 0;
}
static int channel_quiesce(TSCSTATE *c,unsigned int ssid,unsigned int owner)
{TIOREQUEST *r=(TIOREQUEST *)TIOSTATE(&channel_io,c);(void)ssid;(void)owner;if(clear_fail)return -1;r->phase=TIO_IDLE;c->occupied=c->quarantined=0U;return 0;}
#include "batch-native.inc"
static unsigned char *request(TSGREQUEST *q,unsigned int op,unsigned int handle)
{unsigned char *h=user+64U;memset(h,0,64U);cms_put_word(h,1U);cms_put_word(h+4U,64U);cms_put_word(h+8U,handle);
 q->address.hi=0U;q->address.lo=64U;q->length=64U;q->direction=op;q->svc=200U;return h;}
static void read_request(unsigned char *h)
{cms_put_word(h+24U,80U);cms_put_word(h+36U,256U);}
int main(void)
{
    TSGREQUEST q;unsigned char *h,saved[64];unsigned int handle,calls;TIOREQUEST *operation;
    host_real=calloc(1,0x100000U);assert(host_real);TIOINIT(&channel_io);
    native_invocations.depth=1U;native_invocations.frame[0].token=7U;native_invocations.frame[0].state=TSV_ACTIVE;
    h=request(&q,TSA_IO_BATCH_OPEN,0U);cms_put_word(h+12U,1U);cms_put_word(h+16U,12U);memcpy(saved,h,64U);
    writable=0U;assert(batch_device_service(&q)==8U&&!io_calls&&!memcmp(saved,h,64U));writable=1U;
    assert(!batch_device_service(&q));handle=cms_word(h+8U);assert(handle&&TSVHAS(&native_invocations,7U,TSV_BATCH_DEVICE,handle)==TSV_OK);
    h=request(&q,TSA_IO_BATCH_READ,handle);read_request(h);cms_put_word(h+36U,64U);calls=io_calls;
    assert(batch_device_service(&q)==8U&&io_calls==calls&&!read_calls);
    cms_put_word(h+36U,1000U);assert(batch_device_service(&q)==8U&&io_calls==calls&&!read_calls);
    cms_put_word(h+36U,256U);cms_put_word(h+24U,79U);assert(batch_device_service(&q)==8U&&io_calls==calls);
    cms_put_word(h+24U,80U);assert(!batch_device_service(&q)&&cms_word(h+28U)==80U&&read_calls==1U&&user[256U]==0U&&user[335U]==79U);
    h=request(&q,TSA_IO_BATCH_READ,handle);read_request(h);memset(user+256U,0xa5,80U);fault=1U;
    assert(batch_device_service(&q)==12U&&!cms_word(h+28U)&&!cms_word(h+60U)&&user[256U]==0xa5U&&user[335U]==0xa5U);
    fault=2U;assert(!batch_device_service(&q)&&cms_word(h+60U)==1U&&!cms_word(h+28U)&&user[256U]==0xa5U);
    fault=3U;assert(batch_device_service(&q)==12U&&cms_word(h+40U)==14U&&cms_word(h+48U)==1U&&h[52U]==0x40U&&!cms_word(h+60U)&&user[256U]==0xa5U);fault=0U;
    native_invocations.frame[0].token=8U;calls=io_calls;assert(batch_device_service(&q)==8U&&io_calls==calls);native_invocations.frame[0].token=7U;
    operation=(TIOREQUEST *)TIOSTATE(&channel_io,&batch_channels[0]);operation->phase=TIO_QUARANTINED;batch_channels[0].occupied=batch_channels[0].quarantined=1U;clear_fail=1U;
    h=request(&q,TSA_IO_BATCH_CLOSE,handle);assert(batch_device_service(&q)==12U&&TBCFIND(&batch_endpoints,7U,handle));
    assert(TSVHAS(&native_invocations,7U,TSV_BATCH_DEVICE,handle)==TSV_OK);clear_fail=0U;
    assert(!batch_device_service(&q)&&!TBCFIND(&batch_endpoints,7U,handle)&&TSVHAS(&native_invocations,7U,TSV_BATCH_DEVICE,handle)==TSV_STALE);
    assert(batch_device_service(&q)==8U);
    free(host_real);puts("PD033 actual K provider: preflight/no-I/O, full/partial/EOF, saved Sense, owner and failed-clear controls PASS");return 0;
}
