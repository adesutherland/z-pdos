/* SPDX-License-Identifier: MIT; checked K session provider for console devices. */
#define TD_REAL_OWNER 0x54445301U
static TDSPOOL device_sessions;
static unsigned int device_real,device_query_supported,device_query_boundary,device_query_bytes,device_raw_handle;
static unsigned char *device_query_data;
static T27CAPABILITIES device_advertised;
static unsigned int device_legacy_dx;
static TSCSTATE device_printer_channel;
static unsigned int device_printer_real,device_printer_address,device_printer_ssid;
static TSCSTATE *device_channel(const TDSSESSION *s)
{return s->device_class==TSA_DEVICE_PRINTER?&device_printer_channel:s->address==tc_cap.address?&console_channel:&tc_monitor_channel;}
static unsigned int device_ssid(const TDSSESSION *s)
{return s->device_class==TSA_DEVICE_PRINTER?device_printer_ssid:s->address==tc_cap.address?tc_ssid:tc_monitor_ssid;}
static unsigned int device_printer(unsigned int address)
{
    unsigned int n,ssid;
    if(device_printer_address)return device_printer_address==address?0U:20U;
    if(!device_printer_real){
        /* Native control linkage requires a workspace below16MiB. The
         * reserved bootstrap bank supplies a disjoint supervisor-only region;
         * application heaps must not determine whether a printer can open. */
        if(TSCINIT(&device_printer_channel,(unsigned char *)TSF_KAPERTURE_VA,TSF_REAL_BYTES,TTC_PRINTER_REAL)||
           TIOBIND(&channel_io,&device_printer_channel))return 12U;
        device_printer_real=TTC_PRINTER_REAL;
    }
    ssid=tc_find(address,&device_printer_channel);if(!ssid)return 20U;
    if(TSCBUILDCONSCMD(&device_printer_channel,0xe4U,7U,1U)||channel_run(&device_printer_channel,ssid)||
       TSCCHECKIN(&device_printer_channel,7U,&n))return 12U;
    if(n!=7U||TSCDATA(&device_printer_channel)[4U]!=0x32U||TSCDATA(&device_printer_channel)[5U]!=0x87U)return 20U;
    device_printer_address=address;device_printer_ssid=ssid;return 0U;
}
static unsigned int device_colours(void){return device_advertised.colours_valid;}
static unsigned int device_highlights(void){return device_advertised.highlighting;}
static unsigned int device_buffer(void){return !device_legacy_dx;}
static unsigned int console_device_geometry_check(TTCCAP *next,unsigned int ignore)
{
    unsigned int i;TDSSESSION *s;
    if(!next)return 8U;
    if(next->rows==tc_cap.rows&&next->columns==tc_cap.columns&&next->encoding==tc_cap.encoding&&next->generation==tc_cap.generation)return 0U;
    if(tc_attention_pending||tc_cap.generation>=0xfffffffeU)return 16U;
    if(next->generation<=tc_cap.generation)next->generation=tc_cap.generation+1U;
    for(i=0U;i<TDS_SLOTS;++i){s=&device_sessions.session[i];
        if(!s->handle||s->address!=tc_cap.address)continue;
        if(s->handle!=ignore&&s->phase!=TDS_READY)return 16U;
        if(s->count>TDS_EVENTS-2U||(s->sequence_hi==0xffffffffU&&s->sequence_lo>=0xfffffffeU))return 16U;
    }
    return 0U;
}
static unsigned int console_device_geometry(TTCCAP *next)
{
    T27GEOMETRY g;
    if(!next)return 8U;
    if(next->rows==tc_cap.rows&&next->columns==tc_cap.columns&&next->encoding==tc_cap.encoding&&next->generation==tc_cap.generation)return 0U;
    if(next->generation<=tc_cap.generation){if(tc_cap.generation==0xffffffffU)return 12U;next->generation=tc_cap.generation+1U;}
    if(T27GEOM(&g,next->rows,next->columns,next->encoding)||TDSGEOM(&device_sessions,tc_cap.address,&g,next->generation))return 12U;
    tc_cap=*next;tc_base_cap.generation=next->generation;tc_display_known=0U;return 0U;
}
static unsigned int console_device_reap(unsigned int owner)
{
    unsigned int i;TDSSESSION *s;
    for(i=0U;i<TDS_SLOTS;++i){s=&device_sessions.session[i];if(!s->handle||s->owner!=owner)continue;
        if(s->phase==TDS_ASSEMBLING||s->phase==TDS_SEALED){s->phase=TDS_READY;s->transfer_bytes=s->transfer_command=0U;}
        else if(s->phase==TDS_QUARANTINED||s->phase==TDS_SUBMITTED){
            TSCSTATE *c=device_channel(s);
            if(channel_quiesce(c,device_ssid(s),owner)||TDSFINISH(s,1U))return 12U;
        }
    }
    return 0U;
}
typedef struct {unsigned int attention;} TDQUERY;
static int device_query_notice(TSCSTATE *s,unsigned int ssid,const unsigned char *irb,void *argument)
{
    const TIOREQUEST *r=TIOSTATE(&channel_io,s);
    if((irb[8U]&0x80U)&&argument&&r){if(r->phase==TIO_PREPARED){tc_attention(ssid);return TIO_DEFER;}((TDQUERY *)argument)->attention=1U;}
    return 0;
}
static unsigned int device_query(void)
{
    unsigned int n,i,generation,legacy=0U;int status;T27CAPABILITIES cap;TTCCAP next;T27GEOMETRY geometry;
    CHANNELCONTEXT context;TDQUERY query;unsigned char *data;
    if(tc_cap.device_class!=TSA_DEVICE_3270)return 20U;
    if(tc_read_owner||console_read_phase||tc_attention_pending||tc_raw_owner||
       (tc_ready&&tc_input_owner&&!device_query_boundary))return 16U;
    for(i=0U;i<TDS_SLOTS;++i)if(device_sessions.session[i].handle&&device_sessions.session[i].address==tc_cap.address&&
       (device_sessions.session[i].phase!=TDS_READY||device_sessions.session[i].count>TDS_EVENTS-2U))return 16U;
    query.attention=0U;context.notice=device_query_notice;context.argument=&query;
    if(T27QUERY(tc_stream,TTC_MAX_STREAM,&n)||TSCBUILDCONSCMD(&console_channel,17U,n,0U))return 12U;
    for(i=0U;i<n;++i)TSCDATA(&console_channel)[i]=tc_stream[i];
    status=channel_run_notice(&console_channel,tc_ssid,&context);
    if(status==TIO_DEFER)return 16U;
    if(status){
        const TIOREQUEST *r=TIOSTATE(&channel_io,&console_channel);
        if(!r||!r->status_valid||!(r->status[8U]&2U)||r->status[9U]||console_channel.quarantined)return 12U;
        context.argument=0;
        if(TSCBUILDCONSCMD(&console_channel,4U,32U,1U)||channel_run_notice(&console_channel,tc_ssid,&context)||
           TSCCHECKIN(&console_channel,32U,&n)||!n)return 12U;
        if(TSCDATA(&console_channel)[0U]!=0x80U&&TSCDATA(&console_channel)[0U]!=0x01U)return 12U;
        {unsigned char *clear=(unsigned char *)&device_advertised;for(i=0U;i<sizeof device_advertised;++i)clear[i]=0U;}
        device_query_supported=device_query_bytes=device_legacy_dx=0U;tc_display_known=0U;return 20U;
    }
    if(TSCCHECKOUT(&console_channel))return 12U;
    while(!query.attention){status=(int)tc_wait(&console_channel,tc_ssid);if(status)return 12U;if(TSCIRB(&console_channel)[8U]&0x80U)query.attention=1U;}
    context.argument=0;
    if(TSCBUILDCONSCMD(&console_channel,6U,TSC_MAX_INPUT,1U)||channel_run_notice(&console_channel,tc_ssid,&context)||
       TSCCHECKIN(&console_channel,TSC_MAX_INPUT,&n))return 12U;
    data=TSCDATA(&console_channel);
    if(T27QRPLY(data,n,&cap)){
        if(T27DXQR(data,n,&cap))return 8U;
        legacy=1U;
    }
    next=tc_cap;
    if(cap.implicit_valid){if(TTCGEOM(&next,cap.default_rows,cap.default_columns,cap.alternate_rows,cap.alternate_columns))return 20U;}
    else if(cap.usable_valid){if(TTCGEOM(&next,cap.rows,cap.columns,cap.rows,cap.columns))return 20U;}
    if(next.generation!=tc_cap.generation){
        generation=next.generation;
        if(T27GEOM(&geometry,next.rows,next.columns,next.encoding)||TDSGEOM(&device_sessions,tc_cap.address,&geometry,generation))return 16U;
    }
    for(i=0U;i<n;++i)device_query_data[i]=data[i];device_query_bytes=n;
    device_advertised=cap;device_legacy_dx=legacy;device_query_supported=1U;tc_cap=next;tc_base_cap=next;
    tc_display_known=0U;device_query_boundary=0U;
    if(tc_ready){if(TTCVINIT(tc_view,&tc_cap))return 20U;return tc_render();}
    return 0U;
}
static unsigned int console_device_init(void)
{
    unsigned int rc;
    if(!device_real){
        if(TSRALLOC(&storage.real,TD_REAL_OWNER,(TDS_POOL_BYTES+TSC_MAX_INPUT+4095U)&~4095U,TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,&device_real)!=TSR_OK)return 4U;
        if(TDSINIT(&device_sessions,(unsigned char *)TSF_KAPERTURE_VA+device_real,TDS_POOL_BYTES))return 12U;
        device_query_data=(unsigned char *)TSF_KAPERTURE_VA+device_real+TDS_POOL_BYTES;
    }
    if(tc_cap.device_class!=TSA_DEVICE_3270)return 0U;
    rc=device_query();return rc==20U?0U:rc;
}
static unsigned int console_device_close(unsigned int owner,unsigned int handle)
{
    TDSSESSION *s=TDSFIND(&device_sessions,owner,handle);unsigned int ssid,rc,reaping;
    if(!s)return 8U;
    if(s->phase==TDS_SUBMITTED||s->phase==TDS_QUARANTINED){
        TSCSTATE *c=device_channel(s);
        ssid=device_ssid(s);
        if(channel_quiesce(c,ssid,owner)||TDSFINISH(s,1U))return 12U;
    }
    if(device_raw_handle==handle){TTCCAP base=tc_base_cap;
        if(tc_attention_pending){
            if(tc_attention_owner!=owner)return 16U;
            if(channel_quiesce(&console_channel,tc_ssid,owner))return 12U;
            tc_attention_pending=tc_attention_owner=0U;tc_gap(owner,3U);
        }
        /* Extended/character reply modes are endpoint state. Return native
         * line clients to their field mode before repainting the Workbench. */
        if(T27HAS(&device_advertised,0x88U)){
            unsigned int bytes;
            if(T27REPLY(0U,0,0U,tc_stream,TTC_MAX_STREAM,&bytes)||
               tc_output(&console_channel,tc_ssid,17U,tc_stream,bytes))return 12U;
        }
        /* Closing discards this session's records. Exclude it while reserving
         * immutable geometry notifications for all surviving sessions. */
        s->handle=0U;rc=console_device_geometry(&base);s->handle=handle;
        if(rc)return rc;
        reaping=native_invocations.depth&&native_invocations.frame[native_invocations.depth-1U].token==owner&&
                native_invocations.frame[native_invocations.depth-1U].state==TSV_REAPING;
        tc_raw_owner=0U;tc_display_known=0U;tc_input_owner=0U;
        if(!reaping){rc=tc_render();if(rc){tc_raw_owner=owner;s->phase=console_channel.quarantined?TDS_QUARANTINED:TDS_CLOSING;return rc;}}
        device_raw_handle=0U;
    }
    return TDSCLOSE(&device_sessions,owner,handle)?12U:0U;
}
static unsigned int device_receive(TDSSESSION *s,unsigned int command)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);TSCSTATE *c;
    unsigned int ssid,rc,n=0U,real;int status;CHANNELCONTEXT context;
    if(!f||!s||s->phase!=TDS_READY||s->count==TDS_EVENTS)return 16U;
    if(s->device_class==TSA_DEVICE_PRINTER)return 20U;
    if(s->address==tc_cap.monitor&&s->device_class==TSA_DEVICE_LINE){
        if(!tc_monitor_interactive)return 20U;
        tc_other_status(0U);if(tc_monitor_stale||tc_monitor_state!=TSA_MONITOR_ONLINE)return 16U;
    }
    if(s->device_class==TSA_DEVICE_LINE){if(command&&command!=0xf6U)return 20U;command=10U;}
    else{unsigned int input;if(!command)command=0xf6U;if(T27CMAP(command,&command,&input)||!input)return 20U;}
    if(s->device_class==TSA_DEVICE_3270&&command==2U&&device_legacy_dx)return 20U;
    if(tc_read_owner||console_read_phase||(tc_screen_owner&&tc_screen_owner!=f->token)||(tc_raw_owner&&tc_raw_owner!=f->token)||
       (s->device_class==TSA_DEVICE_3270&&tc_input_owner&&tc_input_owner!=f->token))return 16U;
    ssid=s->address==tc_cap.address?tc_ssid:tc_monitor_ssid;c=s->address==tc_cap.address?&console_channel:&tc_monitor_channel;
    if(c->quarantined)return 12U;
    if(TSVOWN(&native_invocations,f->token,TSV_TERMINAL,ssid)!=TSV_OK)return 16U;
    tc_read_owner=f->token;tc_read_ssid=ssid;
    if(ssid==tc_monitor_ssid&&tc_cap.device_class==TSA_DEVICE_3270){
        unsigned char unlock=0xc2U;tc_input_owner=f->token;
        rc=tc_write(&console_channel,tc_ssid,1U,&unlock,1U,1U);if(rc)goto done;
    }
    if(s->device_class==TSA_DEVICE_3270){
        rc=tc_claim_attention(f->token);if(rc)goto done;
        if(command==2U&&tc_input_owner&&tc_input_owner!=f->token){rc=16U;goto done;}
        while(command!=2U&&!tc_attention_pending){rc=tc_wait(c,ssid);if(rc)goto done;if(TSCIRB(c)[8U]&0x80U)tc_attention(ssid);}
    }
    real=(unsigned int)(s->transfer-(unsigned char *)TSF_KAPERTURE_VA);
    n=s->device_class==TSA_DEVICE_LINE?256U:TDS_RECORD_BYTES;
    if(TSCBUILDEXT(c,command,real,n,1U)||channel_take(c,ssid,f->token,&tc_read_handle)){rc=12U;goto done;}
    if(TSVOWN(&native_invocations,f->token,TSV_IO,tc_read_handle)!=TSV_OK){channel_cancel(c,f->token,tc_read_handle);tc_read_handle=0U;rc=4U;goto done;}
    context.notice=tc_notice;context.argument=0;
    status=channel_submit(c,f->token,tc_read_handle,&context);if(status){rc=12U;goto done;}
    tc_submitted=1U;if(ssid==tc_ssid)tc_attention_pending=tc_attention_owner=0U;
    for(;;){
        status=channel_poll(c,f->token,tc_read_handle,&context);
        if(status==TIO_OK){if(TSCCHECKEXT(c,n,1U,&n)){rc=12U;goto done;}break;}
        if(status!=TIO_PENDING){rc=12U;goto done;}
        tc_other_status(ssid);
        if(ssid==tc_monitor_ssid&&tc_cap.device_class==TSA_DEVICE_3270&&
           tc_attention_pending&&tc_attention_owner==f->token){
            if(console_v1_cancel(f->token))return 12U;
            tc_monitor_stale=1U;tc_gap(f->token,6U);rc=16U;goto done;
        }
        if((s->device_class==TSA_DEVICE_LINE?TSCWAITI(ssid,TSCIRB(c)):TSCWAITN(ssid,TSCIRB(c)))){rc=12U;goto done;}
    }
    if(TSVCOMPLETE(&native_invocations,f->token,tc_read_handle)!=TSV_OK||TSVFORGET(&native_invocations,f->token,TSV_IO,tc_read_handle)!=TSV_OK||channel_release(c,f->token,tc_read_handle)){rc=12U;goto done;}
    tc_read_handle=tc_submitted=0U;
    if(s->device_class==TSA_DEVICE_3270&&n==TDS_RECORD_BYTES){rc=TDSPOST(s,TDS_EVENT_TRUNCATED,s->transfer,n)?12U:0U;tc_gap(f->token,5U);goto done;}
    rc=TDSPOST(s,s->device_class==TSA_DEVICE_LINE?TDS_EVENT_LINE:command==2U?TDS_EVENT_BUFFER:n&&s->transfer[0U]==0x88U?TDS_EVENT_STRUCTURED:TDS_EVENT_KEY,s->transfer,n)?8U:0U;
    if(rc){if(TDSPOST(s,TDS_EVENT_INVALID,s->transfer,n))return 12U;tc_gap(f->token,5U);rc=0U;}
    if(!rc&&ssid==tc_ssid)device_query_boundary=1U;
done:
    if(console_v1_cancel(f->token))return 12U;tc_snapshot();return rc;
}
static unsigned int console_device_service(TSGREQUEST *request)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);TDSSESSION *s;const TDSEVENT *event;
    unsigned char header[64],caps[128];TSPADDR pointer;T27GEOMETRY geometry;TTCCAP cap;
    unsigned int op=request->direction,rc,handle,generation,flags,command,offset,capacity,i,n,real,ssid,input;int status;
    if(!f||*(volatile const unsigned int *)0x3000U||*(volatile const unsigned int *)0x3010U||request->length!=TSA_DEVICE_BYTES||tc_span(request->address,64U,TSG_READ,header,0U)||
       tc_span(request->address,64U,TSG_WRITE,header,0U)||tc_span(request->address,64U,TSG_READ,header,1U))return 8U;
    if(cms_word(header)!=TSA_VERSION||cms_word(header+4U)!=TSA_DEVICE_BYTES)return 20U;
    handle=cms_word(header+8U);generation=cms_word(header+12U);flags=cms_word(header+16U);command=cms_word(header+20U);
    offset=cms_word(header+24U);capacity=cms_word(header+28U);pointer.hi=cms_word(header+32U);pointer.lo=cms_word(header+36U);
    if(op!=TSA_IO_DEVICE_WRITE&&flags)return 8U;
    if(op==TSA_IO_DEVICE_CAPS&&(capacity<TSA_DEVICE_CAP_BYTES||tc_span(pointer,TSA_DEVICE_CAP_BYTES,TSG_WRITE,caps,0U)))return 8U;
    if((op==TSA_IO_DEVICE_READ||op==TSA_IO_DEVICE_EVENT)&&(capacity>TDS_RECORD_BYTES||(capacity&&tc_span(pointer,capacity,TSG_WRITE,caps,0U))))return 8U;
    if(op==TSA_IO_DEVICE_WRITE&&(flags&~TSA_DEVICE_END||capacity>TDS_TRANSFER_BYTES||offset>TDS_TRANSFER_BYTES||capacity>TDS_TRANSFER_BYTES-offset||
       (capacity&&tc_span(pointer,capacity,TSG_READ,caps,0U))))return 8U;
    rc=console_v1_init();if(rc)return rc;
    if(op==TSA_IO_DEVICE_OPEN){
        if(handle||generation||offset||capacity||pointer.hi||pointer.lo)return 8U;
        cap=tc_cap;
        if(command&&command!=tc_cap.address){
            if(command==tc_cap.monitor&&tc_monitor_ssid){if(TTCCONFIG(&cap,TSA_DEVICE_LINE,command,0U,0U))return 20U;}
            else{rc=device_printer(command);if(rc)return rc;cap.device_class=TSA_DEVICE_PRINTER;cap.address=command;cap.generation=1U;}
        }
        if(cap.device_class==TSA_DEVICE_3270&&T27GEOM(&geometry,cap.rows,cap.columns,cap.encoding))return 20U;
        if(TDSOPEN(&device_sessions,f->token,cap.address,cap.device_class,cap.device_class==TSA_DEVICE_3270?&geometry:0,cap.generation,&handle))return 4U;
        if(TSVOWN(&native_invocations,f->token,TSV_DEVICE_SESSION,handle)!=TSV_OK){TDSCLOSE(&device_sessions,f->token,handle);return 4U;}
        s=TDSFIND(&device_sessions,f->token,handle);
        if(cap.device_class==TSA_DEVICE_3270&&device_query_supported&&device_query_bytes&&TDSPOST(s,TDS_EVENT_STRUCTURED,device_query_data,device_query_bytes))return 12U;
        cms_put_word(header+8U,handle);cms_put_word(header+12U,cap.generation);
        return tc_span(request->address,64U,TSG_WRITE,header,1U);
    }
    s=TDSFIND(&device_sessions,f->token,handle);if(!s||TSVHAS(&native_invocations,f->token,TSV_DEVICE_SESSION,handle)!=TSV_OK)return 8U;
    if(op==TSA_IO_DEVICE_CLOSE){rc=console_device_close(f->token,handle);if(!rc&&TSVFORGET(&native_invocations,f->token,TSV_DEVICE_SESSION,handle)!=TSV_OK)return 12U;return rc;}
    if(op==TSA_IO_DEVICE_CAPS){
        if(capacity<TSA_DEVICE_CAP_BYTES||tc_span(pointer,TSA_DEVICE_CAP_BYTES,TSG_WRITE,caps,0U))return 8U;
        cap=tc_cap;if(s->device_class==TSA_DEVICE_LINE&&s->address!=tc_cap.address)TTCCONFIG(&cap,TSA_DEVICE_LINE,s->address,0U,0U);
        for(i=0U;i<128U;++i)caps[i]=0U;
        if(s->device_class==TSA_DEVICE_PRINTER){cms_put_word(caps,1U);cms_put_word(caps+4U,64U);cms_put_word(caps+8U,TSA_DEVICE_PRINTER);
            cms_put_word(caps+12U,s->address);cms_put_word(caps+48U,TSA_FEATURE_PRINTER);cms_put_word(caps+60U,s->generation);}
        else{TTCCAPS(&cap,caps);if(s->address==tc_cap.monitor&&!tc_monitor_interactive)cms_put_word(caps+48U,cap.features&~TSA_FEATURE_LINE_INPUT);}
        if(s->device_class==TSA_DEVICE_3270&&!device_legacy_dx)cms_put_word(caps+TSA_CAP_FEATURES,cap.features|TSA_FEATURE_READ_BUFFER);
        cms_put_word(caps+64U,s->device_class==TSA_DEVICE_PRINTER?0x0fU:s->device_class==TSA_DEVICE_LINE?
            s->address==tc_cap.monitor&&!tc_monitor_interactive?0x6fU:0x7fU:0xffU);
        cms_put_word(caps+72U,s->device_class==TSA_DEVICE_PRINTER?0U:s->device_class==TSA_DEVICE_LINE?256U:TDS_RECORD_BYTES);
        cms_put_word(caps+76U,s->device_class==TSA_DEVICE_LINE?256U:TDS_TRANSFER_BYTES);
        if(s->device_class==TSA_DEVICE_3270){for(i=0U;i<16U;++i)caps[80U+i]=device_advertised.colours[i];for(i=0U;i<32U;++i)caps[96U+i]=device_advertised.replies[i];}
        cms_put_word(header+12U,s->generation);cms_put_word(header+48U,128U);
        if(tc_span(pointer,128U,TSG_WRITE,caps,1U))return 8U;return tc_span(request->address,64U,TSG_WRITE,header,1U);
    }
    if(op==TSA_IO_DEVICE_ACK)return TDSACK(s,cms_word(header+40U),cms_word(header+44U))?8U:0U;
    if(generation!=s->generation)return 16U;
    if(op==TSA_IO_DEVICE_QUERY){if(s->address!=tc_cap.address)return 20U;rc=device_query();if(rc)return rc;return TDSPOST(s,TDS_EVENT_STRUCTURED,device_query_data,device_query_bytes)?16U:0U;}
    if(op==TSA_IO_DEVICE_READ){
        if(capacity>TDS_RECORD_BYTES||(capacity&&tc_span(pointer,capacity,TSG_WRITE,caps,0U)))return 8U;
        if(!s->count){rc=device_receive(s,command);if(rc)return rc;}
    }
    if(op==TSA_IO_DEVICE_EVENT||op==TSA_IO_DEVICE_READ){
        status=TDSPEEK(s,&event);if(status==TDS_EMPTY)return 16U;if(status)return 8U;
        if(!(op==TSA_IO_DEVICE_READ&&!capacity)&&(capacity<event->bytes||(event->bytes&&tc_span(pointer,event->bytes,TSG_WRITE,(unsigned char *)event->data,0U))))return 8U;
        cms_put_word(header+12U,event->generation);cms_put_word(header+16U,event->cursor_valid?TSA_DEVICE_CURSOR_VALID:0U);
        cms_put_word(header+20U,event->aid);cms_put_word(header+40U,event->sequence_hi);cms_put_word(header+44U,event->sequence_lo);
        cms_put_word(header+48U,event->bytes);cms_put_word(header+52U,event->type);cms_put_word(header+56U,event->cursor);cms_put_word(header+60U,event->fields);
        if(event->bytes&&capacity&&tc_span(pointer,event->bytes,TSG_WRITE,(unsigned char *)event->data,1U))return 8U;
        return tc_span(request->address,64U,TSG_WRITE,header,1U);
    }
    if(op!=TSA_IO_DEVICE_WRITE||flags&~TSA_DEVICE_END||capacity>TDS_TRANSFER_BYTES||offset>TDS_TRANSFER_BYTES||capacity>TDS_TRANSFER_BYTES-offset)return 8U;
    if(capacity&&tc_span(pointer,capacity,TSG_READ,s->transfer+offset,0U))return 8U;
    if(T27CMAP(command,&n,&input)||input)return 20U;
    if(s->device_class==TSA_DEVICE_PRINTER&&n!=1U)return 20U;
    if(s->device_class==TSA_DEVICE_LINE&&(n!=1U||offset+capacity>256U))return 20U;
    if(s->phase==TDS_SEALED&&flags==TSA_DEVICE_END&&!capacity&&offset==s->transfer_bytes&&command==s->transfer_command)status=0;
    else{
        if(s->phase!=TDS_READY&&s->phase!=TDS_ASSEMBLING)return 16U;
        if((s->phase==TDS_READY&&offset)||(s->phase==TDS_ASSEMBLING&&(offset!=s->transfer_bytes||command!=s->transfer_command))||
           (flags&&s->transfer_sequence==0xffffffffU))return 8U;
        if(capacity&&tc_span(pointer,capacity,TSG_READ,s->transfer+offset,1U))return 8U;
        status=TDSFRAGMENT(s,generation,command,offset,s->transfer+offset,capacity,flags?1U:0U);
    }
    if(status)return status==TDS_BUSY?16U:8U;
    if(!flags)return 0U;
    cap=tc_cap;geometry=s->geometry;
    if(s->device_class==TSA_DEVICE_3270&&(n==5U||n==13U)){
        if(tc_attention_pending)return 16U;
        cap.rows=n==5U?cap.default_rows:cap.alternate_rows;cap.columns=n==5U?cap.default_columns:cap.alternate_columns;
        cap.encoding=cap.rows*cap.columns<=4096U?TSA_ADDRESS_CODED12:TSA_ADDRESS_BINARY14;
        if(console_device_geometry_check(&cap,handle)||T27GEOM(&geometry,cap.rows,cap.columns,cap.encoding))return 16U;
    }
    if(n==17U){unsigned int at=0U;T27FIELD field;while(at<s->transfer_bytes)if(T27WALK(s->transfer,s->transfer_bytes,&at,&field))return 8U;}
    else if(s->device_class==TSA_DEVICE_PRINTER&&T27PRWRT(s->transfer,s->transfer_bytes))return 20U;
    else if(n!=15U&&s->device_class==TSA_DEVICE_3270&&T27WRITE(&geometry,s->transfer,s->transfer_bytes))return 8U;
    if(s->device_class==TSA_DEVICE_LINE){for(i=0U;i<s->transfer_bytes;++i)if(s->transfer[i]<0x40U&&s->transfer[i]!=0x15U)return 8U;}
    if(n==15U&&s->transfer_bytes!=1U)return 8U;
    if(tc_read_owner||console_read_phase||(tc_screen_owner&&tc_screen_owner!=f->token)||(tc_raw_owner&&tc_raw_owner!=f->token)||
       (s->device_class==TSA_DEVICE_3270&&tc_input_owner&&tc_input_owner!=f->token))return 16U;
    ssid=device_ssid(s);
    {TSCSTATE *c=device_channel(s);
     real=(unsigned int)(s->transfer-(unsigned char *)TSF_KAPERTURE_VA);
     if(TSCBUILDEXT(c,n,real,s->transfer_bytes,0U))return 16U;
     if(ssid==tc_ssid)tc_display_known=0U;
     if(TDSSUBMIT(s))return 12U;
     if(ssid==tc_ssid&&s->device_class==TSA_DEVICE_3270){tc_raw_owner=f->token;device_raw_handle=handle;device_query_boundary=0U;}
     {CHANNELCONTEXT context;TCNOTICE notice;notice.command=n;notice.prompt=0U;notice.text=s->transfer;
      context.notice=s->device_class==TSA_DEVICE_PRINTER?0:tc_notice;context.argument=&notice;status=channel_run_notice(c,ssid,&context);}
     if(TDSFINISH(s,c->quarantined?0U:1U))return 12U;
     if(status||TSCCHECKEXT(c,offset+capacity,0U,&i)){if(s->device_class!=TSA_DEVICE_PRINTER)tc_gap(f->token,2U);return 12U;}}
    if(ssid==tc_ssid&&s->device_class==TSA_DEVICE_3270){
        if(console_device_geometry(&cap))return 12U;
        tc_raw_owner=f->token;device_raw_handle=handle;device_query_boundary=0U;
        if(TTXAPPEND(&tc_log,TSA_TRANSCRIPT_SCREEN_CHANGE,f->token,0U,TSA_TRANSCRIPT_RAW_SCREEN,s->transfer,offset+capacity))return 24U;
        tc_gap(f->token,2U);
    }
    cms_put_word(header+48U,i);tc_snapshot();return tc_span(request->address,64U,TSG_WRITE,header,1U);
}
