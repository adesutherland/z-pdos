/* SPDX-License-Identifier: MIT; K retained presentation, independent of PCOMM.
 * All physical transfers use the shared engine. Only checked completion updates
 * the confirmed surface. Input leases and semantic history are separate. */
#define TP_OWNER 0x54575001U
#define TP_BYTES 0x600000U
#define TP_HISTORY 0x400000U
#define TP_RECORDS 16384U
#define TP_WINDOWS 16U
#define TP_SHELL 0xffffffffU
#define TP_FOCUS_OUTPUT 0x100U
#define TP_FOCUS_SHELL 0x101U
typedef struct {
    unsigned int handle,owner,visible,staged,row,column,height,width,role,bytes,title_bytes;
    unsigned int live_visible,live_row,live_column,live_height,live_width,live_role,live_bytes,live_title_bytes;
    unsigned char title[132],live_title[132],*data,*live_data;
} TPPANEL;
static TPPANEL tp_windows[TP_WINDOWS];
static unsigned int tp_real,tp_controller,tp_next_handle,tp_source,tp_waiting,tp_focus,tp_help,tp_flushes,tp_erases;
static unsigned int tp_job,tp_phase,tp_os,tp_rc_valid,tp_rc,tp_external,tp_known,tp_gap,tp_shell_controller,tp_control,tp_joined;
typedef struct {unsigned int phase,os,valid,rc,name_bytes;unsigned char name[198];} TPCONTEXT;
static TPCONTEXT tp_saved[TSV_MAX_DEPTH+1U];
static unsigned int tp_saved_source[TSV_MAX_DEPTH+1U];
static void console_panel_gap(void){tp_gap=1U;}
static unsigned char tp_title[132],tp_name[198],tp_volume[65];
static unsigned int tp_title_bytes,tp_name_bytes,tp_volume_bytes;
static unsigned char tp_draft[TSV_MAX_DEPTH+1U][256],tp_prefetch[TSV_MAX_DEPTH+1U][256];
static unsigned int tp_prefetch_owner[TSV_MAX_DEPTH+1U],tp_prefetch_aid[TSV_MAX_DEPTH+1U],tp_prefetch_bytes[TSV_MAX_DEPTH+1U];
static unsigned int tp_draft_bytes[TSV_MAX_DEPTH+1U],tp_cursor[TSV_MAX_DEPTH+1U],tp_cursor_valid[TSV_MAX_DEPTH+1U],tp_draft_owner[TSV_MAX_DEPTH+1U];
static unsigned char *tp_stream,*tp_scratch;
static TPHSTATE tp_history;
static TPHVIEW tp_program,tp_shell;
static TWFSURFACE tp_surface,tp_confirmed;
static TWBPLAN tp_plan;
static TWFPALETTE tp_palette;
static void tp_copy(unsigned char *d,const unsigned char *s,unsigned int n)
{unsigned int i;for(i=0U;i<n;++i)d[i]=s[i];}
static unsigned int tp_depth(unsigned int owner)
{unsigned int i;for(i=0U;i<native_invocations.depth;++i)if(native_invocations.frame[i].token==owner)return i+1U;return 0U;}
static unsigned int tp_allocate(void)
{
    unsigned char *p;unsigned int i;T27GEOMETRY g;
    if(tp_real)return 0U;
    if(TSRALLOC(&storage.real,TP_OWNER,TP_BYTES,TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,&tp_real)!=TSR_OK)return 4U;
    p=(unsigned char *)TSF_KAPERTURE_VA+tp_real;
    if(TPHINIT(&tp_history,p,TP_HISTORY,(TPHRECORD *)(p+TP_HISTORY),TP_RECORDS))return 12U;
    p+=TP_HISTORY+TP_RECORDS*sizeof(TPHRECORD);
    if(T27GEOM(&g,tc_cap.device_class==TSA_DEVICE_LINE?24U:tc_cap.rows,tc_cap.device_class==TSA_DEVICE_LINE?80U:tc_cap.columns,tc_cap.device_class==TSA_DEVICE_LINE?T27_CODED12:tc_cap.encoding))return 20U;
    if(TWFINIT(&tp_surface,&g,p,p+TWF_MAX_CELLS,p+2U*TWF_MAX_CELLS)||
       TWFINIT(&tp_confirmed,&g,p+3U*TWF_MAX_CELLS,p+4U*TWF_MAX_CELLS,p+5U*TWF_MAX_CELLS))return 12U;
    p+=6U*TWF_MAX_CELLS;tp_stream=p;p+=65536U;tp_scratch=p;p+=TWF_MAX_CELLS;
    for(i=0U;i<TP_WINDOWS;++i){tp_windows[i].data=p;p+=TWF_MAX_CELLS;tp_windows[i].live_data=p;p+=TWF_MAX_CELLS;}
    if((unsigned int)(p-((unsigned char *)TSF_KAPERTURE_VA+tp_real))>TP_BYTES)return 12U;
    TPHFOLLOW(&tp_program,0U);TPHFOLLOW(&tp_shell,TP_SHELL);
    tp_volume_bytes=6U;if(TWDLabel("PDOS00",tp_volume,sizeof tp_volume,&tp_volume_bytes))return 12U;
    return 0U;
}
static unsigned int console_panel_enabled(void)
{return tp_controller!=0U;}
static unsigned int console_panel_source(void)
{return tp_controller?tp_source:tc_next_input;}
static void console_panel_primary(void)
{tp_source=tc_next_input=0U;}
static void console_panel_waiting(unsigned int waiting)
{tp_waiting=waiting;}
static unsigned int tp_write(unsigned int command,unsigned int n,unsigned int prompt)
{
    CHANNELCONTEXT context;TCNOTICE notice;unsigned int got;int rc;
    if(tc_submitted||console_read_phase||console_channel.quarantined)return 16U;
    if(prompt&&tc_attention_pending)return 16U;
    if(TSCBUILDEXT(&console_channel,command,(unsigned int)(tp_stream-(unsigned char *)TSF_KAPERTURE_VA),n,0U))return 12U;
    /* Catch attention that arrives after the render's initial check too. */
    notice.prompt=2U;notice.command=command;notice.text=tp_stream;context.notice=tc_notice;context.argument=&notice;
    rc=channel_run_notice(&console_channel,tc_ssid,&context);
    if(rc==TIO_DEFER)return 16U;if(rc)return rc==TIO_BUSY?16U:12U;
    if(TSCCHECKEXT(&console_channel,n,0U,&got))return 12U;
    tc_unlock_owner(command,tp_stream);return 0U;
}
/* Immediate Read Buffer captures a locked short-AID draft, or an unlocked
 * draft before an explicitly requested full presentation transition. Attention
 * already pending belongs to its original consumer and is never consumed here. */
static unsigned int tp_capture(unsigned int owner,unsigned int field)
{
    T27GEOMETRY g;T27INPUTEVENT event;CHANNELCONTEXT context;TDQUERY query;unsigned int n,d,rc;int status;
    d=tp_depth(owner);if(!d||tc_attention_pending||tc_submitted||console_read_phase||tc_raw_owner)return 16U;
    if(T27GEOM(&g,tc_cap.rows,tc_cap.columns,tc_cap.encoding))return 20U;
    /* The exact legacy DX profile ignores Read Buffer. Its Read Modified
     * replies with No-AID and the MDT editor. Host-restored drafts also carry
     * MDT, so this snapshot preserves them without a timed feature probe. */
    if(TSCBUILDEXT(&console_channel,device_legacy_dx?6U:2U,(unsigned int)(tp_stream-(unsigned char *)TSF_KAPERTURE_VA),65535U,1U))return 12U;
    query.attention=0U;context.notice=device_query_notice;context.argument=&query;
    status=channel_run_notice(&console_channel,tc_ssid,&context);if(status==TIO_DEFER)return 16U;if(status)return 12U;
    if(TSCCHECKEXT(&console_channel,65535U,1U,&n))return 12U;
    if(device_legacy_dx){unsigned int bytes,aid;
        if(T27EVENT(&g,tp_stream,n,&event)||TTCINPUT(&tc_cap,tp_stream,n,field,tp_draft[d],256U,&bytes,&aid)!=TTC_OK)return 12U;
        if(aid!=0x60U){tp_prefetch_owner[d]=owner;tp_prefetch_aid[d]=aid;tp_prefetch_bytes[d]=bytes;tp_copy(tp_prefetch[d],tp_draft[d],bytes);}
        n=aid==0x7dU?0U:bytes;if(aid==0x7dU){event.cursor=field;event.cursor_valid=1U;}rc=0U;
    }else{unsigned int bytes=n;rc=T27BUFFER(&g,tp_stream,bytes,field,tp_draft[d],256U,&n,&event);if(rc)n=bytes;}
    if(rc){unsigned int bytes,aid;
        /* A human AID can win the socket race after the final pre-submit
         * status drain. Retain that owned submission before any full redraw;
         * never deliver it to a child or infer readiness from elapsed time. */
        if(T27EVENT(&g,tp_stream,n,&event)||TTCINPUT(&tc_cap,tp_stream,n,field,tp_prefetch[d],256U,&bytes,&aid)!=TTC_OK)return 12U;
        tp_prefetch_owner[d]=owner;tp_prefetch_aid[d]=aid;tp_prefetch_bytes[d]=bytes;
        if(aid==0x7dU){n=0U;event.cursor=field;event.cursor_valid=1U;}
        else{n=bytes;tp_copy(tp_draft[d],tp_prefetch[d],bytes);}
    }
    tp_draft_owner[d]=owner;tp_draft_bytes[d]=n;tp_cursor[d]=event.cursor;tp_cursor_valid[d]=event.cursor_valid;
    rc=0U;return rc;
}
static unsigned int tp_layer(TPPANEL *w,unsigned int staged)
{
    unsigned int r,c,at,row=staged?w->row:w->live_row,col=staged?w->column:w->live_column;
    unsigned int height=staged?w->height:w->live_height,width=staged?w->width:w->live_width,stride=width;
    unsigned int role=staged?w->role:w->live_role,bytes=staged?w->bytes:w->live_bytes,tb=staged?w->title_bytes:w->live_title_bytes;
    unsigned char *data=staged?w->data:w->live_data,*title=staged?w->title:w->live_title;
    if(!(staged?w->visible:w->live_visible))return 0U;
    /* Clip to the output body. Every row has an explicit protected leading
     * attribute; no layer can intersect the command field or status/footer. */
    if(row<tp_plan.body||row>=tp_plan.position||col<1U||col>=tc_cap.columns||!height||!width)return 8U;
    if(height>tp_plan.position-row)height=tp_plan.position-row;if(width>tc_cap.columns-col)width=tc_cap.columns-col;
    for(r=0U;r<height;++r){at=(row+r)*tc_cap.columns+col-1U;tp_surface.kind[at]=TWF_ATTRIBUTE;tp_surface.role[at]=(unsigned char)(r?role:TWF_HEADER);
        for(c=0U;c<width;++c){at=(row+r)*tc_cap.columns+col+c;tp_surface.kind[at]=TWF_DATA;tp_surface.role[at]=(unsigned char)(r?role:TWF_HEADER);
            tp_surface.text[at]=r?( (r-1U)*stride+c<bytes?data[(r-1U)*stride+c]:0x40U):c<tb?title[c]:0x40U;}}
    return 0U;
}
static unsigned int tp_compose(unsigned int staged)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);TWBCONTEXT c;T27GEOMETRY g;unsigned int d=f?tp_depth(f->token):0U,i;
    if(T27GEOM(&g,tc_cap.rows,tc_cap.columns,tc_cap.encoding)||TWBLAYOUT(g.rows,g.columns,&tp_plan))return 20U;
    if(tp_surface.geometry.columns!=g.columns){tp_program.row=tp_shell.row=0U;}
    tp_surface.geometry=g;tp_confirmed.geometry=g;
    c.title=tp_title;c.title_bytes=tp_title_bytes;c.name=tp_name;c.name_bytes=tp_name_bytes;c.volume=tp_volume;c.volume_bytes=tp_volume_bytes;
    c.draft=tp_draft[d];c.draft_bytes=tp_draft_owner[d]==(f?f->token:0U)?tp_draft_bytes[d]:0U;
    c.cursor=tp_cursor[d];c.cursor_valid=tp_draft_owner[d]==(f?f->token:0U)&&tp_cursor_valid[d]&&tp_cursor[d]<g.cells;
    c.job=tp_job;c.phase=tp_phase;c.os=tp_os;c.rc_valid=tp_rc_valid;c.rc=tp_rc;c.focus=tp_focus;c.source=tp_source;
    c.input_program=f&&(!tp_shell_controller||f->token!=tp_controller);c.waiting=tp_waiting;c.help=tp_help;c.capture_gap=tp_gap;c.joined=tp_joined;
    c.input_name=f?tc_headers[native_invocations.depth]:0;c.input_name_bytes=f?tc_header_lengths[native_invocations.depth]:0U;
    if(tp_shell_controller&&!c.input_program){static const unsigned char shell_name[]={0xd7U,0xc3U,0xd6U,0xd4U,0xd4U};c.input_name=shell_name;c.input_name_bytes=sizeof shell_name;}
    if(TWBCOMPOSE(&tp_surface,&tp_plan,&c,&tp_history,&tp_program,&tp_shell,tp_scratch,TWF_MAX_CELLS))return 8U;
    if(!tp_help)for(i=0U;i<TP_WINDOWS;++i)if(tp_windows[i].handle&&tp_layer(&tp_windows[i],staged&&tp_windows[i].staged&&tp_windows[i].owner==(f?f->token:0U)))return 8U;
    return 0U;
}
static void tp_snapshot(void)
{
    volatile unsigned int *p=(volatile unsigned int *)(TSF_KAPERTURE_VA+0x3ae00U);
    p[0]=0x57424e31U;p[1]=tc_cap.rows;p[2]=tc_cap.columns;p[3]=tp_surface.input;p[4]=256U;p[5]=tp_palette.valid;
    p[6]=tp_history.count;p[7]=tp_history.evicted;p[8]=tp_job;p[9]=tp_phase;p[10]=tp_os;p[11]=tp_rc_valid;p[12]=tp_rc;
    p[13]=tp_source;p[14]=tp_focus;p[15]=tp_help;p[16]=tc_cap.generation;p[17]=tp_flushes;p[18]=tp_erases;
    p[19]=(unsigned int)(tp_confirmed.text-(unsigned char *)TSF_KAPERTURE_VA);p[20]=tp_controller;p[21]=tp_gap;
}
static unsigned int tp_render(unsigned int staged,unsigned int force)
{
    unsigned int rc,n,full,i;const TSVFRAME *f=TSVTOP(&native_invocations);
    if(!tp_controller||tc_cap.device_class!=TSA_DEVICE_3270)return 0U;
    if(tc_raw_owner||console_read_phase||tc_submitted||console_channel.quarantined)return 16U;
    if(tc_attention_pending)return staged?16U:0U;
    full=force||!tp_known||!tc_display_known||tp_confirmed.geometry.rows!=tc_cap.rows||tp_confirmed.geometry.columns!=tc_cap.columns;
    if(full&&tp_known&&tc_input_owner){
        if(!f||tc_input_owner!=f->token)return 16U;
        rc=tp_capture(f->token,tc_input_field);if(rc)return rc;
    }
    rc=tp_compose(staged);if(rc)return rc;
    tp_palette.valid=device_advertised.colours_valid;tp_palette.highlighting=device_advertised.highlighting;
    for(i=0U;i<16U;++i)tp_palette.colour[i]=device_advertised.colours[i];
    if(TWFENCODE(&tp_surface,&tp_confirmed,full,&tp_palette,tp_stream,65535U,&n))return 8U;
    if(n){if(full&&!tp_waiting&&tp_phase&&tp_phase!=TWB_DONE)tp_stream[0U]=0xc1U;rc=tp_write(full?13U:1U,n,full);
        if(rc==16U&&tc_attention_pending&&!staged)return 0U;
        if(rc){tc_display_known=0U;return rc;}++tp_flushes;if(full)++tp_erases;}
    if(TWFCOMMIT(&tp_confirmed,&tp_surface))return 12U;
    tp_known=tc_display_known=1U;tc_input_field=tp_surface.input;tc_input_end=tp_surface.input+256U;
    /* A completed full presentation creates this caller's editable field,
     * even when its WCC preserves the existing keyboard lock state. Deltas
     * leave the prior field lease untouched; a deferred write creates none. */
    if(full)tc_input_owner=f?f->token:0U;
    tp_snapshot();return 0U;
}
static unsigned int console_panel_render(void)
{return tp_render(0U,0U);}
static unsigned int console_panel_line(const unsigned char *text,unsigned int length)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);
    if(f&&f->token==tp_controller&&tp_control==1U)return 0U;
    if(TPHAPPEND(&tp_history,f&&f->token==tp_controller&&tp_control==2U?TP_SHELL:tp_job,f?f->token:0U,TPH_TEXT,text,length))return 24U;
    return console_panel_render();
}
static void console_panel_begin(const TSVFRAME *frame)
{
    unsigned int d;if(!tp_controller||!frame)return;d=tp_depth(frame->token);tp_draft_owner[d]=frame->token;tp_draft_bytes[d]=tp_cursor_valid[d]=0U;
    tp_saved_source[d]=tp_source;
    tp_saved[d].phase=tp_phase;tp_saved[d].os=tp_os;tp_saved[d].valid=tp_rc_valid;tp_saved[d].rc=tp_rc;tp_saved[d].name_bytes=tp_name_bytes;tp_copy(tp_saved[d].name,tp_name,tp_name_bytes);
    tp_phase=TWB_ACTIVE;tp_os=tp_rc_valid=tp_rc=0U;
    tp_copy(tp_name,tc_headers[native_invocations.depth],tc_header_lengths[native_invocations.depth]);tp_name_bytes=tc_header_lengths[native_invocations.depth];
}
static void console_panel_end(unsigned int token,const unsigned char *result)
{
    unsigned int d=tp_depth(token),i;
    if(!tp_controller)return;
    if(d){tp_draft_owner[d]=tp_draft_bytes[d]=tp_cursor_valid[d]=0U;}
    for(i=0U;i<TP_WINDOWS;++i)if(tp_windows[i].owner==token)tp_windows[i].handle=0U;
    if(native_invocations.depth+1U<=TSV_MAX_DEPTH)tp_source=tp_saved_source[native_invocations.depth+1U];
    if(token==tp_controller){tp_controller=0U;tp_known=0U;return;}
    tp_phase=TWB_DONE;tp_os=cms_word(result+TSA_RESULT_OS_STATUS);tp_rc_valid=cms_word(result+TSA_RESULT_RC_VALID);tp_rc=cms_word(result+TSA_RESULT_APP_RC);
    {const TSVFRAME *parent=TSVTOP(&native_invocations);unsigned int saved=native_invocations.depth+1U;
     if(parent&&saved<=TSV_MAX_DEPTH&&(!tp_shell_controller||parent->token!=tp_controller)){
        unsigned char completed[256];unsigned int bytes=tp_name_bytes,n;
        tp_copy(completed,tp_name,bytes);if(!TWDLabel(" / completed",completed+bytes,sizeof completed-bytes,&n))TPHAPPEND(&tp_history,TP_SHELL,token,TPH_TEXT,completed,bytes+n);
        tp_phase=tp_saved[saved].phase;tp_os=tp_saved[saved].os;tp_rc_valid=tp_saved[saved].valid;tp_rc=tp_saved[saved].rc;
        tp_name_bytes=tp_saved[saved].name_bytes;tp_copy(tp_name,tp_saved[saved].name,tp_name_bytes);
     }}
    /* Actual invocation result is retained until the U controller supplies the
     * command result. The latter also covers builtins that never invoke K. */
}
static void console_panel_operator(unsigned int token,unsigned int end)
{if(tp_controller&&token==tp_controller){if(!end){tp_phase=TWB_ACTIVE;return;}tp_phase=TWB_DONE;tp_os=cms_word(call_last_result+TSA_RESULT_OS_STATUS);tp_rc_valid=cms_word(call_last_result+TSA_RESULT_RC_VALID);tp_rc=cms_word(call_last_result+TSA_RESULT_APP_RC);}}
static int console_panel_close(unsigned int owner,unsigned int handle)
{unsigned int i;for(i=0U;i<TP_WINDOWS;++i)if(tp_windows[i].handle==handle&&tp_windows[i].owner==owner){tp_windows[i].handle=0U;return 0;}return 8;}
static unsigned int console_panel_take(unsigned int owner,unsigned char *text,unsigned int capacity,unsigned int *bytes,unsigned int *aid)
{unsigned int d=tp_depth(owner);if(!d||tp_prefetch_owner[d]!=owner)return 16U;if(tp_prefetch_bytes[d]>capacity)return 8U;
 tp_copy(text,tp_prefetch[d],tp_prefetch_bytes[d]);*bytes=tp_prefetch_bytes[d];*aid=tp_prefetch_aid[d];tp_prefetch_owner[d]=0U;return 0U;}
static unsigned int console_panel_hit(unsigned int cursor)
{unsigned int row=cursor/tc_cap.columns,col=cursor%tc_cap.columns;
 if(tp_plan.wide&&row>=2U&&row<tp_plan.position&&col>=tp_plan.shell_column&&col<tp_plan.shell_column+tp_plan.shell_width)return TP_FOCUS_SHELL;
 if(!tp_plan.wide&&row>=tp_plan.position+1U&&row<tp_plan.shell_row+tp_plan.shell_rows&&col>=tp_plan.shell_column&&col<tp_plan.shell_column+tp_plan.shell_width)return TP_FOCUS_SHELL;
 if(row>=2U&&row<=tp_plan.position&&col>=1U&&col<=tp_plan.body_width)return TP_FOCUS_OUTPUT;return 0U;}
static unsigned int console_panel_cursor(unsigned int owner)
{unsigned int d=tp_depth(owner);return d&&tp_draft_owner[d]==owner&&tp_cursor_valid[d]&&tp_cursor[d]<tc_cap.rows*tc_cap.columns?tp_cursor[d]:tc_input_field;}
static unsigned int console_panel_key(unsigned int owner,const unsigned char *text,unsigned int bytes,unsigned int aid,unsigned int cursor)
{
    unsigned int d=tp_depth(owner),rc=0U;TPHVIEW *v;unsigned int width,height;
    if(!tp_controller||!d||bytes>256U)return 20U;
    if(console_channel.quarantined)return 12U;
    if(aid==0x7bU||aid==0x7cU)return 20U;
    if(aid==0x6dU){tp_draft_owner[d]=owner;tp_draft_bytes[d]=tp_cursor_valid[d]=0U;tc_input_owner=0U;tp_known=tc_display_known=0U;return console_panel_render();}
    tp_draft_owner[d]=owner;
    if(aid==0x7dU){tp_draft_bytes[d]=tp_cursor_valid[d]=0U;tp_waiting=0U;return 0U;}
    if(text){tp_copy(tp_draft[d],text,bytes);tp_draft_bytes[d]=bytes;tp_cursor[d]=cursor;tp_cursor_valid[d]=cursor<tc_cap.rows*tc_cap.columns;}
    else{rc=tp_capture(owner,tc_input_field);if(rc)return rc;}
    v=tp_focus?&tp_program:&tp_shell;width=tp_focus?tp_plan.body_width:tp_plan.shell_width;height=tp_focus?tp_plan.body_rows:tp_plan.shell_rows;
    if(aid==0xf1U)tp_help=!tp_help;
    else if(aid==0xf2U){
        tp_help=0U;tp_stream[0U]=0xc2U;tp_stream[1U]=0x11U;TTCADDR(&tc_cap,tc_input_field+(tp_draft_bytes[d]<256U?tp_draft_bytes[d]:255U),tp_stream+2U);tp_stream[4U]=0x13U;
        rc=tp_write(1U,5U,0U);if(rc)return rc;tp_cursor[d]=tc_input_field+(tp_draft_bytes[d]<256U?tp_draft_bytes[d]:255U);tp_cursor_valid[d]=1U;
    }
    else if(aid==0xf7U)TPHPAGE(&tp_history,v,width,height,-1);
    else if(aid==0xf8U)TPHPAGE(&tp_history,v,width,height,1);
    else if(aid==0xf9U)TPHFOLLOW(v,v->group);
    else if(aid==0x7aU)tp_focus=!tp_focus;
    else if(aid==TP_FOCUS_OUTPUT)tp_focus=1U;
    else if(aid==TP_FOCUS_SHELL)tp_focus=0U;
    else if(aid==0xf5U||aid==0xf6U)return 20U;
    else if(aid==0x6dU){tc_input_owner=0U;tp_known=tc_display_known=0U;tp_draft_bytes[d]=tp_cursor_valid[d]=0U;}
    else return 20U;
    return console_panel_render();
}
static unsigned int console_panel_set(TSGREQUEST *request,const unsigned char *header)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);TSPADDR p;unsigned char draft[256];unsigned int d,n,rc,i,cursor;
    if(!tp_controller||!f||tc_read_owner||tc_raw_owner||tc_attention_pending||console_channel.quarantined||cms_word(header+28U)!=tc_cap.generation)return 16U;
    n=cms_word(header+12U);cursor=cms_word(header+8U);p.hi=cms_word(header+16U);p.lo=cms_word(header+20U);
    if(n>256U||cursor>n||tc_span(p,n,TSG_READ,draft,1U))return 8U;
    for(i=0U;i<n;++i)if(draft[i]<0x40U||draft[i]==0xffU)return 8U;
    /* An explicit editor action replaces input only after an AID boundary. */
    tp_stream[0U]=0xc2U;tp_stream[1U]=0x11U;TTCADDR(&tc_cap,tc_input_field-1U,tp_stream+2U);
    tp_stream[4U]=0x1dU;tp_stream[5U]=n?0xc1U:0x40U;
    tp_stream[6U]=0x3cU;TTCADDR(&tc_cap,tc_input_end,tp_stream+7U);tp_stream[9U]=0U;
    tp_stream[10U]=0x11U;TTCADDR(&tc_cap,tc_input_field,tp_stream+11U);tp_copy(tp_stream+13U,draft,n);
    tp_stream[13U+n]=0x11U;TTCADDR(&tc_cap,tc_input_field+(cursor<256U?cursor:255U),tp_stream+14U+n);tp_stream[16U+n]=0x13U;
    rc=tp_write(1U,17U+n,1U);if(rc)return rc;
    d=tp_depth(f->token);tp_draft_owner[d]=f->token;tp_draft_bytes[d]=n;tp_cursor[d]=tc_input_field+(cursor<256U?cursor:255U);tp_cursor_valid[d]=1U;tp_copy(tp_draft[d],draft,n);
    (void)request;return 0U;
}
static unsigned int console_panel_service(TSGREQUEST *request)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);unsigned char h[64],data[256];TSPADDR p;TPPANEL *w=0;
    unsigned int op=request->direction,rc,n,i,j,handle,generation,flags,len;
    if(!f||*(volatile const unsigned int *)0x3000U||*(volatile const unsigned int *)0x3010U)return 8U;
    len=op>=TSA_IO_PANEL_CREATE?64U:32U;
    if(request->length!=len||tc_span(request->address,len,TSG_READ,h,1U)||tc_span(request->address,len,TSG_WRITE,h,0U))return 8U;
    if(cms_word(h)!=1U||cms_word(h+4U)!=len)return 20U;
    if(op==TSA_IO_WORKBENCH){
        if(cms_word(h+8U)==1U){TTCCAP base=tc_base_cap;
            if(tc_read_owner||tc_attention_pending)return 16U;
            if(tp_controller!=f->token&&!tc_raw_owner)return 0U;
            if(tc_raw_owner&&tc_raw_owner!=f->token)return 16U;
            if(device_raw_handle){rc=console_device_close(f->token,device_raw_handle);if(rc)return rc;}
            else if(tc_raw_owner){rc=console_device_geometry(&base);if(rc)return rc;}
            tc_raw_owner=tc_screen_owner=tc_input_owner=0U;tp_known=0U;tc_display_known=0U;if(tp_controller==f->token)tp_controller=0U;return 0U;
        }
        n=cms_word(h+12U);p.hi=cms_word(h+16U);p.lo=cms_word(h+20U);
        if(cms_word(h+8U)||!n||n>132U||tc_span(p,n,TSG_READ,data,1U))return 8U;
        for(i=0U;i<n;++i)if(data[i]<0x40U||data[i]==0xffU)return 8U;
        rc=console_v1_init();if(rc)return rc;
        if(tp_controller&&tp_controller!=f->token)return 0U;
        rc=tp_allocate();if(rc)return rc;
        if(tc_attention_pending)return 16U;
        if(tc_cap.device_class==TSA_DEVICE_3270&&tc_input_owner){rc=tp_capture(f->token,tc_input_field);if(rc)return rc;}
        {TWBPLAN next;unsigned int d=tp_depth(f->token);
         if(tc_cap.device_class==TSA_DEVICE_3270&&!TWBLAYOUT(tc_cap.rows,tc_cap.columns,&next)&&tp_cursor_valid[d]&&tp_cursor[d]>=tc_input_field&&tp_cursor[d]-tc_input_field<256U)tp_cursor[d]=next.input+tp_cursor[d]-tc_input_field;}
        tp_copy(tp_title,data,n);tp_title_bytes=n;tp_controller=f->token;tp_control=0U;tc_screen_owner=0U;
        if(!tp_shell_controller){tp_phase=TWB_ACTIVE;tp_copy(tp_name,tc_headers[native_invocations.depth],tc_header_lengths[native_invocations.depth]);tp_name_bytes=tc_header_lengths[native_invocations.depth];}
        tp_known=tc_display_known=0U;return console_panel_render();
    }
    if(!tp_controller){if(tc_cap.device_class==TSA_DEVICE_LINE&&op<TSA_IO_PANEL_CREATE)return 0U;return 20U;}
    if(op<TSA_IO_PANEL_CREATE){
        if(f->token!=tp_controller&&op!=TSA_IO_UI_INPUT)return 16U;
        n=cms_word(h+12U);p.hi=cms_word(h+16U);p.lo=cms_word(h+20U);flags=cms_word(h+8U);
        if(op==TSA_IO_UI_CONTROL){if(flags>2U||n||p.hi||p.lo)return 8U;tp_control=flags;return 0U;}
        if(op==TSA_IO_UI_ACTION){if(flags>0xffU)return 8U;rc=console_panel_key(f->token,0,0U,flags,0xffffffffU);return rc;}
        if(op==TSA_IO_UI_INPUT){if(flags>1U||tc_read_owner||tc_attention_pending||tc_raw_owner||flags&&!tc_monitor_ssid)return 16U;
            tc_other_status(0U);if(tc_attention_pending)return 16U;if(flags&&!tc_monitor_interactive)return 20U;
            if(flags&&(tc_monitor_stale||tc_monitor_state!=TSA_MONITOR_ONLINE))return 16U;
            tp_source=tc_next_input=flags;return console_panel_render();}
        if(op==TSA_IO_TTY_MODE)return flags?20U:0U;
        if(op==TSA_IO_UI_STATUS){
            if(flags==2U){if(n>198U||p.hi||p.lo||cms_word(h+24U)>1U)return 8U;tp_joined=cms_word(h+24U)?n+1U:0U;return console_panel_render();}
            if(flags==1U){if(n>65U||tc_span(p,n,TSG_READ,data,1U))return 8U;for(i=0U;i<n;++i)if(data[i]<0x40U||data[i]==0xffU)return 8U;tp_copy(tp_volume,data,n);tp_volume_bytes=n;return console_panel_render();}
            if(flags)return 8U;cms_put_word(h+8U,tc_cap.generation);cms_put_word(h+12U,tp_job);cms_put_word(h+16U,tp_phase);cms_put_word(h+20U,tp_source);cms_put_word(h+24U,tp_flushes);cms_put_word(h+28U,tp_erases);return tc_span(request->address,32U,TSG_WRITE,h,1U);
        }
        if(op==TSA_IO_JOB_BEGIN){
            if(flags==3U){if(n||p.hi||p.lo)return 8U;tp_shell_controller=1U;tp_phase=0U;return console_panel_render();}
            if(flags==2U){if(n||p.hi||p.lo)return 8U;tp_external=1U;return 0U;}
            if(n>198U||!n||tc_span(p,n,TSG_READ,data,1U)||tp_job>=0xfffffffdU)return 8U;
            for(i=0U;i<n;++i)if(data[i]<0x40U||data[i]==0xffU)return 8U;
            if(flags>1U)return 8U;tp_shell_controller=1U;tp_external=flags;++tp_job;tp_phase=TWB_DISPATCH;tp_os=tp_rc_valid=tp_rc=0U;tp_help=tp_waiting=0U;
            tp_copy(tp_name,data,n);tp_name_bytes=n;TPHFOLLOW(&tp_program,tp_job);
            {unsigned char digits[10];unsigned int at=0U,v=tp_job,j=0U;
             tp_scratch[j++]=0x7bU;do{digits[at++]=(unsigned char)(0xf0U+v%10U);v/=10U;}while(v);while(at)tp_scratch[j++]=digits[--at];
             tp_scratch[j++]=0x40U;tp_scratch[j++]=0x6eU;tp_scratch[j++]=0x40U;tp_copy(tp_scratch+j,data,n);
             if(TPHAPPEND(&tp_history,TP_SHELL,f->token,TPH_TEXT,tp_scratch,j+n))return 24U;}return console_panel_render();
        }
        if(op==TSA_IO_JOB_END){
            if(flags>1U)return 8U;
            if(tp_external&&cms_word(call_last_result)){tp_os=cms_word(call_last_result+TSA_RESULT_OS_STATUS);tp_rc_valid=cms_word(call_last_result+TSA_RESULT_RC_VALID);tp_rc=cms_word(call_last_result+TSA_RESULT_APP_RC);}
            else{tp_os=cms_word(h+24U);tp_rc_valid=flags;tp_rc=n;}
            tp_phase=TWB_DONE;tp_waiting=0U;
            n=0U;if(TWDLabel("OS=",data,sizeof data,&n))return 12U;
            {unsigned char digits[10];unsigned int at=0U,v=tp_os;do{digits[at++]=(unsigned char)(0xf0U+v%10U);v/=10U;}while(v);while(at)data[n++]=digits[--at];}
            if(TWDLabel(tp_rc_valid?" RC=":" RC=unavailable",data+n,sizeof data-n,&i))return 12U;n+=i;
            if(tp_rc_valid){unsigned char digits[10];unsigned int at=0U,v=tp_rc;if(v&0x80000000U){data[n++]=0x60U;v=(~v)+1U;}do{digits[at++]=(unsigned char)(0xf0U+v%10U);v/=10U;}while(v);while(at)data[n++]=digits[--at];}
            if(TPHAPPEND(&tp_history,TP_SHELL,f->token,TPH_TEXT,data,n))return 24U;return console_panel_render();
        }
        return 20U;
    }
    handle=cms_word(h+8U);generation=cms_word(h+12U);flags=cms_word(h+16U);
    if(generation!=tc_cap.generation||flags)return 16U;
    if(op==TSA_IO_PANEL_COMMIT){
        if(handle||tc_read_owner||tc_raw_owner)return 16U;rc=tp_render(1U,0U);if(rc)return rc;
        for(i=0U;i<TP_WINDOWS;++i){w=&tp_windows[i];if(!w->handle||!w->staged||w->owner!=f->token)continue;
            w->live_visible=w->visible;w->live_row=w->row;w->live_column=w->column;w->live_height=w->height;w->live_width=w->width;w->live_role=w->role;
            w->live_bytes=w->bytes;w->live_title_bytes=w->title_bytes;tp_copy(w->live_title,w->title,w->title_bytes);tp_copy(w->live_data,w->data,w->bytes);w->staged=0U;}
        return 0U;
    }
    if(op==TSA_IO_PANEL_CREATE){
        if(handle||tp_next_handle==0xffffffffU)return 8U;
        for(i=0U;i<TP_WINDOWS;++i)if(!tp_windows[i].handle){w=&tp_windows[i];break;}if(!w)return 4U;
    }else{
        for(i=0U;i<TP_WINDOWS;++i)if(tp_windows[i].handle==handle&&tp_windows[i].owner==f->token){w=&tp_windows[i];break;}
        if(!w||TSVHAS(&native_invocations,f->token,TSV_PANEL,handle)!=TSV_OK)return 8U;
    }
    if(op==TSA_IO_PANEL_CLOSE){if(console_panel_close(f->token,handle))return 8U;return TSVFORGET(&native_invocations,f->token,TSV_PANEL,handle)==TSV_OK?0U:12U;}
    if(op==TSA_IO_PANEL_CREATE||op==TSA_IO_PANEL_UPDATE){
        unsigned int row=cms_word(h+20U),col=cms_word(h+24U),height=cms_word(h+28U),width=cms_word(h+32U),role=cms_word(h+36U);
        n=cms_word(h+40U);p.hi=cms_word(h+44U);p.lo=cms_word(h+48U);
        if(row<tp_plan.body||row>=tp_plan.position||col<1U||col>=tc_cap.columns||!height||!width||height>256U||width>TWF_MAX_CELLS||height>TWF_MAX_CELLS/width||role>TWF_FOCUS||n>132U||tc_span(p,n,TSG_READ,data,1U))return 8U;
        for(j=0U;j<n;++j)if(data[j]<0x40U||data[j]==0xffU)return 8U;
        if(op==TSA_IO_PANEL_CREATE){handle=++tp_next_handle;if(TSVOWN(&native_invocations,f->token,TSV_PANEL,handle)!=TSV_OK)return 4U;w->handle=handle;w->owner=f->token;w->bytes=w->live_bytes=0U;w->visible=1U;w->live_visible=0U;}
        w->row=row;w->column=col;w->height=height;w->width=width;w->role=role;w->title_bytes=n;tp_copy(w->title,data,n);w->staged=1U;
    }else if(op==TSA_IO_PANEL_TEXT){
        n=cms_word(h+52U);p.hi=cms_word(h+56U);p.lo=cms_word(h+60U);
        if(n>TWF_MAX_CELLS||tc_span(p,n,TSG_READ,tp_scratch,1U))return 8U;
        for(j=0U;j<n;++j)if(tp_scratch[j]<0x40U||tp_scratch[j]==0xffU)return 8U;
        tp_copy(w->data,tp_scratch,n);w->bytes=n;w->staged=1U;
    }else if(op==TSA_IO_PANEL_SHOW||op==TSA_IO_PANEL_HIDE){w->visible=op==TSA_IO_PANEL_SHOW;w->staged=1U;}
    else return 20U;
    cms_put_word(h+8U,handle);return tc_span(request->address,64U,TSG_WRITE,h,1U);
}
