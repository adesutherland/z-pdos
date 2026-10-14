/* SPDX-License-Identifier: MIT; serial K job provider and native DD scope. */
static TJBPOOL job_pool;
static unsigned int job_real,job_ready,job_failed,job_id,job_owner,job_token;
static unsigned int job_base_depth,job_output_error,job_cancelling;
static TSPADDR job_reply;
static TSVCONTEXT job_caller;
static const unsigned char job_key[11]={0x50U,0x44U,0x30U,0x33U,0x33U,0x2eU,0x53U,0x50U,0x4fU,0x4fU,0x4cU};
static const unsigned char job_dd[8]={0xd1U,0xc5U,0xe2U,0xd4U,0xe2U,0xc7U,0xd3U,0xc7U};
static unsigned int job_active(void){return job_id!=0U;}
static unsigned int job_dd_scope(void)
{return job_id&&native_invocations.depth>job_base_depth;}
static void job_native_error(unsigned int rc)
{if(job_id&&!job_output_error)job_output_error=rc;}
static int job_record(void *context,const unsigned char *data,unsigned int n)
{
    const unsigned char *e=(const unsigned char *)context;unsigned char blank=0x40U;
    return TJBAPP(&job_pool,job_owner,job_id,e+32U,9U,n?data:&blank,n?n:1U);
}
static unsigned int job_native_output(const unsigned char *entry,const unsigned char *block,unsigned int n)
{
    unsigned char *j=TJBFIND(&job_pool,job_owner,job_id);unsigned int cost,rc;
    if(!job_dd_scope()||!j)return 12U;
    rc=(unsigned int)TJDOUT(entry,block,n,&cost,0,0);if(rc)return rc;
    if(cost>TJBWORD(j+20U)-TJBWORD(j+24U)){job_native_error(4U);return 4U;}
    rc=(unsigned int)TJDOUT(entry,block,n,&cost,job_record,(void *)entry);
    if(!rc)rc=job_commit();if(rc)job_native_error(rc);return rc;
}
static unsigned int job_allocation_ok(unsigned int bytes)
{
    unsigned int d,r,a,used=0U,rounded;
    if(!job_id)return 1U;
    if(bytes>0x10000000U-4095U)return 0U;rounded=(bytes+4095U)&~4095U;
    for(d=job_base_depth;d<native_invocations.depth;++d){const TSVFRAME *f=&native_invocations.frame[d];
        for(r=0U;r<f->resources;++r)if(f->resource[r].kind==TSV_ALLOCATION)
            for(a=0U;a<TSM_ALLOCS;++a)if(storage.allocations[a].handle==f->resource[r].handle){
                if(storage.allocations[a].bytes>0x10000000U-used)return 0U;used+=storage.allocations[a].bytes;break;}}
    return rounded<=0x10000000U-used;
}
static unsigned int job_memory_room(void)
{
    unsigned int i,used=0U;
    for(i=0U;i<storage.real.count;++i)if(storage.real.ranges[i].phases&TSR_RUN){
        if(storage.real.ranges[i].size>TSF_REAL_BYTES-used)return 0U;used+=storage.real.ranges[i].size;}
    return TSF_REAL_BYTES-used>=0x18000000U;
}
static void job_cleanup_failed(void)
{
    unsigned char *j=TJBFIND(&job_pool,job_owner,job_id);
    if(j){unsigned int step=TJBWORD(j+12U),count=TJBWORD(j+16U);
        TJBPUT(j+44U,12U);if(count&&step>=count)step=count-1U;
        if(count)TJBPUT(TJBSTEP(j,step)+140U,12U);}
    job_failed=1U;*(volatile unsigned int *)0x4fbcU=1U;
}
static unsigned int job_is_root(unsigned int token){return job_id&&token==job_token;}
static int job_equal(const unsigned char *p,const char *s,unsigned int n)
{unsigned int i;for(i=0U;i<n;++i)if(p[i]!=(unsigned char)s[i])return 0;return 1;}
static unsigned int job_commit(void)
{
    int rc=TSSPUT(&native_store,0x50444a32U,job_key,sizeof job_key,job_pool.bytes,TJB_BYTES);
    if(rc!=TSS_OK){job_failed=1U;return rc==TSS_FULL?4U:12U;}
    {volatile unsigned int *o=(volatile unsigned int *)(TSF_KAPERTURE_VA+0x3ba00U);
     o[0U]=0x50444a32U;o[1U]=job_id;o[6U]=native_invocations.depth;o[7U]=job_real;o[8U]=job_failed;}
    return 0U;
}
static unsigned int job_attach(void)
{
    unsigned int n;int rc;
    if(job_failed)return 12U;if(job_ready)return 0U;
    if(store_attach()!=TSS_OK)return 12U;
    rc=TSSADMIT(&native_store,0x50444a32U,job_key,sizeof job_key,TJB_BYTES);
    if(rc)return rc==TSS_FULL?4U:12U;
    if(!job_real&&TSRALLOC(&storage.real,0x544a4201U,TJB_BYTES,TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,&job_real)!=TSR_OK)return 4U;
    job_pool.bytes=(unsigned char *)TSF_KAPERTURE_VA+job_real;
    rc=TSSREAD(&native_store,0x50444a32U,job_key,sizeof job_key,job_pool.bytes,TJB_BYTES,&n);
    if(rc==TSS_ABSENT){if(TJBINIT(&job_pool,job_pool.bytes,TJB_BYTES))return 12U;}
    else if(rc||TJBLOAD(&job_pool,job_pool.bytes,n)){job_failed=1U;return 12U;}
    /* One operator domain. Persisted ownership is rebound only after the
       checked restart has held queued jobs and interrupted uncertain work. */
    {unsigned int i;for(i=0U;i<TJB_SLOTS;++i){unsigned char *j=job_pool.bytes+TJB_BASE+i*TJB_SLOT_BYTES;
     if(TJBWORD(j))TJBPUT(j+4U,native_invocations.frame[0U].token);}}
    if(job_commit())return 12U;job_ready=1U;return 0U;
}
static void job_status(unsigned char *h,const unsigned char *j)
{unsigned int i;TJBPUT(h+8U,TJBWORD(j));for(i=0U;i<88U;++i)h[40U+i]=j[i];}
static unsigned int job_note(const char *label,unsigned int value)
{
    unsigned char text[132],digits[10];unsigned int n=0U,k=0U,i;
    while(label[n]&&n<110U){text[n]=(unsigned char)label[n];++n;}
    do{digits[k++]=(unsigned char)(0xf0U+value%10U);value/=10U;}while(value&&k<10U);
    for(i=0U;i<k;++i)text[n++]=digits[k-i-1U];
    return (unsigned int)TJBAPP(&job_pool,job_owner,job_id,job_dd,9U,text,n);
}
static unsigned int job_done(void)
{
    unsigned char h[TJB_REQUEST_BYTES],*j=TJBFIND(&job_pool,job_owner,job_id);
    volatile unsigned int *saved=(volatile unsigned int *)0x3000U;unsigned int rc=0U,i,id=job_id;
    if(!j)return 12U;
    if(sf_temp_end()){job_cleanup_failed();return 12U;}
    if(job_note("JOB END STATE ",TJBWORD(j+8U))){
        if(TJBWORD(j+8U)==TJB_DONE){TJBPUT(j+8U,TJB_FAILED);TJBPUT(j+32U,4U);TJBPUT(j+36U,0U);TJBPUT(j+40U,0xffffffffU);}
    }
    if(!job_failed)rc=job_commit();else rc=12U;
    if(rc){TJBPUT(j+8U,TJB_INTERRUPTED);TJBPUT(j+32U,12U);TJBPUT(j+36U,0U);TJBPUT(j+40U,0xffffffffU);}
    for(i=0U;i<sizeof h;++i)h[i]=0U;TJBPUT(h,1U);TJBPUT(h+4U,sizeof h);job_status(h,j);
    command_restore(&job_caller);
    if(tc_span(job_reply,sizeof h,TSG_WRITE,h,1U))rc=12U;
    saved[0U]=0U;saved[1U]=rc;saved[30U]=0U;saved[31U]=rc;
    job_id=job_token=job_owner=job_base_depth=job_output_error=0U;
    *(volatile unsigned int *)0x4fb4U=0U;*(volatile unsigned int *)0x4fb8U=0U;
    *(volatile unsigned int *)0x4f54U=1U;
    {volatile unsigned int *o=(volatile unsigned int *)(TSF_KAPERTURE_VA+0x3ba00U);
     o[0U]=0x50444a32U;o[1U]=id;o[2U]=TJBWORD(j+8U);o[3U]=TJBWORD(j+32U);o[4U]=TJBWORD(j+36U);o[5U]=TJBWORD(j+40U);o[6U]=native_invocations.depth;o[7U]=job_real;o[8U]=job_failed;}
    return 0U;
}
static unsigned int job_dispatch(void)
{
    unsigned char parameters[104],*j,*s;TSLIMAGE *image;unsigned int i,n,rc;int next;
    for(;;){
        j=TJBFIND(&job_pool,job_owner,job_id);if(!j)return 12U;
        next=TJBNEXT(&job_pool,job_owner,job_id);
        if(next==1)return job_done();if(next)return 12U;
        if(job_failed){TJBEND(&job_pool,job_owner,job_id,12U,0U,0U,0U,0U);continue;}
        s=TJBSTEP(j,TJBWORD(j+12U));
        if(job_note("STEP BEGIN ",TJBWORD(j+12U)+1U)){job_output_error=4U;}
        if(job_commit()){TJBEND(&job_pool,job_owner,job_id,12U,0U,0U,0U,0U);continue;}
        command_restore(&job_caller);
        rc=load_native(TSV_TSO,31U,s+8U,&image);
        if(!rc)rc=sf_batch_begin(TJBDDS(&job_pool,job_id),TJBWORD(j+12U)+1U);
        if(!rc){
            n=TJBWORD(s+16U);for(i=0U;i<sizeof parameters;++i)parameters[i]=0U;
            parameters[0U]=(unsigned char)(n>>8);parameters[1U]=(unsigned char)n;
            for(i=0U;i<n;++i)parameters[2U+i]=s[24U+i];
            rc=call_launch(image,parameters,n+2U,2U);
            if(!rc){job_token=TSVTOP(&native_invocations)->token;
                *(volatile unsigned int *)0x3080U|=0x01000000U;
                *(volatile unsigned int *)0x4fb4U=job_token;return 0U;}
        }
        if(native_invocations.depth>job_base_depth){job_cleanup_failed();return 12U;}
        sf_batch_end();
        if(job_note("LOAD FAILURE ",rc))job_output_error=4U;
        if(TJBEND(&job_pool,job_owner,job_id,rc,0U,0U,0U,0U))return 12U;
        if(job_commit())job_failed=1U;
    }
}
static unsigned int job_step_done(unsigned int token,unsigned int os,unsigned int valid,unsigned int rc,unsigned int abnormal)
{
    if(!job_id||token!=job_token)return 12U;
    if(sf_temp_step()){job_cleanup_failed();return 12U;}
    sf_batch_end();
    job_token=0U;*(volatile unsigned int *)0x4fb4U=0U;
    if(job_cancelling){os=16U;valid=0U;abnormal=3U;}
    else if(job_output_error){os=job_output_error;valid=0U;}
    if(job_note("STEP OS ",os)||job_note("STEP RC ",valid?rc:0xffffffffU)){
        job_output_error=4U;os=4U;valid=0U;}
    if(TJBEND(&job_pool,job_owner,job_id,os,valid,rc,abnormal,0U))return 12U;
    job_output_error=0U;
    if(job_commit())job_failed=1U;
    return job_dispatch();
}
/* Native controlled unwind is the sole resource reaper. Cancellation of a
   nested LINK chain unwinds children before the batch root. No fabricated RC. */
static unsigned int job_abort(void)
{
    unsigned int rc;job_cancelling=1U;
    while(job_id&&native_invocations.depth>job_base_depth){
        const TSVFRAME *f=TSVTOP(&native_invocations);
        if(!f||f->controlled!=3U)return 12U;
        *(volatile unsigned int *)0x309cU=2U;
        rc=native_call_finish();if(rc)return rc;
    }
    job_cancelling=0U;return 0U;
}
/* The interrupt key pauses U. K offers only this bounded operator dialogue.
   For a key received during a K I/O wait, the nucleus merely sets pending;
   channel_wait_native enters here at its safe point, without re-entering C. */
static unsigned int job_control(void)
{
    unsigned char text[256];unsigned int count,aid,rc;unsigned char *j;
    *(volatile unsigned int *)0x4fb8U=0U;
    if(!job_id)return 0U;
    for(;;){
        static const unsigned char prompt[]="JOB CONTROL: STATUS / CANCEL / CONTINUE";
        if(console_v1_line(prompt,sizeof prompt-1U))return 12U;
        count=aid=0U;rc=console_v1_read(text,sizeof text,&count,&aid,0U);if(rc)return rc;
        if(count==6U&&job_equal(text,"STATUS",6U)){
            static const unsigned char status[]="JOB ACTIVE; SERIAL EXECUTION; OTHER COMMANDS BUSY";
            if(console_v1_line(status,sizeof status-1U))return 12U;continue;
        }
        if(count==8U&&job_equal(text,"CONTINUE",8U))return 0U;
        if(count==6U&&job_equal(text,"CANCEL",6U)){
            rc=(unsigned int)TJBACT(&job_pool,job_owner,job_id,3U);if(rc)return rc;
            j=TJBFIND(&job_pool,job_owner,job_id);if(!j)return 12U;
            return job_commit();
        }
        {static const unsigned char busy[]="BUSY: USE STATUS, CANCEL OR CONTINUE";
         if(console_v1_line(busy,sizeof busy-1U))return 12U;}
    }
}
static unsigned int job_wait_control(void)
{
    unsigned char *j;unsigned int rc=job_control();
    j=TJBFIND(&job_pool,job_owner,job_id);
    return rc||!j||TJBWORD(j+48U)?12U:0U;
}
static unsigned int job_interrupt(void)
{
    unsigned int rc;unsigned char *j;
    if(!job_id||*(volatile const unsigned int *)0x309cU!=3U||
       *(volatile const unsigned short *)0x86U!=0x40U)return 8U;
    rc=job_control();j=TJBFIND(&job_pool,job_owner,job_id);
    if(rc){job_output_error=rc;return job_abort();}
    if(j&&TJBWORD(j+48U))return job_abort();
    /* Preserve every interrupted U register; C's status is not an app RC. */
    *(volatile unsigned int *)0x4f54U=1U;return 0U;
}
static unsigned int job_print(unsigned char *j,TBCENDPOINT *e)
{
    unsigned int at=0U,n,i,rc=0U;unsigned char *out=TJBOUT(&job_pool,TJBWORD(j));TSCSTATE *c;
    if(TJBWORD(j+8U)<TJB_DONE||TJBWORD(j+28U)==TJB_PRINT_ACTIVE)return 16U;
    if(TJBWORD(j+56U)==0xffffffffU)return 4U;
    TJBPUT(j+28U,TJB_PRINT_ACTIVE);TJBPUT(j+56U,TJBWORD(j+56U)+1U);
    if(job_commit())return 12U;c=&batch_channels[1U];
    while(at<TJBWORD(j+24U)){
        if(c->occupied||c->quarantined){rc=16U;break;}n=TJBWORD(out+at+8U);
        for(i=0U;i<n;++i)TSCDATA(c)[i]=out[at+24U+i];
        rc=batch_transfer(e,TJBWORD(out+at+12U),n);if(rc)break;at+=24U+n;
    }
    TJBPUT(j+28U,rc?TJB_PRINT_FAILED:TJB_PRINT_DONE);
    if(job_commit()){TJBPUT(j+28U,TJB_PRINT_FAILED);return 12U;}return rc;
}
static unsigned int job_service(TSGREQUEST *request)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);unsigned char h[TJB_REQUEST_BYTES],descriptor[TJD_BYTES],data[132],*j,*s;
    unsigned int op=request->direction,owner,id,n,offset,rc,i,selector;TSPADDR payload;TBCENDPOINT *e;
    if(!f||request->length!=sizeof h||*(volatile const unsigned int *)0x3000U||*(volatile const unsigned int *)0x3010U||
       tc_span(request->address,sizeof h,TSG_READ,h,0U)||tc_span(request->address,sizeof h,TSG_WRITE,h,0U)||
       tc_span(request->address,sizeof h,TSG_READ,h,1U))return 8U;
    if(TJBWORD(h)!=1U||TJBWORD(h+4U)!=sizeof h)return 20U;
    id=TJBWORD(h+8U);selector=TJBWORD(h+12U);n=TJBWORD(h+16U);offset=TJBWORD(h+36U);
    payload.hi=TJBWORD(h+24U);payload.lo=TJBWORD(h+28U);
    if(op==TSA_IO_JOB_EMIT){
        if(!job_id||f->token!=job_token||(id&&id!=job_id)||n>132U||!n||selector>1U||offset||batch_overlap(request->address,sizeof h,payload,n)||tc_span(payload,n,TSG_READ,data,1U))return 8U;
        rc=(unsigned int)TJBAPP(&job_pool,job_owner,job_id,h+64U,TJBWORD(h+20U),data,n);
        if(rc){if(rc==4U)job_output_error=4U;return rc;}
        if(job_commit()){job_output_error=12U;return 12U;}
        if(selector){static const unsigned char notice[]="JOB STEP WAITING FOR DEVICE";
            if(console_v1_line(notice,sizeof notice-1U))return 12U;}
        TJBPUT(h+8U,job_id);TJBPUT(h+32U,n);return tc_span(request->address,sizeof h,TSG_WRITE,h,1U);
    }
    if(job_id)return 16U;owner=native_invocations.frame[0U].token;
    if(op==TSA_IO_JOB_SUBMIT){
        if(id||selector||offset||TJBWORD(h+20U)||n!=TJB_DESC_BYTES||batch_overlap(request->address,sizeof h,payload,n)||tc_span(payload,n,TSG_READ,descriptor,1U))return 8U;
        rc=(unsigned int)TJBCHECK(descriptor,n);if(rc)return rc;
        rc=job_attach();if(rc)return rc;
        rc=(unsigned int)TJBSUB(&job_pool,owner,descriptor,n,&id);if(rc)return rc;
        if(job_commit())return 12U;j=TJBFIND(&job_pool,owner,id);job_status(h,j);return tc_span(request->address,sizeof h,TSG_WRITE,h,1U);
    }
    rc=job_attach();if(rc)return rc;
    if(op==TSA_IO_JOB_STATUS&&!id){
        if(selector||n||offset||payload.hi||payload.lo||TJBWORD(h+20U))return 8U;
        for(i=40U;i<sizeof h;++i)h[i]=0U;
        TJBPUT(h+40U,TJBWORD(job_pool.bytes+8U));TJBPUT(h+44U,TJBWORD(job_pool.bytes+12U));
        for(i=0U;i<TJB_SLOTS;++i){unsigned char *a=job_pool.bytes+TJB_BASE+i*TJB_SLOT_BYTES;
            TJBPUT(h+48U+12U*i,TJBWORD(a));TJBPUT(h+52U+12U*i,TJBWORD(a+8U));TJBPUT(h+56U+12U*i,TJBWORD(a+28U));}
        return tc_span(request->address,sizeof h,TSG_WRITE,h,1U);
    }
    j=TJBFIND(&job_pool,owner,id);if(!j)return 8U;
    if(op==TSA_IO_JOB_BIND){
        if(selector||offset||TJBWORD(h+20U)||n!=TJD_BYTES||batch_overlap(request->address,sizeof h,payload,n)||tc_span(payload,n,TSG_READ,descriptor,1U))return 8U;
        rc=(unsigned int)TJBBIND(&job_pool,owner,id,descriptor,n);if(rc)return rc;
        if(job_commit())return 12U;
    }else if(op==TSA_IO_JOB_STATUS){
        if(TJBWORD(h+20U)||offset)return 8U;
        if(selector){if(selector>TJBWORD(j+16U)||n!=TJB_STEP_BYTES||batch_overlap(request->address,sizeof h,payload,n)||tc_span(payload,n,TSG_WRITE,descriptor,0U))return 8U;
            s=TJBSTEP(j,selector-1U);if(tc_span(payload,n,TSG_WRITE,s,1U))return 8U;TJBPUT(h+32U,n);}
        else if(n||payload.hi||payload.lo)return 8U;
    }else if(op==TSA_IO_JOB_OUTPUT){
        if(selector||TJBWORD(h+20U)||offset>TJBWORD(j+24U)||!n||n>TJB_PRINT_BYTES||batch_overlap(request->address,sizeof h,payload,n)||tc_span(payload,n,TSG_WRITE,descriptor,0U))return 8U;
        i=TJBWORD(j+24U)-offset;if(i>n)i=n;
        if(tc_span(payload,i,TSG_WRITE,TJBOUT(&job_pool,id)+offset,1U))return 8U;TJBPUT(h+32U,i);
    }else{
        if(selector||n||offset||payload.hi||payload.lo)return 8U;
        if(op==TSA_IO_JOB_PRINT){e=TBCFIND(&batch_endpoints,f->token,TJBWORD(h+20U));
            if(!e||e->kind!=TBC_PRINTER||TSVHAS(&native_invocations,f->token,TSV_BATCH_DEVICE,e->handle)!=TSV_OK)return 8U;
            rc=job_print(j,e);job_status(h,j);if(tc_span(request->address,sizeof h,TSG_WRITE,h,1U))return 8U;return rc;
        }
        if(TJBWORD(h+20U))return 8U;
        if(op==TSA_IO_JOB_RUN){
            if(native_invocations.depth>=TSV_MAX_DEPTH)return 4U;
            if(!job_memory_room())return 4U;
            rc=(unsigned int)TJBSTART(&job_pool,owner,id);if(rc)return rc;
            job_id=id;job_owner=owner;job_reply=request->address;job_base_depth=native_invocations.depth;
            /* The active frame's saved continuation is captured by the same
               native_begin used by LINK. No U-provided caller context. */
            {volatile const unsigned int *v=(volatile const unsigned int *)0x3000U;
             for(i=0U;i<16U;++i){job_caller.gpr[i].hi=v[2U*i];job_caller.gpr[i].lo=v[2U*i+1U];job_caller.fpr[i].hi=v[(0xb0U+8U*i)/4U];job_caller.fpr[i].lo=v[(0xb4U+8U*i)/4U];job_caller.access[i]=v[(0x130U+4U*i)/4U];}
             job_caller.psw.hi=v[32U];job_caller.psw.lo=v[33U];job_caller.psw_address.hi=v[34U];job_caller.psw_address.lo=v[35U];job_caller.asce.hi=v[36U];job_caller.asce.lo=v[37U];job_caller.fpc=v[0x170U/4U];job_caller.key=8U;}
            if(job_note("JOB BEGIN ",id))job_output_error=4U;
            if(job_commit()){TJBPUT(j+32U,12U);return job_dispatch();}return job_dispatch();
        }
        if(op<TSA_IO_JOB_HOLD||op>TSA_IO_JOB_PURGE)return 20U;
        rc=(unsigned int)TJBACT(&job_pool,owner,id,op-TSA_IO_JOB_HOLD+1U);if(rc)return rc;
        if(job_commit())return 12U;
    }
    job_status(h,j);return tc_span(request->address,sizeof h,TSG_WRITE,h,1U);
}
