/* SPDX-License-Identifier: MIT; native provider for the common I/O engine. */
struct channel_context {
    int (*notice)(TSCSTATE *,unsigned int,const unsigned char *,void *);
    void *argument;
};
static unsigned int channel_owner(void)
{const TSVFRAME *f=TSVTOP(&native_invocations);return f?f->token:0U;}
static int channel_start_native(unsigned int ssid,unsigned char *orb,unsigned char *irb,void *context)
{
    (void)context;
    if(TSCNOW((unsigned int *)(irb+128U)))return -1;
    return TSCPOST(ssid,orb,irb);
}
static int channel_poll_native(unsigned int ssid,unsigned char *irb,void *context)
{(void)context;return TSCPOLL(ssid,irb);}
static int channel_wait_native(unsigned int ssid,unsigned char *irb,unsigned int idle,void *context)
{(void)context;return idle?TSCWAITI(ssid,irb):TSCWAITN(ssid,irb);}
static int channel_clear_native(unsigned int ssid,unsigned char *irb,void *context)
{(void)context;return TSCCLEAR(ssid,irb);}
static int channel_notice_native(TSCSTATE *s,unsigned int ssid,const unsigned char *irb,void *context)
{
    CHANNELCONTEXT *c=(CHANNELCONTEXT *)context;
    if(channel_ahead.valid&&channel_ahead.ssid==ssid)channel_ahead.valid=0U;
    return c&&c->notice?c->notice(s,ssid,irb,c->argument):0;
}
static void channel_ops(TIOOPS *o,CHANNELCONTEXT *context)
{
    o->start=channel_start_native;o->poll=channel_poll_native;o->wait=channel_wait_native;
    o->clear=channel_clear_native;o->notice=channel_notice_native;o->context=context;
}
static int channel_run_notice(TSCSTATE *s,unsigned int ssid,CHANNELCONTEXT *context)
{
    TIOOPS ops;int rc;unsigned int at,command=0U;
    const unsigned char *base=s->aperture+s->region_real;
    for(at=TSC_CCW_OFFSET;at+8U<=TSC_SEEK_OFFSET;at+=8U)
        if(base[at]!=8U&&!(base[at+1U]&0xc0U)){command=base[at];break;}
    if(channel_ahead.valid&&channel_ahead.ssid==ssid&&(command==5U||command==13U||command==29U))channel_ahead.valid=0U;
    channel_ops(&ops,context);rc=TIOEXEC(&channel_io,s,ssid,channel_owner(),&ops);
    if(rc&&channel_ahead.ssid==ssid)channel_ahead.valid=0U;
    return rc;
}
static int channel_run(TSCSTATE *s,unsigned int ssid)
{return channel_run_notice(s,ssid,0);}
static int channel_take(TSCSTATE *s,unsigned int ssid,unsigned int owner,unsigned int *ticket)
{return TIOTAKE(&channel_io,s,ssid,owner,ticket);}
static int channel_submit(TSCSTATE *s,unsigned int owner,unsigned int ticket,CHANNELCONTEXT *context)
{TIOOPS ops;channel_ops(&ops,context);return TIOSUB(&channel_io,s,owner,ticket,&ops);}
static int channel_poll(TSCSTATE *s,unsigned int owner,unsigned int ticket,CHANNELCONTEXT *context)
{TIOOPS ops;channel_ops(&ops,context);return TIOPOLL(&channel_io,s,owner,ticket,&ops);}
static int channel_release(TSCSTATE *s,unsigned int owner,unsigned int ticket)
{return TIOFREE(&channel_io,s,owner,ticket);}
static int channel_cancel(TSCSTATE *s,unsigned int owner,unsigned int ticket)
{TIOOPS ops;channel_ops(&ops,0);return TIOCANCEL(&channel_io,s,owner,ticket,&ops);}
static int channel_quiesce(TSCSTATE *s,unsigned int ssid,unsigned int owner)
{TIOOPS ops;channel_ops(&ops,0);return TIOQUIESCE(&channel_io,s,ssid,owner,&ops);}
static int channel_vector(unsigned int ssid,unsigned int cylinder,unsigned int head,unsigned int record,unsigned int capacity,const unsigned char **data)
{
    unsigned int records,i,j,stride;unsigned char *target;
    if(!data||capacity<4096U||capacity>TSC_MAX_RECORD||!record||record>255U)return -1;
    ++channel_ahead.requests;
    if(channel_ahead.valid&&channel_ahead.ssid==ssid&&channel_ahead.cylinder==cylinder&&channel_ahead.head==head&&
       channel_ahead.bytes==capacity&&record>=channel_ahead.first&&record-channel_ahead.first<channel_ahead.records){
        *data=(unsigned char *)TSF_KAPERTURE_VA+channel_ahead.real+(record-channel_ahead.first)*capacity;++channel_ahead.hits;return (int)capacity;
    }
    /* Avoid an extra speculative miss at every track boundary. Interior
     * groups begin at record one; remaining requests use the normal path. */
    if(record!=1U)return -1;
    records=TSC_MAX_DATA/(capacity+8U);if(records>TSC_MAX_VECTOR)records=TSC_MAX_VECTOR;
    if(records>256U-record)records=256U-record;if(records<2U)return -1;
    channel_ahead.valid=0U;
    if(!channel_ahead.real&&TSRALLOC(&storage.real,0x54494f01U,TSC_REGION_BYTES,TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,&channel_ahead.real)!=TSR_OK)return -1;
    if(TSCBUILDV(&channel,cylinder,head,record,capacity,records)||channel_run(&channel,ssid)||TSCCHECKV(&channel,cylinder,head,record,capacity,records))return -1;
    target=(unsigned char *)TSF_KAPERTURE_VA+channel_ahead.real;stride=capacity+8U;
    for(i=0U;i<records;++i)for(j=0U;j<capacity;++j)target[i*capacity+j]=TSCDATA(&channel)[i*stride+8U+j];
    channel_ahead.ssid=ssid;channel_ahead.cylinder=cylinder;channel_ahead.head=head;channel_ahead.first=record;channel_ahead.records=records;channel_ahead.bytes=capacity;channel_ahead.valid=1U;++channel_ahead.batches;
    *data=target;return (int)capacity;
}
static int channel_missing(unsigned int ssid)
{
    const TIOREQUEST *r=TIOSTATE(&channel_io,&channel);unsigned int got;
    if(!r||!r->status_valid||!(r->status[8U]&2U)||r->status[9U]||channel.quarantined)return 0;
    if(TSCBUILDCONSCMD(&channel,4U,32U,1U)||channel_run(&channel,ssid)||TSCCHECKIN(&channel,32U,&got)||got<2U)return 0;
    /* CKD basic sense: byte 1, 08 is No Record Found. An equipment,
     * protection or other failure is never a track boundary. */
    return TSCDATA(&channel)[0U]==0U&&(TSCDATA(&channel)[1U]&8U)&&!(TSCDATA(&channel)[1U]&0xd7U);
}
/* Read-only, bounded engineering checkpoint. The U64 fixture owns its 64-KiB
 * span. Native return values, record identities and full byte comparison decide
 * correctness; STCK records costs after the provider's completion checks. */
static unsigned int channel_benchmark(const TSGREQUEST *request)
{
    static const unsigned char name[]={0xd2U,0xc3U,0xd6U,0xd9U,0xc5U,0x4bU,0xc2U,0xc9U,0xd5U};
    TSGCONTEXT gate;TSGREQUEST transfer=*request;TSKEXTENT extent;
    const TSVFRAME *f=native_caller(*(volatile const unsigned int *)0x308cU);
    unsigned char result[128],*scratch;const unsigned char *record;
    unsigned int clock[2],i,j,n,ssid,one_start,batch_start,code;
    volatile unsigned int *progress=(volatile unsigned int *)(TSF_KAPERTURE_VA+0x3ac00U);
    progress[0U]=0x50443235U;progress[1U]=1U;
    if(!f||f->personality!=TSV_TSO||f->amode!=64U||request->direction||request->length!=128U||
       !(*(volatile const unsigned int *)0x3080U&1U))return 8U;
    gate.u_tables=&u_tables;gate.real_aperture=(unsigned char *)TSF_KAPERTURE_VA;gate.real_bytes=TSF_REAL_BYTES;gate.rights=user_rights;gate.rights_context=0;
    transfer.length=65536U;transfer.direction=TSG_WRITE;if(TSGSPROB(&gate,&transfer))return 8U;
    transfer.direction=TSG_READ;if(TSGSPROB(&gate,&transfer))return 8U;
    transfer.length=128U;if(TSGSCOPY(&gate,&transfer,result,sizeof result)||cms_word(result)!=1U||cms_word(result+4U)!=128U)return 8U;
    if(!channel_bench_real&&TSRALLOC(&storage.real,0x54494201U,65536U,TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,&channel_bench_real)!=TSR_OK)return 4U;
    scratch=(unsigned char *)TSF_KAPERTURE_VA+channel_bench_real;ssid=*(volatile unsigned int *)0x40bcU;
    progress[1U]=2U;
    if(TSKFIND(channel_boot_record,&channel,name,sizeof name,&extent)||extent.block_length!=18452U||extent.record_format!=0x80U||!TSKWITHIN(&extent,extent.start_cylinder,extent.start_head))return 12U;
    progress[1U]=3U;progress[2U]=extent.start_cylinder;progress[3U]=extent.start_head;
    for(i=0U;i<3U;++i){if(channel_record_ssid(&channel,ssid,extent.start_cylinder,extent.start_head,i+1U,18452U,&record)!=18452)return 12U;
        for(j=0U;j<18452U;++j)scratch[i*18452U+j]=record[j];}
    for(i=0U;i<128U;++i)result[i]=0U;cms_put_word(result,1U);cms_put_word(result+4U,128U);cms_put_word(result+8U,0x50443235U);cms_put_word(result+12U,40U);
    one_start=channel_io.next_ticket;if(TSCNOW(clock))return 12U;cms_put_word(result+16U,clock[0]);cms_put_word(result+20U,clock[1]);
    progress[1U]=4U;
    for(n=0U;n<40U;++n)for(i=0U;i<3U;++i)if(channel_record_ssid(&channel,ssid,extent.start_cylinder,extent.start_head,i+1U,18452U,&record)!=18452)return 12U;
    if(TSCNOW(clock))return 12U;cms_put_word(result+24U,clock[0]);cms_put_word(result+28U,clock[1]);cms_put_word(result+48U,channel_io.next_ticket-one_start);
    for(j=0U;j<18452U;++j)if(record[j]!=scratch[2U*18452U+j])return 12U;
    batch_start=channel_io.next_ticket;if(TSCNOW(clock))return 12U;cms_put_word(result+32U,clock[0]);cms_put_word(result+36U,clock[1]);
    progress[1U]=5U;
    for(n=0U;n<40U;++n)if(TSCBUILDV(&channel,extent.start_cylinder,extent.start_head,1U,18452U,3U)||channel_run(&channel,ssid)||TSCCHECKV(&channel,extent.start_cylinder,extent.start_head,1U,18452U,3U))return 12U;
    if(TSCNOW(clock))return 12U;cms_put_word(result+40U,clock[0]);cms_put_word(result+44U,clock[1]);cms_put_word(result+52U,channel_io.next_ticket-batch_start);
    for(i=0U;i<3U;++i)for(j=0U;j<18452U;++j)if(TSCDATA(&channel)[i*18460U+8U+j]!=scratch[i*18452U+j])return 12U;
    cms_put_word(result+56U,18452U);cms_put_word(result+60U,1U);
    progress[1U]=6U;
    transfer.length=65536U;transfer.direction=TSG_READ;if(TSGSCOPY(&gate,&transfer,scratch,65536U))return 8U;
    if(TSCNOW(clock))return 12U;cms_put_word(result+64U,clock[0]);cms_put_word(result+68U,clock[1]);
    for(n=0U;n<100U;++n){transfer.direction=TSG_READ;code=TSGSCOPY(&gate,&transfer,scratch,65536U);if(code)return 8U;
        transfer.direction=TSG_WRITE;if(TSGSCOPY(&gate,&transfer,scratch,65536U))return 8U;}
    if(TSCNOW(clock))return 12U;cms_put_word(result+72U,clock[0]);cms_put_word(result+76U,clock[1]);cms_put_word(result+80U,100U);cms_put_word(result+84U,65536U);
    cms_put_word(result+88U,observation_scans);cms_put_word(result+92U,normal_boot()?1U:0U);
    cms_put_word(result+96U,channel_ahead.requests);cms_put_word(result+100U,channel_ahead.hits);cms_put_word(result+104U,channel_ahead.batches);
    progress[1U]=7U;
    transfer.length=128U;transfer.direction=TSG_WRITE;return TSGSCOPY(&gate,&transfer,result,sizeof result)?8U:0U;
}
