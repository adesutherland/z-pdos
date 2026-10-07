/* SPDX-License-Identifier: MIT
 * Synchronous native CMS PROGRAM and TSO LINK adapters. This uses the same
 * K invocation, allocation and return machinery as PCOMM ATTACH. */
static void call_result(unsigned int status,unsigned int reason,unsigned int abend,
                         unsigned int valid,unsigned int app_rc,unsigned int token)
{
    unsigned int i;
    for(i=0U;i<TSA_RESULT_BYTES;++i)call_last_result[i]=0U;
    cms_put_word(call_last_result+TSA_RESULT_VERSION,TSA_VERSION);
    cms_put_word(call_last_result+TSA_RESULT_OS_STATUS,status);
    cms_put_word(call_last_result+TSA_RESULT_REASON,reason);
    cms_put_word(call_last_result+TSA_RESULT_ABEND,abend);
    cms_put_word(call_last_result+TSA_RESULT_RC_VALID,valid);
    cms_put_word(call_last_result+TSA_RESULT_APP_RC,app_rc);
    cms_put_word(call_last_result+TSA_RESULT_TOKEN,token);
    for(i=0U;i<TSA_RESULT_BYTES;++i)
        ((volatile unsigned char *)TSF_KAPERTURE_VA)[0x3a800U+i]=call_last_result[i];
}

static void call_error_trace(const TSVFRAME *frame,unsigned int status,
                              unsigned int reason,unsigned int abend,unsigned int exit)
{
    volatile unsigned int *out=(volatile unsigned int *)(TSF_KAPERTURE_VA+0x3a000U);
    volatile const unsigned int *saved=(volatile const unsigned int *)0x3000U;
    unsigned int n=out[0U];
    if(n>=16U)return;
    out[1U+n*8U]=frame?frame->amode:0U;out[2U+n*8U]=status;
    out[3U+n*8U]=reason;out[4U+n*8U]=abend;
    out[5U+n*8U]=saved[2U];out[6U+n*8U]=saved[3U];
    out[7U+n*8U]=exit;out[8U+n*8U]=frame?frame->token:0U;
    out[0U]=n+1U;
}

static unsigned int call_alloc(unsigned int bytes,unsigned int *virtual_address,
                                unsigned char **real_address)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);TSPADDR low,high,address;
    unsigned int i,rc;
    if(!f)return 8U;
    low.hi=high.hi=0U;low.lo=f->amode==24U?0x20000U:0x30000000U;
    high.lo=f->amode==24U?0xffffffU:0x7fffffffU;
    rc=TSMALLOC(&storage,f->runtime_owner,f->amode==24U?24U:31U,low,high,bytes,0,&address);
    if(rc!=TSM_OK)return 4U;
    for(i=0U;i<TSM_ALLOCS;++i)if(storage.allocations[i].handle&&
        storage.allocations[i].task==f->runtime_owner&&
        storage.allocations[i].address.lo==address.lo){
        if(TSVOWN(&native_invocations,f->token,TSV_ALLOCATION,
                   storage.allocations[i].handle)!=TSV_OK)break;
        *virtual_address=address.lo;
        *real_address=(unsigned char *)TSF_KAPERTURE_VA+storage.allocations[i].real;
        return 0U;
    }
    TSMFREE(&storage,f->runtime_owner,address);return 4U;
}

static unsigned int call_launch(TSLIMAGE *image,const unsigned char *parameters,
                                 unsigned int bytes,unsigned int cms_inline)
{
    TSGREQUEST begin,end;TSVFRAME *f;TSVCONTEXT caller;
    const TSVFRAME *parent=TSVTOP(&native_invocations);
    unsigned int parent_task=image&&image->overlay?image->overlay_task:parent?parent->image_owner:0U;
    unsigned int rc,arg,stack,depth,i,returned;unsigned char *arg_real,*stack_real;
    if(!image||(!bytes&&image->personality!=TSV_TSO)||bytes>1024U||
       native_invocations.depth>=TSV_MAX_DEPTH)return 8U;
    begin.address.hi=begin.address.lo=begin.direction=0U;
    begin.length=256U+image->owner;begin.svc=235U;
    rc=native_begin(&begin);if(rc)return rc;
    f=&native_invocations.frame[native_invocations.depth-1U];depth=native_invocations.depth-1U;
    caller=f->caller;
    if(cms_inline)f->caller.psw_address.lo+=4U;
    call_overlay_task[depth]=0U;
    if(image->overlay){
        if(!parent_task||TSMOVERLAYPUSH(&storage,parent_task,image->address,
             (unsigned char *)TSF_KAPERTURE_VA+image->staged_real,image->bytes)!=TSM_OK){rc=4U;goto failed;}
        call_overlay_task[depth]=parent_task;call_overlay_address[depth]=image->address;
    }
    arg=0U;arg_real=0;
    if(bytes){
        rc=call_alloc(4096U,&arg,&arg_real);if(rc)goto failed;
        for(i=0U;i<4096U;++i)arg_real[i]=0U;
    }
    if(image->personality==TSV_CMS)
        for(i=0U;i<bytes;++i)arg_real[i]=parameters[i];
    rc=call_alloc(image->mode==24U?0x10000U:0x100000U,&stack,&stack_real);if(rc)goto failed;
    for(i=0U;i<256U;++i)stack_real[i]=0U;
    cms_put_word(stack_real+204U,stack+384U);
    if(image->personality==TSV_TSO){
        /* Selected native PARAM list: fenced addresses to fullword values.
         * Copying both levels permits the recorded 31/64 -> 24 controls. */
        unsigned int count=bytes/8U;
        for(i=0U;i<count;++i){
            cms_put_word(arg_real+64U+4U*i,cms_word(parameters+4U*i));
            cms_put_word(arg_real+4U*i,(arg+64U+4U*i)|(i+1U==count?0x80000000U:0U));
        }
    }
    f->controlled=3U;++controlled_launches;
    {volatile unsigned int *trace=(volatile unsigned int *)(TSF_KAPERTURE_VA+0x39000U);
     unsigned int n=trace[0U];
     if(n<64U){
         trace[1U+6U*n]=parent?parent->amode:64U;
         trace[2U+6U*n]=image->mode;trace[3U+6U*n]=image->personality;
         trace[4U+6U*n]=cms_word(image->name);trace[5U+6U*n]=cms_word(image->name+4U);
         trace[6U+6U*n]=f->token;trace[0U]=n+1U;
     }}
    *(volatile unsigned int *)0x4f00U=controlled_launches;
    command_enter(image->entry,arg,stack+128U,0xb80U);
    {volatile unsigned int *saved=(volatile unsigned int *)0x3000U;
     saved[32U]=0x04810000U|(image->mode==64U?1U:0U);
     saved[33U]=image->mode==24U?0U:0x80000000U;
     saved[29U]=image->mode==31U?0x80000b80U:0xb80U;
     if(image->personality==TSV_TSO&&image->mode==64U)
         saved[31U]=(caller.psw.hi&1U)?0xfffff004U:
                    (caller.psw.lo&0x80000000U)?0xfffff002U:0xfffff000U;}
    *(volatile unsigned int *)0x4f54U=1U;return 0U;
failed:
    if(call_overlay_task[depth]){
        if(TSMOVERLAYPOP(&storage,call_overlay_task[depth],call_overlay_address[depth],0U,&returned)!=TSM_OK)
            return 12U;
        call_overlay_task[depth]=0U;
    }
    end.address.hi=0U;end.address.lo=f->token;end.length=end.direction=0U;end.svc=236U;
    if(native_end(&end))return 12U;
    command_restore(&caller);call_result(rc,0U,0U,0U,0xffffffffU,end.address.lo);return rc;
}

static unsigned int native_call_finish(void)
{
    volatile unsigned int *saved=(volatile unsigned int *)0x3000U;
    TSVFRAME *f;TSVCONTEXT caller;TSGREQUEST end;
    unsigned int depth,token,rc,kind,status,app_rc,returned;
    if(!native_invocations.depth)return 12U;
    depth=native_invocations.depth-1U;f=&native_invocations.frame[depth];
    if(f->controlled!=3U||!controlled_launches)return 12U;
    if((saved[32U]&0x04f10000U)!=0x04810000U||(saved[32U]&0x0000c000U)||
       saved[36U]!=*(volatile const unsigned int *)0x4008U||
       saved[37U]!=*(volatile const unsigned int *)0x400cU)return 12U;
    kind=saved[39U];
    if(kind!=1U&&kind!=2U)return 12U;
    status=kind==2U?12U:0U;app_rc=kind==2U?0xffffffffU:saved[31U];
    if(kind==2U)++*(volatile unsigned int *)0x4f90U;
    caller=f->caller;token=f->token;
    if(call_overlay_task[depth]){
        if(TSMOVERLAYPOP(&storage,call_overlay_task[depth],call_overlay_address[depth],app_rc,&returned)!=TSM_OK)
            return 12U;
        call_overlay_task[depth]=0U;
    }
    f->controlled=0U;end.address.hi=0U;end.address.lo=token;
    end.length=end.direction=0U;end.svc=236U;rc=native_end(&end);
    if(rc){f->controlled=3U;return rc;}
    --controlled_launches;*(volatile unsigned int *)0x4f00U=controlled_launches;
    command_restore(&caller);saved[0U]=0U;saved[1U]=status;saved[30U]=0U;saved[31U]=app_rc;
    call_result(status,kind==2U?1U:0U,0U,kind==1U,app_rc,token);
    console_v1_end(token,call_last_result);
    if(!native_invocations.depth)
        *(volatile unsigned int *)0x4f9cU=*(volatile const unsigned int *)0x40acU;
    *(volatile unsigned int *)0x4f54U=1U;
    return 0U;
}

static unsigned int native_cms_call(const TSGREQUEST *request,unsigned int profile)
{
    const TSVFRAME *parent=TSVTOP(&native_invocations);TSLIMAGE *image;
    unsigned char parameters[544],token[8];unsigned int n,i,rc,fenced=0U;
    if(!parent||parent->personality!=TSV_CMS||request->address.hi)return 12U;
    for(n=0U;n<sizeof parameters;n+=8U){
        if(profile==24U&&request->address.lo+n>0x1000000U-8U)return 12U;
        if(sf_span(request->address.lo+n,8U,0U,token,1U))return 12U;
        for(i=0U;i<8U;++i)parameters[n+i]=token[i];
        for(i=0U;i<8U&&token[i]==0xffU;++i){}
        if(i==8U){n+=8U;fenced=1U;break;}
    }
    if(!fenced)return 12U;
    if(profile==31U){
        volatile const unsigned int *saved=(volatile const unsigned int *)0x3000U;
        if(saved[1U]||(saved[31U]!=0U&&saved[31U]!=0xa000U))return 12U;
        rc=saved[31U]==0xa000U?load_native(TSV_CMS,24U,parameters,&image):
                              load_callable(TSV_CMS,profile,parameters,&image);
    }else rc=load_native(TSV_CMS,profile,parameters,&image);
    if(rc){
        unsigned int reason=rc==TSL_ABSENT?3U:4U;
        call_result(rc==4U?TSA_OS_STORAGE:TSA_OS_EXECUTION,reason,0U,0U,0xffffffffU,parent->token);
        call_error_trace(parent,rc==4U?TSA_OS_STORAGE:TSA_OS_EXECUTION,reason,0U,0U);
        return rc==TSL_ABSENT?0xfffffffdU:0xfffffffcU;
    }
    return call_launch(image,parameters,n,profile==24U);
}

static unsigned int native_tso_link(void)
{
    const TSVFRAME *parent=TSVTOP(&native_invocations);
    volatile unsigned int *saved=(volatile unsigned int *)0x3000U;
    unsigned char ctl[12],name[8],word[4],parameters[128];
    unsigned int list=saved[3U]&0x7fffffffU,n=0U,p,i,rc;TSLIMAGE *image;
    if(!parent||parent->personality!=TSV_TSO||
       ((saved[32U]&1U)&&(saved[30U]||saved[2U]))||
       (parent->amode==24U&&(saved[31U]&0x7fffffffU)>0x1000000U-sizeof ctl))return 8U;
    if(sf_span(saved[31U]&0x7fffffffU,12U,0U,ctl,1U)||ctl[4U]!=0x80U||
       ctl[5U]||ctl[6U]||ctl[7U]||
       (parent->amode==24U&&cms_word(ctl)>0x1000000U-sizeof name)||
       sf_span(cms_word(ctl),8U,0U,name,1U))return 8U;
    for(i=0U;list&&i<16U;++i){
        if(parent->amode==24U&&list+4U*i>0x1000000U-4U)return 8U;
        if(sf_span(list+4U*i,4U,0U,word,1U))return 8U;
        p=cms_word(word);
        if(parent->amode==24U&&(p&0x7fffffffU)>0x1000000U-4U)return 8U;
        if(sf_span(p&0x7fffffffU,4U,0U,parameters+4U*i,1U))return 8U;
        n=8U*(i+1U);if(p&0x80000000U)break;
    }
    if(list&&i==16U)return 8U;
    rc=load_callable(TSV_TSO,parent->amode,name,&image);
    if(rc){
        TSPADDR exit_address;TSGREQUEST check;TSGCONTEXT gate;
        unsigned int exit=cms_word(ctl+8U)&0x7fffffffU;
        if(rc==TSL_ABSENT&&exit){
            exit_address.hi=0U;exit_address.lo=exit;
            gate.u_tables=&u_tables;gate.real_aperture=(unsigned char *)TSF_KAPERTURE_VA;
            gate.real_bytes=TSF_REAL_BYTES;gate.rights=user_rights;gate.rights_context=0;
            check.address=exit_address;check.length=2U;check.direction=TSG_READ;check.svc=0U;
            if((exit&1U)||TSGPROBE(&gate,&check)!=TSG_OK)return 8U;
            saved[2U]=4U;saved[3U]=0x806U;
            saved[30U]=0U;saved[31U]=exit;
            saved[34U]=0U;saved[35U]=exit;
            call_result(TSA_OS_EXECUTION,4U,0x806U,0U,0xffffffffU,parent->token);
            call_error_trace(parent,TSA_OS_EXECUTION,4U,0x806U,exit);
            *(volatile unsigned int *)0x4f54U=1U;return 0U;
        }
        /* A loader failure is confined to this invocation. The parent frame
         * is restored by the same fault unwind as an unhandled U exception. */
        saved[39U]=2U;
        return native_call_finish();
    }
    return call_launch(image,parameters,n,0U);
}

/* Diagnostic image setup enters the same checked registry loader. */
static unsigned int native_load_probe(TSGREQUEST *request)
{
    unsigned char name[8];TSLIMAGE *image;unsigned int mode=request->length&255U;
    unsigned int personality=request->length>>8,rc;
    TSGCONTEXT gate;TSGREQUEST copy=*request;
    gate.u_tables=&u_tables;gate.real_aperture=(unsigned char *)TSF_KAPERTURE_VA;
    gate.real_bytes=TSF_REAL_BYTES;gate.rights=user_rights;gate.rights_context=0;
    copy.length=8U;copy.direction=TSG_READ;
    if(request->direction||TSGCOPY(&gate,&copy,name,sizeof name)!=TSG_OK)return 8U;
    rc=load_native(personality,mode,name,&image);if(rc)return rc;
    request->address.hi=0U;request->address.lo=image->entry;
    *(volatile unsigned int *)0x4fb0U=256U+image->owner;
    return 0U;
}

static unsigned int native_entry_probe(TSGREQUEST *request)
{
    unsigned char name[8],parameters[8];TSLIMAGE *image;
    unsigned int i,mode=request->length&255U,personality=request->length>>8,rc;
    TSGCONTEXT gate;TSGREQUEST copy=*request;
    gate.u_tables=&u_tables;gate.real_aperture=(unsigned char *)TSF_KAPERTURE_VA;
    gate.real_bytes=TSF_REAL_BYTES;gate.rights=user_rights;gate.rights_context=0;
    copy.length=8U;copy.direction=TSG_READ;
    if(TSVTOP(&native_invocations)||request->direction||
       TSGCOPY(&gate,&copy,name,sizeof name)!=TSG_OK)return 8U;
    if(!*(volatile unsigned int *)0x4f98U)
        *(volatile unsigned int *)0x4f98U=*(volatile const unsigned int *)0x40acU;
    rc=load_native(personality,mode,name,&image);if(rc)return rc;
    for(i=0U;i<8U;++i)parameters[i]=0U;
    if(personality==TSV_TSO){parameters[1U]=2U;parameters[2U]=0x60U;parameters[3U]=0xa5U;}
    return call_launch(image,parameters,8U,0U);
}
