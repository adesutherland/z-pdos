/* SPDX-License-Identifier: MIT; actual endpoint/provider code with event stubs. */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "twospace_io.h"
#include "twospace_console.h"
#include "twospace_invocation.h"
#include "twospace_real.h"
typedef struct channel_context CHANNELCONTEXT;
static TIOPOOL channel_io;static TSCSTATE console_channel,tc_monitor_channel;
static unsigned int tc_ssid=1U,tc_monitor_ssid=2U,console_ssid=1U,console_read_phase;
static unsigned int tc_attention_pending,tc_attention_owner,tc_input_owner,tc_screen_owner,tc_monitor_stale,tc_monitor_state,tc_display_known;
static unsigned int tc_read_owner,tc_read_ssid,tc_read_handle,tc_submitted,tc_next_input;
static TTCCAP tc_cap;static TSVSTACK native_invocations;static TSVFRAME active;
static struct {unsigned int valid,ssid,cylinder,head,bytes,first,records,real,requests,hits,batches;} channel_ahead;
static struct {TSRPLAN real;} storage;
static unsigned char *aperture;
#define TSF_KAPERTURE_VA aperture
#define TSF_CORE_BYTES 0x10000U
#define TSF_REAL_BYTES 0x30000U
#define channel console_channel
static unsigned int vector_mode,bad_vector;
int TSRALLOC(TSRPLAN *p,unsigned int owner,unsigned int bytes,unsigned int lo,unsigned int hi,unsigned int kind,unsigned int *real)
{(void)p;(void)owner;(void)lo;(void)hi;(void)kind;assert(bytes==65536U);*real=0x20000U;return 0;}
static unsigned int gaps,starts,polls,waits,clears;static int start_rc[8],wait_error,clear_error;
static unsigned int pending[3],complete[3],status[8],no_complete,events_after_wait[4];
const TSVFRAME *TSVTOP(const TSVSTACK *s){(void)s;return &active;}
int TSVCOMPLETE(TSVSTACK *s,unsigned int owner,unsigned int handle){(void)s;(void)owner;(void)handle;return 0;}
int TSVFORGET(TSVSTACK *s,unsigned int owner,unsigned int kind,unsigned int handle){(void)s;(void)owner;(void)kind;(void)handle;return 0;}
static void tc_gap(unsigned int token,unsigned int reason){(void)token;(void)reason;++gaps;}
static void tc_snapshot(void){}
static void word(unsigned char *p,unsigned int n){p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
int TSCNOW(unsigned int *p){p[0]=1U;p[1]=0U;return 0;}
int TSCPOST(unsigned int ssid,unsigned char *orb,unsigned char *irb){int rc=start_rc[starts];(void)orb;(void)irb;if(rc==-2)pending[ssid]=status[starts];else if(!rc&&!no_complete)complete[ssid]=status[starts];++starts;
 if(vector_mode&&!rc){unsigned int i;unsigned char *p=TSCDATA(&console_channel);for(i=0U;i<3U;++i){p[0]=0U;p[1]=3U;p[2]=0U;p[3]=4U;p[4]=(unsigned char)(i+1U);p[5]=0U;p[6]=0x10U;p[7]=(unsigned char)(bad_vector&&i==1U?1U:0U);memset(p+8U,(int)(0x50U+i+ssid),4096U);p+=4104U;}}
 return rc;}
int TSCPOLL(unsigned int ssid,unsigned char *irb){unsigned int n=0U;++polls;if(pending[ssid]){n=pending[ssid];pending[ssid]=0U;}else if(complete[ssid]){n=complete[ssid];complete[ssid]=0U;}if(!n)return 1;memset(irb,0,64U);irb[8U]=(unsigned char)n;irb[3U]=7U;word(irb+4U,vector_mode?0x250U:ssid==1U?0x208U:0x10208U);return 0;}
int TSCWAITN(unsigned int ssid,unsigned char *irb){(void)ssid;(void)irb;++waits;return wait_error||waits>4U?-1:0;}
int TSCWAITI(unsigned int ssid,unsigned char *irb){(void)ssid;(void)irb;if(waits>=4U)return -1;pending[2U]=events_after_wait[waits++];return 0;}
int TSCCLEAR(unsigned int ssid,unsigned char *irb){(void)ssid;++clears;if(clear_error)return -1;memset(irb,0,64U);irb[2U]=0x10U;return 0;}
#include "channel-provider.inc"
#include "channel-cache.inc"
#include "console-functions.inc"
static void reset(unsigned char *memory)
{
    unsigned int i;aperture=memory;TIOINIT(&channel_io);assert(!TSCINIT(&console_channel,memory,0x30000U,0U));assert(!TSCINIT(&tc_monitor_channel,memory,0x30000U,0x10000U));assert(!TIOBIND(&channel_io,&console_channel)&&!TIOBIND(&channel_io,&tc_monitor_channel));
    memset(&channel_ahead,0,sizeof channel_ahead);vector_mode=bad_vector=0U;
    gaps=starts=polls=waits=clears=tc_attention_pending=tc_attention_owner=tc_monitor_stale=tc_read_owner=tc_read_ssid=tc_read_handle=tc_submitted=tc_next_input=tc_screen_owner=console_read_phase=no_complete=0U;
    wait_error=clear_error=0;active.token=tc_input_owner=42U;tc_cap.device_class=TSA_DEVICE_3270;
    for(i=0U;i<8U;++i){start_rc[i]=0;status[i]=12U;}
    memset(pending,0,sizeof pending);memset(complete,0,sizeof complete);memset(events_after_wait,0,sizeof events_after_wait);
}
int main(void)
{
    unsigned char *memory=(unsigned char *)calloc(1,0x30000U);unsigned char text[4]={0x40U,0x11U,0x40U,0x40U},unlock[1]={0xc2U};const unsigned char *data;unsigned int n,ticket,i;
    assert(memory);
    reset(memory);start_rc[0]=-2;status[0]=0x80U;active.token=99U;
    assert(!tc_output(&console_channel,1U,1U,text,4U)&&starts==2U&&tc_attention_pending&&tc_attention_owner==42U);
    assert(!tc_write(&console_channel,1U,1U,unlock,1U,1U)&&starts==2U);
    reset(memory);start_rc[0]=-2;status[0]=0x80U;
    assert(!tc_write(&console_channel,1U,1U,unlock,1U,1U)&&starts==1U&&tc_attention_pending);
    reset(memory);status[0]=0x8cU;tc_read_owner=77U;
    assert(!tc_write(&console_channel,1U,1U,unlock,1U,1U)&&tc_attention_owner==77U);
    reset(memory);start_rc[0]=-2;status[0]=0x82U;
    assert(tc_output(&console_channel,1U,1U,text,4U)==12U&&!tc_attention_pending);
    reset(memory);no_complete=1U;wait_error=clear_error=-1;
    assert(tc_output(&console_channel,1U,1U,text,4U)==12U&&console_channel.quarantined&&clears==1U);
    assert(tc_output(&console_channel,1U,1U,text,4U)==12U&&starts==1U);
    reset(memory);status[0]=0x8cU;
    assert(!tc_sense(&console_channel,1U,&n)&&n==7U&&tc_attention_owner==42U);
    reset(memory);no_complete=1U;wait_error=clear_error=-1;
    assert(tc_sense(&console_channel,1U,&n)==12U&&console_channel.quarantined);
    reset(memory);pending[1U]=0x80U;events_after_wait[1U]=0x80U;
    assert(!tc_wait(&tc_monitor_channel,2U)&&waits==2U&&tc_attention_owner==42U);
    reset(memory);events_after_wait[0U]=0x82U;assert(tc_wait(&tc_monitor_channel,2U)==12U&&waits==1U);
    reset(memory);tc_attention_pending=1U;tc_attention_owner=42U;tc_read_owner=99U;active.token=99U;
    assert(tc_claim_attention(99U)==16U&&!starts&&tc_attention_owner==42U);
    tc_attention_owner=0U;assert(!tc_claim_attention(99U)&&gaps==1U&&!tc_attention_pending&&tc_input_owner==99U);
    reset(memory);tc_cap.device_class=TSA_DEVICE_LINE;status[0]=0x8cU;
    assert(!tc_output(&console_channel,1U,1U,text,4U)&&!tc_attention_pending);
    reset(memory);console_read_phase=2U;TSCDATA(&console_channel)[0U]=0x5aU;
    assert(tc_output(&console_channel,1U,1U,text,4U)==16U&&!starts&&TSCDATA(&console_channel)[0U]==0x5aU);
    assert(!tc_output(&tc_monitor_channel,2U,1U,text,4U));
    reset(memory);no_complete=1U;assert(!TSCBUILDCONSREAD(&console_channel,4096U));assert(!channel_take(&console_channel,1U,42U,&ticket));assert(!tc_post_read(&console_channel,42U,ticket));
    tc_read_owner=42U;tc_read_ssid=1U;tc_read_handle=ticket;tc_submitted=1U;clear_error=-1;
    assert(console_v1_cancel(43U)==8U&&!clears);assert(console_v1_cancel(42U)==12U&&console_channel.quarantined&&tc_read_owner==42U);
    clear_error=0;assert(!console_v1_cancel(42U)&&!console_channel.quarantined&&!tc_read_owner&&!tc_submitted);
    assert(tc_legacy_control(209U)&&tc_legacy_control(239U)&&!tc_legacy_control(200U)&&!tc_legacy_control(93U));
    reset(memory);vector_mode=1U;
    assert(channel_vector(1U,3U,4U,1U,4096U,&data)==4096&&starts==1U&&channel_ahead.batches==1U);
    for(i=0U;i<4096U;++i)assert(data[i]==0x51U);
    assert(channel_vector(1U,3U,4U,2U,4096U,&data)==4096&&starts==1U);
    for(i=0U;i<4096U;++i)assert(data[i]==0x52U);
    assert(channel_vector(1U,3U,4U,3U,4096U,&data)==4096&&starts==1U&&channel_ahead.hits==2U);
    assert(channel_vector(1U,3U,4U,4U,4096U,&data)==-1&&starts==1U);
    assert(channel_vector(2U,3U,4U,1U,4096U,&data)==4096&&starts==2U&&data[0]==0x52U);
    assert(channel_vector(2U,3U,5U,2U,4096U,&data)==-1&&starts==2U);
    assert(!channel_notice_native(&console_channel,2U,TSCIRB(&console_channel),0)&&!channel_ahead.valid);
    reset(memory);vector_mode=bad_vector=1U;data=0;
    assert(channel_vector(1U,3U,4U,1U,4096U,&data)==-1&&!data&&!channel_ahead.valid&&!channel_ahead.batches);
    reset(memory);channel_ahead.valid=channel_ahead.ssid=1U;
    assert(!TSCBUILDCONSCMD(&console_channel,5U,4U,0U)&&!channel_run(&console_channel,1U)&&!channel_ahead.valid);
    reset(memory);channel_ahead.valid=channel_ahead.ssid=1U;start_rc[0]=-1;
    assert(!TSCBUILDCONSCMD(&console_channel,1U,4U,0U)&&channel_run(&console_channel,1U)&&!channel_ahead.valid);
    free(memory);return 0;
}
