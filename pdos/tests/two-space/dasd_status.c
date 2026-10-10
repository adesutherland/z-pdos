/* SPDX-License-Identifier: MIT; execute the actual K status service with
 * deterministic channel, owned-allocation and U-span adapters. */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "twospace_dasd.h"
#include "twospace_gate.h"
#include "twospace_session.h"
static unsigned char u[TDA_MAX_TRACKS+8192U],f4[140],f1[140];
static unsigned int reads,copies,allocations,releases,bad_at,read_failure,alloc_failure,active=1U;
static unsigned int pointer_hi,pointer_lo;static void *owned;
static struct {int volume_count;struct {int device,address;char volser[7];} volumes[1];} media_state;
static unsigned int cms_word(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
static void cms_put_word(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static unsigned int tc_span(TSPADDR p,unsigned int n,unsigned int direction,unsigned char *data,unsigned int copy)
{
    unsigned int offset;
    assert(n<=TDS_TRANSFER_BYTES);
    if(p.hi==0U&&p.lo==1024U&&n==64U)offset=1024U;
    else{
        unsigned int delta=p.lo-pointer_lo;
        if(p.hi!=pointer_hi+(p.lo<pointer_lo)||delta>TDA_MAX_TRACKS||n>TDA_MAX_TRACKS-delta)return 8U;
        offset=4096U+delta;
        if(bad_at&&offset<bad_at&&n>bad_at-offset)return 8U;
        if(bad_at&&offset>=bad_at)return 8U;
    }
    if(copy){assert(data);if(direction==TSG_READ)memcpy(data,u+offset,n);
        else {memcpy(u+offset,data,n);++copies;}}
    return 0U;
}
static int media_status_active(void){return (int)active;}
static int media_init(void){return 0;}
static void *media_malloc(unsigned int n)
{assert(!owned);if(alloc_failure)return 0;owned=malloc(n);assert(owned);++allocations;return owned;}
static void media_free(void *p){assert(p&&p==owned);free(p);owned=0;++releases;}
static int media_rdblock(int device,int c,int h,int r,void *data,int capacity,int command)
{
    assert(device==7&&capacity==140&&command==0x0e);++reads;
    if(read_failure==reads)return -1;
    if(c==1&&h==0&&r==1)memcpy(data,f4,140U);
    else if(c==1&&h==0&&r==3)memcpy(data,f1,140U);
    else memset(data,0,140U);
    return 140;
}
#include "twospace_dasd_status.inc"
static void half(unsigned char *p,unsigned int n){p[0]=(unsigned char)(n>>8);p[1]=(unsigned char)n;}
static TSGREQUEST setup(unsigned int cylinders,unsigned int capacity)
{
    TSGREQUEST request;
    assert(!owned);reads=copies=allocations=releases=bad_at=read_failure=alloc_failure=0U;active=1U;
    pointer_hi=0U;pointer_lo=4096U;
    memset(u,0xa5,sizeof u);memset(u+1024U,0,64U);cms_put_word(u+1024U,1U);cms_put_word(u+1028U,64U);
    cms_put_word(u+1036U,capacity);cms_put_word(u+1084U,4096U);
    memset(f4,0,140U);f4[44]=0xf4U;half(f4+62U,cylinders);half(f4+64U,15U);f4[74]=3U;
    half(f4+107U,1U);half(f4+111U,2U);half(f4+113U,14U);
    memset(f1,0,140U);f1[44]=0xf1U;f1[59]=1U;f1[105]=0x81U;
    half(f1+107U,cylinders-1U);half(f1+111U,cylinders-1U);half(f1+113U,14U);
    media_state.volume_count=1;media_state.volumes[0].device=7;media_state.volumes[0].address=0x1b9;
    memcpy(media_state.volumes[0].volser,"PDOS00",7U);
    memset(&request,0,sizeof request);request.address.lo=1024U;request.length=64U;return request;
}
static void failure(TSGREQUEST *request,unsigned int expected)
{
    unsigned char before[64];memcpy(before,u+1024U,64U);
    assert(media_status(request)==expected&&!memcmp(before,u+1024U,64U)&&!owned&&allocations==releases);
    assert(!copies&&u[4096U]==0xa5U&&u[4096U+TDA_MAX_TRACKS-1U]==0xa5U);
}
int main(void)
{
    TSGREQUEST request;unsigned int i,n;
    request=setup(100U,1500U);assert(!media_status(&request));
    assert(cms_word(u+1040U)==100U&&cms_word(u+1048U)==1500U&&cms_word(u+1056U)==15U);
    assert(u[4096U]==TDA_RESERVED&&u[4096U+1485U]==TDA_ALLOCATED&&u[4096U+1500U]==0xa5U);
    assert(allocations==1U&&releases==1U&&!owned);
    request=setup(1024U,0U);assert(!media_status(&request)&&copies==1U&&u[4096U]==0xa5U);
    assert(cms_word(u+1040U)==1024U&&cms_word(u+1048U)==15360U&&cms_word(u+1060U)==15300U);
    request=setup(1024U,0U);cms_put_word(u+1024U,TDA_GEOMETRY_QUERY);
    assert(!media_status(&request)&&reads==1U&&!allocations&&copies==1U&&cms_word(u+1048U)==15360U);
    for(i=28U;i<48U;++i)assert(!u[1024U+i]);
    request=setup(100U,1500U);cms_put_word(u+1024U,TDA_GEOMETRY_QUERY);failure(&request,8U);assert(!reads);
    request=setup(65535U,TDA_MAX_TRACKS);pointer_hi=1U;pointer_lo=0xffff0000U;
    cms_put_word(u+1080U,pointer_hi);cms_put_word(u+1084U,pointer_lo);
    assert(!media_status(&request)&&copies==9U&&allocations==releases);
    for(i=0U;i<TDA_MAX_TRACKS;++i){n=i<45U?TDA_RESERVED:i>=TDA_MAX_TRACKS-15U?TDA_ALLOCATED:0U;assert(u[4096U+i]==n);}
    assert(u[4096U+TDA_MAX_TRACKS]==0xa5U);
    request=setup(3U,1500U);assert(!media_status(&request)&&u[4096U+45U]==0xa5U);
    request=setup(1024U,1500U);failure(&request,8U);assert(reads==1U&&!allocations);
    request=setup(65535U,TDA_MAX_TRACKS);bad_at=4096U+TDS_TRANSFER_BYTES+8U;
    failure(&request,8U);assert(!reads&&!allocations);
    request=setup(100U,1500U);cms_put_word(u+1084U,1087U);failure(&request,8U);assert(!reads);
    request=setup(100U,1500U);cms_put_word(u+1084U,1023U);failure(&request,8U);assert(!reads);
    request=setup(100U,1500U);cms_put_word(u+1080U,0xffffffffU);cms_put_word(u+1084U,0xfffffffeU);
    failure(&request,8U);assert(!reads);
    request=setup(100U,TDA_MAX_TRACKS+1U);failure(&request,8U);assert(!reads);
    request=setup(100U,1500U);read_failure=5U;failure(&request,12U);assert(allocations==1U);
    request=setup(100U,1500U);half(f1+111U,100U);failure(&request,20U);assert(allocations==1U);
    request=setup(100U,1500U);alloc_failure=1U;failure(&request,4U);assert(reads==1U);
    request=setup(100U,1500U);active=0U;failure(&request,8U);assert(!reads);
    puts("K DASD status: declared geometry, high/carry spans, non-publication and owned cleanup PASS");return 0;
}
