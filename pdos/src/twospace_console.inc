/* SPDX-License-Identifier: MIT
 * K C31 console/device endpoint. Physical I/O uses only the channel primitive;
 * every U span is checked before a device operation or ownership change. */
#define TC_WORK_OWNER 0x54435701U
#define TC_LOG_OWNER 0x54545801U
#define TC_LOG_BYTES 0x200000U
static TTCCAP tc_cap,tc_base_cap;
static TTCVIEW *tc_view;
static TTXSTATE tc_log;
static TSCSTATE tc_monitor_channel;
static unsigned int tc_ready,tc_ssid,tc_monitor_ssid,tc_monitor_state;
static unsigned int tc_next_input,tc_read_owner,tc_read_ssid,tc_read_handle,tc_submitted;
static unsigned int tc_monitor_stale,tc_screen_owner,tc_raw_owner,tc_input_field;
static unsigned int tc_monitor_gap_notice;
static unsigned int tc_display_known,tc_input_end;
static unsigned int tc_attention_pending,tc_attention_owner;
static unsigned int tc_input_owner;
static unsigned int tc_work_real,tc_log_real;
static unsigned char *tc_stream,*tc_staging,*tc_raw,*tc_header_cells,*tc_footer_cells;
static unsigned char tc_headers[TSV_MAX_DEPTH+1U][132];
static unsigned int tc_header_lengths[TSV_MAX_DEPTH+1U];
static const unsigned char tc_default_header[]={0xa9,0x61,0xd7,0xc4,0xd6,0xe2};
static const unsigned char tc_default_footer[]={0xc5,0x95,0xa3,0x85,0x99,0x40,0x81,0x40,0x83,0x96,0x94,0x94,0x81,0x95,0x84};

static int console_v1_enabled(void)
{return *(volatile const unsigned int *)(TSF_KAPERTURE_VA+TTC_CONFIG_REAL)==TTC_CONFIG_MAGIC;}

static TSPADDR tc_plus(TSPADDR address,unsigned int bytes)
{TSPADDR result;result.lo=address.lo+bytes;result.hi=address.hi+(result.lo<address.lo);return result;}
static unsigned int tc_span(TSPADDR address,unsigned int bytes,unsigned int direction,
                             unsigned char *data,unsigned int copy)
{
    TSGCONTEXT gate;TSGREQUEST request;
    if(bytes>TSA_SCREEN_BYTES)return 8U;
    if(!bytes)return 0U;
    gate.u_tables=&u_tables;gate.real_aperture=(unsigned char *)TSF_KAPERTURE_VA;
    gate.real_bytes=TSF_REAL_BYTES;gate.rights=user_rights;gate.rights_context=0;
    request.direction=direction;request.svc=200U;
    request.address=address;request.length=bytes;
    return (copy?TSGSCOPY(&gate,&request,data,bytes):TSGSPROB(&gate,&request))==TSG_OK?0U:8U;
}
static unsigned int tc_find(unsigned int device,TSCSTATE *channel_state)
{
    unsigned int i;
    for(i=0U;i<256U;++i)
        if(TSCDEV(0x10000U+i,TSCSCHIB(channel_state))==(int)device)
            return TSCENABL(0x10000U+i,TSCSCHIB(channel_state))?0U:0x10000U+i;
    return 0U;
}
static void tc_gap(unsigned int,unsigned int);
static unsigned int tc_abort_output(TSCSTATE *device,unsigned int ssid)
{
    const TIOREQUEST *r=TIOSTATE(&channel_io,device);
    (void)ssid;
    if(r&&(r->phase==TIO_ACTIVE||r->phase==TIO_QUARANTINED))channel_cancel(device,r->owner,r->ticket);
    return 12U;
}
static void tc_attention(unsigned int ssid)
{
    if(ssid==tc_ssid){
        /* Read Inquiry owns line input; it has no 3270 field/AID lease. */
        if(tc_cap.device_class!=TSA_DEVICE_3270)return;
        tc_attention_pending=1U;tc_attention_owner=tc_input_owner;
    }else{tc_monitor_stale=1U;tc_gap(0U,3U);}
}
typedef struct {
    unsigned int prompt,command;
    const unsigned char *text;
} TCNOTICE;
static void tc_unlock_owner(unsigned int command,const unsigned char *text)
{
    if(tc_cap.device_class==TSA_DEVICE_3270&&
       (command==15U||((command==1U||command==5U||command==13U)&&(text[0U]&2U)))){
        const TSVFRAME *active=TSVTOP(&native_invocations);
        tc_input_owner=tc_read_owner?tc_read_owner:tc_screen_owner?tc_screen_owner:active?active->token:0U;
    }
}
static int tc_notice(TSCSTATE *device,unsigned int ssid,const unsigned char *irb,void *argument)
{
    TCNOTICE *n=(TCNOTICE *)argument;
    const TIOREQUEST *r=TIOSTATE(&channel_io,device);
    if(irb[8U]&0x80U){
        if(n&&r&&r->phase==TIO_ACTIVE&&(irb[8U]&0x0cU)==0x0cU&&ssid==tc_ssid)tc_unlock_owner(n->command,n->text);
        tc_attention(ssid);
        if(n&&n->prompt&&r&&r->phase==TIO_PREPARED&&ssid==tc_ssid)return TIO_DEFER;
    }
    return 0;
}
static unsigned int tc_write(TSCSTATE *device,unsigned int ssid,unsigned int command,
                               const unsigned char *text,unsigned int length,unsigned int prompt)
{
    unsigned int i;int rc;TCNOTICE notice;CHANNELCONTEXT context;
    if(!device||!device->aperture||!ssid||!text||!length||length>TSC_MAX_CONSOLE||device->quarantined)return 12U;
    if(ssid==console_ssid&&console_read_phase)return 16U;
    if(ssid==tc_read_ssid&&tc_submitted)return 16U;
    if(prompt&&ssid==tc_ssid&&tc_attention_pending)return 0U;
    if(TSCBUILDCONSCMD(device,command,length,0U)!=TSC_OK)return 12U;
    for(i=0U;i<length;++i)TSCDATA(device)[i]=text[i];
    notice.prompt=prompt;notice.command=command;notice.text=text;context.notice=tc_notice;context.argument=&notice;
    rc=channel_run_notice(device,ssid,&context);
    if(rc==TIO_DEFER)return 0U;
    if(rc)return rc==TIO_BUSY?16U:12U;
    if(TSCCHECKOUT(device)!=TSC_OK)return 12U;
    if(ssid==tc_ssid){
        tc_unlock_owner(command,text);
        if(!(TSCIRB(device)[8U]&0x80U)&&tc_cap.device_class==TSA_DEVICE_3270&&
           (command==15U||((command==1U||command==5U||command==13U)&&(text[0U]&2U))))tc_attention_pending=tc_attention_owner=0U;
    }
    return 0U;
}
static unsigned int tc_output(TSCSTATE *device,unsigned int ssid,unsigned int command,
                               const unsigned char *text,unsigned int length)
{return tc_write(device,ssid,command,text,length,0U);}
static unsigned int tc_post_read(TSCSTATE *device,unsigned int owner,unsigned int ticket)
{
    CHANNELCONTEXT context;int rc;
    context.notice=tc_notice;context.argument=0;rc=channel_submit(device,owner,ticket,&context);
    return rc?rc==TIO_BUSY?16U:12U:0U;
}
static unsigned int tc_sense(TSCSTATE *device,unsigned int ssid,unsigned int *count)
{
    CHANNELCONTEXT context;
    if(TSCBUILDCONSCMD(device,0xe4U,7U,1U)!=TSC_OK)return 12U;
    context.notice=tc_notice;context.argument=0;
    if(channel_run_notice(device,ssid,&context)||TSCCHECKIN(device,7U,count)!=TSC_OK)return 12U;
    return 0U;
}
static unsigned int tc_claim_attention(unsigned int token)
{
    unsigned char unused=0U;
    if(tc_cap.device_class!=TSA_DEVICE_3270||!tc_attention_pending||tc_attention_owner==token)return 0U;
    if(tc_attention_owner)return 16U;
    /* Unowned input is quarantined rather than delivered to the next caller.
     * A suspended parent's pending record remains available to that parent. */
    tc_gap(0U,3U);
    return tc_output(&console_channel,tc_ssid,15U,&unused,1U);
}
static int tc_legacy_control(unsigned int svc)
{
    /* These pre-versioned asynchronous test controls use the legacy shared
     * workspace. They are not native CMS/TSO or v1 presentation operations. */
    return svc==209U||svc==210U||svc==239U||svc==240U||svc==241U||svc==243U||svc==248U;
}
static void tc_gap(unsigned int token,unsigned int reason)
{
    unsigned char payload[4];cms_put_word(payload,reason);
    tc_monitor_gap_notice=1U;
    if(tc_ready)TTXAPPEND(&tc_log,TSA_TRANSCRIPT_GAP,token,0U,reason,payload,4U);
}
static unsigned int tc_line_output(TSCSTATE *device,unsigned int ssid,
                                    const unsigned char *text,unsigned int length)
{
    static const unsigned char newline[1]={0x15U};unsigned int at,n,rc;
    if(!length)return tc_output(device,ssid,1U,newline,1U);
    for(at=0U;at<length;at+=n){
        n=length-at>148U?148U:length-at;
        rc=tc_output(device,ssid,at+n==length?9U:1U,text+at,n);if(rc)return rc;
    }
    return 0U;
}
static void tc_snapshot(void)
{
    volatile unsigned int *out=(volatile unsigned int *)(TSF_KAPERTURE_VA+0x3aa00U);
    out[0]=0x434f4e31U;out[1]=tc_log_real;out[2]=tc_log.used;
    out[3]=tc_log.sequence_hi;out[4]=tc_log.sequence_lo;out[5]=tc_log.gaps;
    out[6]=tc_monitor_state;out[7]=tc_read_owner;out[8]=tc_screen_owner;
    out[9]=tc_cap.rows;out[10]=tc_cap.columns;out[11]=tc_cap.device_class;
}
static unsigned int tc_render(void)
{
    unsigned int bytes,depth=native_invocations.depth,rc,i,j,n,origin;
    TTCFIELD fields[4];
    const unsigned char *header=tc_headers[depth];unsigned int hb=tc_header_lengths[depth];
    if(tc_cap.device_class!=TSA_DEVICE_3270)return 0U;
    if(!hb){header=tc_default_header;hb=(unsigned int)sizeof tc_default_header;}
    n=tc_cap.columns-1U;
    for(i=0U;i<n;++i){
        tc_header_cells[i]=i<hb?header[i]:0x40U;
        tc_footer_cells[i]=i<sizeof tc_default_footer?tc_default_footer[i]:0x40U;
    }
    fields[0].row=0U;fields[0].column=0U;fields[0].attributes=TTC_BRIGHT;
    fields[0].length=n;fields[0].text=tc_header_cells;
    fields[1].row=tc_view->body_row;fields[1].column=0U;fields[1].attributes=TTC_PROTECTED;
    fields[1].length=tc_view->body_rows*tc_cap.columns;fields[1].text=tc_view->cells;
    fields[2].row=tc_view->entry_row-1U;fields[2].column=0U;fields[2].attributes=TTC_PROTECTED;
    fields[2].length=n;fields[2].text=tc_footer_cells;
    fields[3].row=tc_view->entry_row;fields[3].column=1U;fields[3].attributes=TTC_INPUT;
    fields[3].length=0U;fields[3].text=0;
    if(tc_display_known){
        if(TTCDIFF(&tc_cap,fields,3U,tc_raw,tc_stream,TTC_MAX_STREAM,&bytes))return 8U;
        if(!bytes)return 0U;
        rc=tc_output(&console_channel,tc_ssid,1U,tc_stream,bytes);
    }else{
        if(TTCENCODE(&tc_cap,fields,4U,tc_view->entry_row,1U,tc_stream,TTC_MAX_STREAM,&bytes))return 8U;
        rc=tc_output(&console_channel,tc_ssid,13U,tc_stream,bytes);
    }
    if(rc){tc_display_known=0U;return rc;}
    for(i=0U;i<3U;++i){
        origin=fields[i].row*tc_cap.columns+fields[i].column;
        for(j=0U;j<fields[i].length;++j)tc_raw[origin+j]=fields[i].text[j];
    }
    tc_display_known=1U;tc_input_field=tc_view->entry_row*tc_cap.columns+1U;
    tc_input_end=tc_cap.rows*tc_cap.columns-1U;
    return rc;
}
static unsigned int console_v1_init(void)
{
    const unsigned char *config=(const unsigned char *)TSF_KAPERTURE_VA+TTC_CONFIG_REAL;
    unsigned char *work;unsigned int rc,type,address,model,monitor,i;
    if(tc_ready)return 0U;
    if(!console_v1_enabled()||cms_word(config+4U)!=32U||cms_word(config+24U)||cms_word(config+28U))return 20U;
    if(console_read_phase)return 16U;
    type=cms_word(config+8U);address=cms_word(config+12U);
    model=cms_word(config+16U);monitor=cms_word(config+20U);
    if(TTCCONFIG(&tc_cap,type,address,model,monitor)!=TTC_OK)return 20U;
    if(TSCINIT(&tc_monitor_channel,(unsigned char *)TSF_KAPERTURE_VA,TSF_REAL_BYTES,TTC_MONITOR_REAL)!=TSC_OK)return 12U;
    if(TIOBIND(&channel_io,&tc_monitor_channel)!=TIO_OK)return 12U;
    tc_ssid=tc_find(address,&console_channel);
    if(!tc_ssid)return 20U;
    if(monitor){tc_monitor_ssid=tc_find(monitor,&tc_monitor_channel);if(!tc_monitor_ssid)return 20U;}
    if(TSRALLOC(&storage.real,TC_WORK_OWNER,0x20000U,TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,&tc_work_real)!=TSR_OK)return 4U;
    if(TSRALLOC(&storage.real,TC_LOG_OWNER,TC_LOG_BYTES,TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,&tc_log_real)!=TSR_OK){
        TSRRELEASE(&storage.real,TC_WORK_OWNER,tc_work_real);tc_work_real=0U;return 4U;
    }
    work=(unsigned char *)TSF_KAPERTURE_VA+tc_work_real;
    for(i=0U;i<0x20000U;++i)work[i]=0U;
    tc_view=(TTCVIEW *)work;tc_stream=work+0x5000U;tc_staging=work+0x9000U;tc_raw=work+0xd000U;
    tc_header_cells=work+0x11000U;tc_footer_cells=work+0x15000U;
    TTXINIT(&tc_log,(unsigned char *)TSF_KAPERTURE_VA+tc_log_real,TC_LOG_BYTES);
    if(type==TSA_DEVICE_3270&&TTCVINIT(tc_view,&tc_cap)!=TTC_OK){rc=20U;goto failed;}
    tc_monitor_state=monitor?TSA_MONITOR_OFFLINE:TSA_MONITOR_DISABLED;
    console_ssid=tc_ssid;*(volatile unsigned int *)0x40c0U=tc_ssid;
    /* Configuration names the geometry explicitly. Sense ID confirms the
     * selected device family, even though Hercules reports a default model. */
    rc=tc_sense(&console_channel,tc_ssid,&i);
    if(rc||i!=7U){rc=12U;goto failed;}
    if(type==TSA_DEVICE_3270){if(TSCDATA(&console_channel)[4U]!=0x32U||TSCDATA(&console_channel)[5U]!=0x78U){rc=20U;goto failed;}}
    else if(TSCDATA(&console_channel)[4U]!=0x32U||TSCDATA(&console_channel)[5U]!=0x15U){rc=20U;goto failed;}
    tc_base_cap=tc_cap;tc_ready=1U;
    for(i=0U;i<native_invocations.depth;++i){
        const TSVFRAME *frame=&native_invocations.frame[i];
        const unsigned char *name=frame->image_owner<TSL_IMAGES&&native_images[frame->image_owner].bytes?
                                  native_images[frame->image_owner].name:tc_default_header;
        unsigned int n=frame->image_owner<TSL_IMAGES&&native_images[frame->image_owner].bytes?8U:sizeof tc_default_header;
        TTXAPPEND(&tc_log,TSA_TRANSCRIPT_COMMAND_BEGIN,frame->token,TSA_ENCODING_1047,0U,name,n);
        for(rc=0U;rc<n;++rc)tc_headers[i+1U][rc]=name[rc];tc_header_lengths[i+1U]=n;
    }
    tc_snapshot();return tc_render();
failed:
    TSRRELEASE(&storage.real,TC_WORK_OWNER,tc_work_real);TSRRELEASE(&storage.real,TC_LOG_OWNER,tc_log_real);
    tc_work_real=tc_log_real=0U;return rc;
}
static unsigned int console_v1_line(const unsigned char *text,unsigned int length)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);unsigned int rc,token=f?f->token:0U;
    rc=console_v1_init();if(rc)return rc;
    if(length>256U||(length&&!text)||tc_read_owner)return 8U;
    if(tc_raw_owner||console_read_phase)return 16U;
    if(TTXAPPEND(&tc_log,TSA_TRANSCRIPT_TEXT,token,TSA_ENCODING_1047,0U,text,length))return 24U;
    if(tc_cap.device_class==TSA_DEVICE_LINE)
        rc=tc_line_output(&console_channel,tc_ssid,text,length);
    else{rc=TTCVLINE(tc_view,text,length)==TTC_OK?tc_render():8U;}
    if(tc_monitor_ssid){
        unsigned int monitor_rc;
        static const unsigned char resumed[]={0xe3,0xd9,0xc1,0xd5,0xe2,0xc3,0xd9,0xc9,0xd7,0xe3,0x40,0xc7,0xc1,0xd7};
        if(tc_monitor_gap_notice&&
           !tc_line_output(&tc_monitor_channel,tc_monitor_ssid,resumed,sizeof resumed))
            tc_monitor_gap_notice=0U;
        monitor_rc=tc_line_output(&tc_monitor_channel,tc_monitor_ssid,text,length);
        if(monitor_rc){if(tc_monitor_state!=TSA_MONITOR_OFFLINE||!tc_log.gaps)tc_gap(token,1U);tc_monitor_state=TSA_MONITOR_OFFLINE;}
        else tc_monitor_state=TSA_MONITOR_ONLINE;
    }
    tc_snapshot();return rc;
}
static void tc_other_status(unsigned int expected)
{
    int rc;unsigned char *irb;
    if(tc_monitor_ssid&&tc_monitor_ssid!=expected){
        rc=TSCPOLL(tc_monitor_ssid,TSCIRB(&tc_monitor_channel));
        if(!rc){
            irb=TSCIRB(&tc_monitor_channel);
            if(irb[9U]||(irb[8U]&3U)){
                tc_abort_output(&tc_monitor_channel,tc_monitor_ssid);
                tc_gap(tc_read_owner,1U);tc_monitor_state=TSA_MONITOR_OFFLINE;
            }else if(irb[8U]&0x80U)tc_attention(tc_monitor_ssid);
            else if(irb[8U]==4U){tc_monitor_stale=0U;tc_monitor_state=TSA_MONITOR_ONLINE;}
        }
    }
    if(tc_ssid!=expected&&!console_read_phase){
        rc=TSCPOLL(tc_ssid,TSCIRB(&console_channel));
        if(!rc){
            irb=TSCIRB(&console_channel);
            if(irb[9U]||(irb[8U]&3U)){
                tc_abort_output(&console_channel,tc_ssid);
                tc_display_known=0U;tc_gap(tc_read_owner,1U);
            }
            else if(irb[8U]&0x80U)tc_attention(tc_ssid);
        }
    }
}
static unsigned int tc_wait(TSCSTATE *device,unsigned int ssid)
{
    int rc;
    for(;;){
        rc=TSCPOLL(ssid,TSCIRB(device));
        if(!rc)return TSCIRB(device)[9U]||(TSCIRB(device)[8U]&3U)?12U:0U;
        if(rc<0)return 12U;
        tc_other_status(ssid);
        if(TSCWAITI(ssid,TSCIRB(device)))return 12U;
    }
}
static unsigned int console_v1_cancel(unsigned int token)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);TSCSTATE *device;
    if(!tc_read_owner)return 0U;
    if(!f||f->token!=token||token!=tc_read_owner)return 8U;
    device=tc_read_ssid==tc_ssid?&console_channel:&tc_monitor_channel;
    {const TIOREQUEST *r=TIOSTATE(&channel_io,device);
     if(r&&r->phase!=TIO_IDLE&&r->owner==token&&channel_cancel(device,token,r->ticket))return 12U;}
    if(tc_read_handle){
        if(TSVCOMPLETE(&native_invocations,token,tc_read_handle)!=TSV_OK||
           TSVFORGET(&native_invocations,token,TSV_IO,tc_read_handle)!=TSV_OK)return 12U;
    }
    if(TSVFORGET(&native_invocations,token,TSV_TERMINAL,tc_read_ssid)!=TSV_OK)return 12U;
    tc_read_owner=tc_read_ssid=tc_read_handle=tc_submitted=tc_next_input=0U;tc_snapshot();return 0U;
}
static unsigned int console_v1_read(unsigned char *text,unsigned int capacity,
                                     unsigned int *count,unsigned int *aid,unsigned int raw_mode)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);TSCSTATE *device;
    unsigned int ssid,rc,n,input_class,read_capacity;
    rc=console_v1_init();if(rc)return rc;
    if(!f||!text||!count||!aid||!capacity||capacity>(raw_mode?TSA_SCREEN_BYTES:256U)||tc_read_owner)return 8U;
    if(console_read_phase)return 16U;
    if(!raw_mode&&tc_raw_owner)return 16U;
    if(raw_mode&&tc_raw_owner&&tc_raw_owner!=f->token)return 16U;
    if(raw_mode&&(tc_cap.device_class!=TSA_DEVICE_3270||tc_next_input))return 20U;
    if(tc_next_input&&(!tc_monitor_ssid||tc_monitor_stale))return 16U;
    ssid=tc_next_input?tc_monitor_ssid:tc_ssid;device=tc_next_input?&tc_monitor_channel:&console_channel;
    if(device->quarantined)return 12U;
    input_class=tc_next_input?TSA_DEVICE_LINE:tc_cap.device_class;
    read_capacity=raw_mode?TSC_MAX_INPUT:4096U;
    if(TSVOWN(&native_invocations,f->token,TSV_TERMINAL,ssid)!=TSV_OK)return 16U;
    tc_read_owner=f->token;tc_read_ssid=ssid;
    if(ssid==tc_ssid){rc=tc_claim_attention(f->token);if(rc)goto done;}
    if(input_class==TSA_DEVICE_3270&&!raw_mode){
        unsigned char prompt[5];
        /* Unlock without clearing already keyed input or resetting its MDT. */
        prompt[0]=0xc2U;prompt[1]=0x11U;TTCADDR(&tc_cap,tc_input_field,prompt+2U);prompt[4]=0x13U;
        rc=tc_write(&console_channel,tc_ssid,1U,prompt,5U,1U);if(rc){tc_display_known=0U;goto done;}
    }
    if(input_class==TSA_DEVICE_3270){
        for(;;){
            if(!tc_attention_pending){
                do{rc=tc_wait(device,ssid);if(rc)goto done;}while(!(TSCIRB(device)[8U]&0x80U));
                tc_attention(ssid);
            }
            rc=tc_claim_attention(f->token);if(rc)goto done;
            if(tc_attention_pending)break;
        }
    }
again:
    if(TSCBUILDCONSCMD(device,input_class==TSA_DEVICE_LINE?10U:6U,read_capacity,1U)!=TSC_OK){rc=12U;goto done;}
    if(channel_take(device,ssid,f->token,&tc_read_handle)){rc=4U;goto done;}
    console_next_io_handle=tc_read_handle;
    if(TSVOWN(&native_invocations,f->token,TSV_IO,tc_read_handle)!=TSV_OK){channel_cancel(device,f->token,tc_read_handle);tc_read_handle=0U;rc=4U;goto done;}
    rc=tc_post_read(device,f->token,tc_read_handle);if(rc)goto done;
    if(ssid==tc_ssid)tc_attention_pending=tc_attention_owner=0U;
    tc_submitted=1U;
    for(;;){
        CHANNELCONTEXT context;int status;
        context.notice=tc_notice;context.argument=0;
        status=channel_poll(device,f->token,tc_read_handle,&context);
        if(status==TIO_OK){if(TSCCHECKIN(device,read_capacity,&n)!=TSC_OK){rc=12U;goto done;}break;}
        if(status!=TIO_PENDING){rc=12U;goto done;}
        tc_other_status(ssid);
        if((input_class==TSA_DEVICE_LINE?TSCWAITI(ssid,TSCIRB(device)):TSCWAITN(ssid,TSCIRB(device)))){rc=12U;goto done;}
    }
    if(TSVCOMPLETE(&native_invocations,f->token,tc_read_handle)!=TSV_OK||
       TSVFORGET(&native_invocations,f->token,TSV_IO,tc_read_handle)!=TSV_OK){rc=12U;goto done;}
    if(channel_release(device,f->token,tc_read_handle)){rc=12U;goto done;}
    tc_read_handle=tc_submitted=0U;
    if(input_class==TSA_DEVICE_LINE){
        if(n>capacity){rc=8U;goto done;}
        for(rc=0U;rc<n;++rc)text[rc]=TSCDATA(device)[rc];*count=n;*aid=0x7dU;rc=0U;
    }else if(n==3U&&TSCDATA(device)[0U]==0x60U){
        rc=tc_wait(device,ssid);if(rc)goto done;goto again;
    }else if(raw_mode){
        if(n>capacity){rc=8U;goto done;}
        for(rc=0U;rc<n;++rc)text[rc]=TSCDATA(device)[rc];*count=n;*aid=n?text[0U]:0U;rc=0U;
    }else{
        unsigned char control[8];
        rc=TTCINPUT(&tc_cap,TSCDATA(device),n,tc_input_field,text,capacity,count,aid)==TTC_OK?0U:8U;
        if(rc)goto done;
        if(*aid==0x7dU){
            /* The submitted bytes are already private. Clear only this field;
             * leave the keyboard locked until the next owning input prompt. */
            control[0]=0xc1U;control[1]=0x11U;TTCADDR(&tc_cap,tc_input_field,control+2U);
            control[4]=0x3cU;TTCADDR(&tc_cap,tc_input_end,control+5U);control[7]=0U;
            rc=tc_output(&console_channel,tc_ssid,1U,control,8U);
            if(rc)tc_display_known=0U;
        }else{
            /* Legacy line readers receive submissions, never PF text as a
             * command. A separate event service exposes the complete AID. */
            if(*aid==0x6dU){tc_display_known=0U;rc=tc_render();}
            else{control[0]=0xc2U;rc=tc_output(&console_channel,tc_ssid,1U,control,1U);}
            if(rc)goto done;
            do{rc=tc_wait(device,ssid);if(rc)goto done;}while(!(TSCIRB(device)[8U]&0x80U));
            goto again;
        }
    }
done:
    if(console_v1_cancel(f->token))return 12U;
    return rc;
}
static unsigned int tc_suspended_screen[TSV_MAX_DEPTH+1U];
static void console_v1_begin(const TSVFRAME *frame)
{
    const unsigned char *name;unsigned int n,i,depth;
    if(!tc_ready||!frame)return;
    depth=native_invocations.depth;
    tc_suspended_screen[depth]=tc_screen_owner;tc_screen_owner=0U;
    name=frame->image_owner<TSL_IMAGES&&native_images[frame->image_owner].bytes?
         native_images[frame->image_owner].name:tc_default_header;
    n=frame->image_owner<TSL_IMAGES&&native_images[frame->image_owner].bytes?8U:sizeof tc_default_header;
    for(i=0U;i<n;++i)tc_headers[depth][i]=name[i];tc_header_lengths[depth]=n;
    TTXAPPEND(&tc_log,TSA_TRANSCRIPT_COMMAND_BEGIN,frame->token,TSA_ENCODING_1047,0U,name,n);tc_snapshot();
}
static void console_v1_failed(unsigned int token,const unsigned char *name,unsigned int length)
{
    if(!tc_ready)return;
    TTXAPPEND(&tc_log,TSA_TRANSCRIPT_COMMAND_BEGIN,token,TSA_ENCODING_1047,0U,name,length);
    TTXAPPEND(&tc_log,TSA_TRANSCRIPT_COMMAND_END,token,0U,0U,call_last_result,TSA_RESULT_BYTES);
    tc_snapshot();
}
static void console_v1_operator(unsigned int token,const unsigned char *name,unsigned int length,unsigned int end)
{
    if(!tc_ready)return;
    TTXAPPEND(&tc_log,end?TSA_TRANSCRIPT_COMMAND_END:TSA_TRANSCRIPT_COMMAND_BEGIN,
              token,end?0U:TSA_ENCODING_1047,0U,end?call_last_result:name,end?TSA_RESULT_BYTES:length);
    tc_snapshot();
}
static void console_v1_flush(void)
{
    static const unsigned char key[]={0xd7,0xc4,0xd6,0xe2,0x4b,0xe3,0xd9,0xc1,0xd5,0xe2,0xc3,0xd9,0xc9,0xd7,0xe3};
    if(tc_ready&&(store_attach()!=TSS_OK||TSSPUT(&native_store,0x5443U,key,sizeof key,tc_log.data,tc_log.used)!=TSS_OK))
        tc_gap(0U,4U);
    tc_snapshot();
}
static void console_v1_end(unsigned int token,const unsigned char *result)
{
    if(!tc_ready)return;
    TTXAPPEND(&tc_log,TSA_TRANSCRIPT_COMMAND_END,token,0U,0U,result,TSA_RESULT_BYTES);
    if(tc_screen_owner==token)tc_screen_owner=0U;
    {const TSVFRAME *parent=TSVTOP(&native_invocations);
     unsigned int depth=native_invocations.depth+1U;
     if(depth<=TSV_MAX_DEPTH&&parent&&tc_suspended_screen[depth]==parent->token)
         tc_screen_owner=parent->token;
     if(depth<=TSV_MAX_DEPTH)tc_suspended_screen[depth]=0U;}
    if(tc_raw_owner==token){tc_raw_owner=0U;tc_cap=tc_base_cap;tc_display_known=0U;}
    if(tc_attention_pending&&tc_attention_owner==token){
        tc_attention_pending=tc_attention_owner=0U;tc_display_known=0U;
        tc_gap(token,3U);
    }
    if(tc_input_owner==token){tc_input_owner=0U;tc_display_known=0U;}
    if(!native_invocations.depth)console_v1_flush();
    tc_render();tc_snapshot();
}
static unsigned int console_v1_native_raw(TSGREQUEST *request,unsigned int input)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);TSPADDR pointer;TTCCAP cap;
    unsigned int rc,command,n,aid,i;unsigned char *data;
    rc=console_v1_init();if(rc)return rc;
    if(!f||tc_cap.device_class!=TSA_DEVICE_3270||request->address.hi||
       !request->length||request->length>TSA_SCREEN_BYTES)return 20U;
    pointer.hi=0U;pointer.lo=request->address.lo&0xffffffU;
    if(request->length>0x1000000U-pointer.lo)return 8U;
    rc=tc_span(pointer,request->length,input?TSG_WRITE:TSG_READ,tc_staging,!input);if(rc)return rc;
    if(input){
        rc=console_v1_read(tc_staging,request->length,&n,&aid,1U);if(rc)return rc;
        if(tc_span(pointer,n,TSG_WRITE,tc_staging,1U))return 8U;
        *(volatile unsigned int *)0x3008U=0U;*(volatile unsigned int *)0x300cU=n;return 0U;
    }
    cap=tc_cap;data=tc_staging;
    if(request->length>1U&&(data[1U]==0xf5U||data[1U]==0x7eU)){
        cap.rows=data[1U]==0xf5U?cap.default_rows:cap.alternate_rows;
        cap.columns=data[1U]==0xf5U?cap.default_columns:cap.alternate_columns;
        cap.encoding=cap.rows*cap.columns<=4096U?TSA_ADDRESS_CODED12:TSA_ADDRESS_BINARY14;
    }
    if(TTCRAW(&cap,data,request->length,&command)!=TTC_OK)return 8U;
    if(tc_screen_owner&&tc_screen_owner!=f->token)return 16U;
    n=request->length-2U;
    for(i=0U;i<n;++i)tc_stream[i]=data[i+2U];
    /* EAU and write NOPs have no payload. Supply an ignored byte to the
     * immediate EAU command; other payload-free write commands are NOPs. */
    if(!n&&command!=15U)return 0U;
    if(!n){tc_stream[0U]=0U;n=1U;}
    tc_display_known=0U;
    rc=tc_output(&console_channel,tc_ssid,command,tc_stream,n);
    if(rc){tc_gap(f->token,2U);return rc;}
    tc_cap=cap;tc_raw_owner=tc_screen_owner=f->token;
    tc_display_known=0U;
    TTXAPPEND(&tc_log,TSA_TRANSCRIPT_SCREEN_CHANGE,f->token,0U,TSA_TRANSCRIPT_RAW_SCREEN,data,request->length);
    tc_gap(f->token,2U);tc_snapshot();return 0U;
}
static unsigned int console_v1_service(TSGREQUEST *request)
{
    const TSVFRAME *frame=TSVTOP(&native_invocations);
    unsigned char header[64],field_bytes[24],answer[64],text[256];TTCFIELD fields[64];
    TSPADDR pointer;unsigned int op=request->direction,rc,i,count,n=0U,aid=0U,at=0U,length;
    if(!frame||*(volatile const unsigned int *)0x3000U||*(volatile const unsigned int *)0x3010U)return 8U;
    if(request->length!=(op==TSA_IO_CAPABILITIES?64U:32U)&&op!=TSA_IO_SCREEN_WRITE)return 8U;
    if(request->length>32U+24U*TSA_SCREEN_FIELDS||
       tc_span(request->address,request->length,TSG_READ,header,0U)||
       tc_span(request->address,request->length,TSG_WRITE,header,0U)||
       tc_span(request->address,op==TSA_IO_CAPABILITIES?64U:32U,TSG_READ,header,1U))return 8U;
    if(cms_word(header)!=1U||cms_word(header+4U)!=request->length)return 20U;
    rc=console_v1_init();if(rc)return rc;
    if(op==TSA_IO_CAPABILITIES){TTCCAPS(&tc_cap,answer);return tc_span(request->address,64U,TSG_WRITE,answer,1U);}
    if(op==TSA_IO_RESULT){
        if(frame->personality!=TSV_PDOS||!cms_word(call_last_result))return 8U;
        return tc_span(request->address,TSA_RESULT_BYTES,TSG_WRITE,call_last_result,1U);
    }
    if(op==TSA_IO_MONITOR_STATUS){
        for(i=0U;i<32U;++i)answer[i]=0U;cms_put_word(answer,1U);cms_put_word(answer+4U,32U);
        cms_put_word(answer+8U,tc_cap.monitor);cms_put_word(answer+12U,tc_monitor_state);
        cms_put_word(answer+16U,tc_log.gaps);cms_put_word(answer+20U,tc_next_input);
        cms_put_word(answer+24U,tc_log.sequence_hi);cms_put_word(answer+28U,tc_log.sequence_lo);
        return tc_span(request->address,32U,TSG_WRITE,answer,1U);
    }
    if(op==TSA_IO_INPUT_HANDOFF){
        if(cms_word(header+8U)>1U||cms_word(header+12U)||cms_word(header+24U)||cms_word(header+28U)||
           tc_read_owner||tc_raw_owner)return 16U;
        tc_other_status(0U);
        if(cms_word(header+8U)&&(!tc_monitor_ssid||tc_monitor_stale))return 16U;
        tc_next_input=cms_word(header+8U);cms_put_word(header+16U,frame->token);cms_put_word(header+20U,1U);
        return tc_span(request->address,32U,TSG_WRITE,header,1U);
    }
    if(op==TSA_IO_LINE_WRITE||op==TSA_IO_LINE_READ){
        count=cms_word(header+12U);pointer.hi=cms_word(header+16U);pointer.lo=cms_word(header+20U);
        if(cms_word(header+8U)||count>256U||
           tc_span(pointer,count,op==TSA_IO_LINE_WRITE?TSG_READ:TSG_WRITE,text,0U))return 8U;
        if(op==TSA_IO_LINE_WRITE){
            if(tc_span(pointer,count,TSG_READ,text,1U))return 8U;
            rc=console_v1_line(text,count);n=count;
        }else{
            rc=console_v1_read(text,count,&n,&aid,0U);
            if(!rc&&tc_span(pointer,n,TSG_WRITE,text,1U))return 8U;
        }
        if(rc)return rc;cms_put_word(header+24U,n);cms_put_word(header+28U,aid);
        return tc_span(request->address,32U,TSG_WRITE,header,1U);
    }
    if(op==TSA_IO_SCREEN_ACQUIRE||op==TSA_IO_SCREEN_RELEASE){
        if(tc_cap.device_class!=TSA_DEVICE_3270)return 20U;
        if(cms_word(header+8U)||cms_word(header+12U)||cms_word(header+28U))return 8U;
        if(op==TSA_IO_SCREEN_ACQUIRE){if(tc_screen_owner&&tc_screen_owner!=frame->token)return 16U;tc_screen_owner=frame->token;}
        else{
            if(tc_screen_owner!=frame->token)return 16U;
            if(tc_raw_owner){tc_cap=tc_base_cap;tc_display_known=0U;}
            tc_screen_owner=tc_raw_owner=0U;
        }
        return 0U;
    }
    if(op==TSA_IO_SCREEN_WRITE){
        if(tc_cap.device_class!=TSA_DEVICE_3270)return 20U;
        count=cms_word(header+12U);
        if(tc_screen_owner!=frame->token||tc_read_owner||cms_word(header+8U)||cms_word(header+28U)||
           cms_word(header+24U)!=tc_cap.generation||!count||count>64U||request->length!=32U+24U*count)return 8U;
        for(i=0U;i<count;++i){
            if(tc_span(tc_plus(request->address,32U+24U*i),24U,TSG_READ,field_bytes,1U))return 8U;
            fields[i].row=cms_word(field_bytes);fields[i].column=cms_word(field_bytes+4U);
            fields[i].attributes=cms_word(field_bytes+8U);fields[i].length=cms_word(field_bytes+12U);
            pointer.hi=cms_word(field_bytes+16U);pointer.lo=cms_word(field_bytes+20U);
            if(fields[i].length>TTC_MAX_STREAM-at||tc_span(pointer,fields[i].length,TSG_READ,tc_staging+at,1U))return 8U;
            fields[i].text=tc_staging+at;at+=fields[i].length;
        }
        if(TTCENCODE(&tc_cap,fields,count,cms_word(header+16U),cms_word(header+20U),
                       tc_stream,TTC_MAX_STREAM,&length)!=TTC_OK)return 8U;
        tc_display_known=0U;
        rc=tc_output(&console_channel,tc_ssid,13U,tc_stream,length);if(rc)return rc;
        tc_raw_owner=0U;
        for(i=0U;i<count;++i){
            if(fields[i].attributes==TTC_INPUT){
                unsigned int j,p,d,best=tc_cap.rows*tc_cap.columns;
                tc_input_field=fields[i].row*tc_cap.columns+fields[i].column;
                for(j=0U;j<count;++j){
                    p=fields[j].row*tc_cap.columns+fields[j].column;
                    p=p?p-1U:tc_cap.rows*tc_cap.columns-1U;
                    d=p>=tc_input_field?p-tc_input_field:p+tc_cap.rows*tc_cap.columns-tc_input_field;
                    if(d<best){best=d;tc_input_end=p;}
                }
            }
            if(fields[i].row==0U&&fields[i].column==0U){
                n=fields[i].length>132U?132U:fields[i].length;
                for(at=0U;at<n;++at)tc_headers[native_invocations.depth][at]=fields[i].text[at];
                tc_header_lengths[native_invocations.depth]=n;
            }
        }
        TTXAPPEND(&tc_log,TSA_TRANSCRIPT_SCREEN_CHANGE,frame->token,0U,0U,header,32U);tc_snapshot();return 0U;
    }
    if(op==TSA_IO_TRANSCRIPT){
        count=cms_word(header+12U);pointer.hi=cms_word(header+16U);pointer.lo=cms_word(header+20U);
        if(cms_word(header+8U)!=TSA_TRANSCRIPT_TEXT||count>256U||tc_span(pointer,count,TSG_READ,text,1U))return 8U;
        rc=TTXAPPEND(&tc_log,TSA_TRANSCRIPT_TEXT,frame->token,TSA_ENCODING_1047,0U,text,count);tc_snapshot();return rc?24U:0U;
    }
    return 20U;
}
