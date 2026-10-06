/* SPDX-License-Identifier: MIT
 * Existing synchronous PCOMM ATTACH/WAIT/DETACH behavior across K/U.
 * Included once by the owning K endpoint, after its checked service helpers.
 */
static unsigned int command_copy(unsigned int address, unsigned int bytes,
                                 unsigned char *buffer, unsigned int write)
{
    TSGCONTEXT gate;
    TSGREQUEST copy;
    if (!address || address>=0x80000000U || bytes>TSG_MAX_COPY) return 8U;
    gate.u_tables=&u_tables;
    gate.real_aperture=(unsigned char *)TSF_KAPERTURE_VA;
    gate.real_bytes=TSF_REAL_BYTES; gate.rights=user_rights;
    gate.rights_context=0;
    copy.address.hi=0; copy.address.lo=address; copy.length=bytes;
    copy.direction=write ? TSG_WRITE : TSG_READ; copy.svc=0;
    return TSGCOPY(&gate,&copy,buffer,bytes)==TSG_OK ? 0U : 8U;
}

static unsigned int command_probe(unsigned int address, unsigned int bytes)
{
    TSGCONTEXT gate; TSGREQUEST request;
    if (!address || address>=0x80000000U || bytes>TSG_MAX_COPY) return 8U;
    gate.u_tables=&u_tables;
    gate.real_aperture=(unsigned char *)TSF_KAPERTURE_VA;
    gate.real_bytes=TSF_REAL_BYTES; gate.rights=user_rights;
    gate.rights_context=0;
    request.address.hi=0; request.address.lo=address;
    request.length=bytes; request.direction=TSG_WRITE; request.svc=0;
    return TSGPROBE(&gate,&request)==TSG_OK ? 0U : 8U;
}

static unsigned int command_alloc(unsigned int bytes, unsigned int *va,
                                  unsigned char **real)
{
    const TSVFRAME *frame=TSVTOP(&native_invocations);
    TSPADDR minimum, maximum, address;
    unsigned int i;
    if (!frame) return 8U;
    minimum.hi=maximum.hi=0; minimum.lo=0x0d000000U;
    maximum.lo=0x7fffffffU;
    if (TSMALLOC(&storage,frame->runtime_owner,31U,minimum,maximum,
                  bytes,0,&address)!=TSM_OK) return 4U;
    for (i=0;i<TSM_ALLOCS;++i)
        if (storage.allocations[i].handle &&
            storage.allocations[i].task==frame->runtime_owner &&
            storage.allocations[i].address.lo==address.lo) {
            if (TSVOWN(&native_invocations,frame->token,TSV_ALLOCATION,
                        storage.allocations[i].handle)!=TSV_OK) break;
            *va=address.lo;
            *real=(unsigned char *)TSF_KAPERTURE_VA+
                  storage.allocations[i].real;
            return 0U;
        }
    return TSMFREE(&storage,frame->runtime_owner,address)==TSM_OK ? 4U : 12U;
}

static void command_enter(unsigned int entry, unsigned int arg,
                          unsigned int stack, unsigned int ret)
{
    volatile unsigned int *saved=(volatile unsigned int *)0x3000U;
    unsigned int i;
    for (i=0;i<32U;++i) saved[i]=0;
    saved[3]=arg; saved[27]=stack;
    saved[29]=ret ? ret|0x80000000U : 0U; saved[31]=entry;
    saved[32]=0x04810000U; saved[33]=0x80000000U;
    saved[34]=0; saved[35]=entry;
}

static void command_observe(unsigned int app_rc)
{
    volatile unsigned int *receipt=(volatile unsigned int *)0x4f40U;
    receipt[3]=command_os_status; receipt[4]=app_rc;
    receipt[5]=native_invocations.depth;
    receipt[6]=TSMLOWFREE(&storage);
}

static unsigned int pcomm_start(const TSGREQUEST *request)
{
    TSGREQUEST init, begin;
    unsigned int status, stack, args, i;
    unsigned char *stack_real, *args_real;
    TSVFRAME *frame;
    if (request->length || request->direction || request->address.hi ||
        request->address.lo || native_invocations.depth) return 8U;
    init=*request; init.svc=208U;
    status=terminal_service(&init);
    if (!status) status=cms31_setup_lowcore();
    if (!status) status=tso_map_service(&init,31U);
    if (!status) status=tso_map_service(&init,32U);
    if (!status) status=tso_map_service(&init,33U);
    if (status) return status;
    begin=init; begin.length=7U;
    status=native_begin(&begin);
    if (status) return status;
    frame=&native_invocations.frame[0];
    status=command_alloc(3U*1024U*1024U,&stack,&stack_real);
    if (!status) status=command_alloc(4096U,&args,&args_real);
    if (status) {
        begin.length=0; begin.address.lo=frame->token;
        native_end(&begin); return status;
    }
    for (i=0;i<4096U;++i) args_real[i]=0;
    cms_put_word(stack_real+76U,stack+256U);
    cms_put_word(args_real,args+16U|0x80000000U);
    /* Empty parameters select the normal interactive PCOMM loop. */
    for (i=0;i<1760U;++i) pcomm_lines[i]=0x40U;
    pcomm_row=pcomm_column=0U;
    pcomm_poll_address=args+256U;
    command_enter(pcomm_entry,args,stack,0U);
    *(volatile unsigned int *)0x4f44U=stack;
    *(volatile unsigned int *)0x4f48U=pcomm_entry;
    return 0U;
}

static unsigned int command_storage10(TSGREQUEST *request)
{
    TSGREQUEST free_request=*request;
    /* The retained PDOS FREEMAIN R macro uses SVC10 with a size prefix.
     * This slice accepts release only; allocation uses checked SVC120. */
    if (request->address.hi || !request->address.lo ||
        (request->address.lo&0x80000000U)) return 8U;
    free_request.length&=0x00ffffffU; free_request.svc=120U;
    return storage_service(&free_request);
}

static unsigned int command_restore(TSVCONTEXT *caller)
{
    volatile unsigned int *saved=(volatile unsigned int *)0x3000U;
    unsigned int i;
    for (i=0;i<16U;++i) {
        saved[2U*i]=caller->gpr[i].hi;
        saved[2U*i+1U]=caller->gpr[i].lo;
    }
    saved[32]=caller->psw.hi; saved[33]=caller->psw.lo;
    saved[34]=caller->psw_address.hi; saved[35]=caller->psw_address.lo;
    saved[36]=caller->asce.hi; saved[37]=caller->asce.lo;
    for (i=0;i<16U;++i) {
        saved[(0xb0U+8U*i)/4U]=caller->fpr[i].hi;
        saved[(0xb4U+8U*i)/4U]=caller->fpr[i].lo;
        saved[(0x130U+4U*i)/4U]=caller->access[i];
    }
    saved[0x170U/4U]=caller->fpc;
    return 0U;
}

static unsigned int pcomm_exit(void)
{
    TSVFRAME *frame=&native_invocations.frame[0];
    TSVCONTEXT caller=frame->caller;
    TSGREQUEST end;
    unsigned int status;
    if (native_invocations.depth!=1U || frame->personality!=TSV_PDOS)
        return 12U;
    end.address.hi=0; end.address.lo=frame->token;
    end.length=end.direction=0; end.svc=236U;
    status=native_end(&end);
    if (status) return status;
    command_restore(&caller);
    *(volatile unsigned int *)0x4f5cU=1U;
    *(volatile unsigned int *)0x4f60U=native_invocations.depth;
    return 0U;
}

/* Recreate writable module state only after private validation and only
 * when its image has no active invocation. A failed reload leaves the old
 * cache mapped, but it is never entered as a substitute for failed input. */
static unsigned int command_refresh(unsigned int selector)
{
    static const unsigned char name[10]={
        0xc3U,0xd4U,0xe2U,0xf3U,0xf1U,0x4bU,0xd9U,0xe7U,0xe5U,0xd4U};
    TSGREQUEST request;
    TSHINFO info;
    unsigned char *stage,*image,*target;
    unsigned int stage_real, image_real, blocks, entry, status, i;
    request.address.hi=request.address.lo=request.length=request.direction=0;
    request.svc=231U;
    if (selector==5U) return tso_map_service(&request,31U);
    if (selector==8U) return tso_map_service(&request,33U);
    if (selector!=2U || native_image_active[4U]) return 8U;
    if (!cms31_loaded) return cms31_map_service(&request);
    status=cms_stage_read(name,10U,31U,&stage,&stage_real,&info,&blocks);
    if (status) return status;
    if (info.image_bytes!=*(volatile unsigned int *)0x40e8U) {
        TSRRELEASE(&storage.real,CMS_STAGE_OWNER,stage_real); return 8U;
    }
    if (TSRALLOC(&storage.real,TSO_IMAGE_OWNER,(info.image_bytes+4095U)&~4095U,
                  TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,&image_real)!=TSR_OK) {
        TSRRELEASE(&storage.real,CMS_STAGE_OWNER,stage_real); return 4U;
    }
    image=(unsigned char *)TSF_KAPERTURE_VA+image_real;
    status=TSHIMAGE(stage,blocks*18452U,31U,0x03000000U,image,
                     info.image_bytes,&entry)==TSH_OK ? 0U : 12U;
    if (!status && entry!=*(volatile unsigned int *)0x40e4U) status=8U;
    if (!status) {
        target=(unsigned char *)TSF_KAPERTURE_VA+
               *(volatile unsigned int *)0x40e0U;
        for (i=0;i<info.image_bytes;++i) target[i]=image[i];
    }
    if (TSRRELEASE(&storage.real,TSO_IMAGE_OWNER,image_real)!=TSR_OK ||
        TSRRELEASE(&storage.real,CMS_STAGE_OWNER,stage_real)!=TSR_OK) return 12U;
    return status;
}

static unsigned int command_attach(void)
{
    volatile unsigned int *saved=(volatile unsigned int *)0x3000U;
    TSGREQUEST begin, end;
    unsigned char ctl[28], name[8], ptr[4], parameters[202], plist[120];
    unsigned char *args_real, *stack_real;
    unsigned int ecb, tcb, tail, count, selector, entry, args, stack;
    unsigned int i, j, field, width, status;
    TSVFRAME *frame;
    const TSVFRAME *parent=TSVTOP(&native_invocations);
    *(volatile unsigned int *)0x4fa0U=1U;
    *(volatile unsigned int *)0x4fa4U=saved[3];
    *(volatile unsigned int *)0x4fa8U=saved[31];
    if (!parent || parent->personality!=TSV_PDOS || command_tcb ||
        saved[30] || saved[2]) return 8U;
    *(volatile unsigned int *)0x4fa0U=2U;
    if (command_copy(saved[31],sizeof ctl,ctl,0)) return 8U;
    for (i=0;i<28U;++i) ((volatile unsigned char *)0x4fc0U)[i]=ctl[i];
    *(volatile unsigned int *)0x4fa0U=3U;
    if (command_copy(cms_word(ctl),8U,name,0) ||
        command_copy(saved[3],4U,ptr,0)) return 8U;
    *(volatile unsigned int *)0x4facU=cms_word(ptr);
    *(volatile unsigned int *)0x4fa0U=4U;
    if (cms_word(ctl+4U) || cms_word(ctl+12U) ||
        (cms_word(ctl+16U)!=0U && cms_word(ctl+16U)!=78U) || cms_word(ctl+20U) ||
        ctl[24] || ctl[25] || ctl[26] || (ctl[27]!=0U && ctl[27]!=8U) ||
        !(cms_word(ptr)&0x80000000U)) return 8U;
    *(volatile unsigned int *)0x4fa0U=5U;
    ecb=cms_word(ctl+8U);
    if (!ecb || ecb>=0x1000000U || (ecb&3U) ||
        command_copy(ecb,4U,ptr,0) || command_copy(ecb,4U,ptr,1)) return 8U;
    if (command_copy(saved[3],4U,ptr,0)) return 8U;
    tail=cms_word(ptr)&0x7fffffffU;
    if (command_copy(tail,2U,parameters,0)) return 8U;
    count=((unsigned int)parameters[0]<<8)|parameters[1];
    if (count>198U || command_copy(tail,count+2U,parameters,0)) return 8U;
    selector=0; entry=0;
    if (name[0]==0xc3U && name[1]==0xd4U && name[2]==0xe2U &&
        name[3]==0x40U && name[4]==0x40U && name[5]==0x40U &&
        name[6]==0x40U && name[7]==0x40U) {
        /* Same bounded token convention as pdosCmsCommand. */
        for (i=0;i<120U;++i) plist[i]=0xffU;
        i=2U; field=0U;
        while (i<count+2U) {
            while (i<count+2U && parameters[i]==0x40U) ++i;
            if (i==count+2U) break;
            if (field>=14U) return 8U;
            for (j=0;j<8U;++j) plist[field*8U+j]=0x40U;
            width=0;
            while (i<count+2U && parameters[i]!=0x40U) {
                if (width==8U) return 8U;
                plist[field*8U+width++]=parameters[i++];
            }
            ++field;
        }
        /* RUN 31 RXVM -v: retain the qualified one-space command subset. */
        if (field<4U || plist[0]!=0xd9U || plist[1]!=0xe4U ||
            plist[2]!=0xd5U || plist[3]!=0x40U ||
            plist[8]!=0xf3U || plist[9]!=0xf1U || plist[10]!=0x40U ||
            plist[16]!=0xd9U || plist[17]!=0xe7U ||
            plist[18]!=0xe5U || plist[19]!=0xd4U || plist[20]!=0x40U)
            return 4U;
        selector=2U; entry=*(volatile unsigned int *)0x40e4U;
        for (i=0;i<104U;++i) plist[i]=plist[i+16U];
        for (;i<120U;++i) plist[i]=0xffU;
    } else if (name[0]==0xd9U && name[1]==0xe7U && name[2]==0xe5U &&
               name[3]==0xd4U && name[4]==0x40U && name[5]==0x40U &&
               name[6]==0x40U && name[7]==0x40U) {
        selector=5U; entry=*(volatile unsigned int *)0x4608U;
    } else if (name[0]==0xd7U && name[1]==0xc4U && name[2]==0xe3U &&
               name[3]==0xc5U && name[4]==0xe2U && name[5]==0xe3U &&
               name[6]==0x40U && name[7]==0x40U) {
        selector=8U;
    } else return 4U;
    status=command_refresh(selector);
    if (status) return status;
    entry=selector==2U ? *(volatile unsigned int *)0x40e4U :
          selector==5U ? *(volatile unsigned int *)0x4608U : command_test_entry;
    if (!entry) return 8U;
    /* The completion TCB is U-owned parent storage and survives child cleanup. */
    status=command_alloc(4096U,&tcb,&args_real);
    if (status) return status;
    for (i=0;i<20U;++i) args_real[i]=0;
    begin.address.hi=begin.address.lo=0U; begin.length=selector;
    begin.direction=0; begin.svc=235U;
    status=native_begin(&begin);
    if (status) goto no_child;
    frame=&native_invocations.frame[native_invocations.depth-1U];
    status=command_alloc(3U*1024U*1024U,&stack,&stack_real);
    if (!status) status=command_alloc(4096U,&args,&args_real);
    if (status) {
        end=begin; end.length=0; end.address.lo=frame->token;
        if (native_end(&end)) return 12U;
        goto no_child;
    }
    for (i=0;i<4096U;++i) args_real[i]=0;
    cms_put_word(stack_real+76U,stack+256U);
    args_real[0]=0x0aU; args_real[1]=3U; /* native return SVC3 */
    if (selector==2U) {
        for (i=0;i<120U;++i) args_real[128U+i]=plist[i];
    } else {
        cms_put_word(args_real+128U,args+144U|0x80000000U);
        for (i=0;i<count+2U;++i) args_real[144U+i]=parameters[i];
    }
    command_ecb=ecb; command_tcb=tcb; command_os_status=0;
    frame->controlled=2U; ++controlled_launches;
    *(volatile unsigned int *)0x4f00U=controlled_launches;
    command_enter(entry,args+128U,stack,args);
    ++*(volatile unsigned int *)0x4f40U;
    command_observe(0U);
    return 0U;
no_child:
    { TSPADDR address; unsigned int handle;
      address.hi=0; address.lo=tcb;
      handle=allocation_handle(parent->runtime_owner,address);
      if (!handle || TSMFREE(&storage,parent->runtime_owner,address)!=TSM_OK ||
          TSVFORGET(&native_invocations,parent->token,TSV_ALLOCATION,handle)!=TSV_OK)
          return 12U;
    }
    return status;
}

static unsigned int command_dispatch_attach(void)
{
    unsigned int status=command_attach();
    if (status) {
        command_os_status=status;
        command_observe(0xffffffffU);
    }
    return status;
}

static unsigned int command_finish(void)
{
    volatile unsigned int *saved=(volatile unsigned int *)0x3000U;
    TSVFRAME *frame=&native_invocations.frame[native_invocations.depth-1U];
    TSVCONTEXT caller=frame->caller;
    TSGREQUEST end;
    unsigned int kind=saved[39], app_rc=kind==1U ? saved[31] : 0xffffffffU;
    unsigned int child_owner=frame->runtime_owner, i;
    unsigned char result[4];
    if (native_invocations.depth!=2U || frame->controlled!=2U ||
        !command_tcb || (kind!=1U && kind!=2U)) return 12U;
    command_os_status=kind==2U ? 12U : 0U;
    if (kind==2U) ++*(volatile unsigned int *)0x4f68U;
    end.address.hi=0; end.address.lo=frame->token;
    end.length=end.direction=0; end.svc=236U;
    frame->controlled=0;
    if (native_end(&end)) { frame->controlled=2U; return 12U; }
    --controlled_launches;
    *(volatile unsigned int *)0x4f00U=controlled_launches;
    for (i=0;i<TSM_ALLOCS;++i)
        if (storage.allocations[i].handle &&
            storage.allocations[i].task==child_owner &&
            !(child_owner==22U && storage.allocations[i].address.lo==0x0c000000U))
            return 12U;
    for (i=0;i<storage.real.count;++i)
        if (storage.real.ranges[i].owner==CMS_LOWCORE_SAVE_OWNER ||
            storage.real.ranges[i].owner==CMS_STAGE_OWNER ||
            storage.real.ranges[i].owner==TSO_STAGE_OWNER ||
            storage.real.ranges[i].owner==TSO_IMAGE_OWNER) return 12U;
    ++*(volatile unsigned int *)0x4f10U;
    command_restore(&caller);
    /* Preserve PDPCLIB's legacy result view while recording OS status. */
    cms_put_word(result,command_os_status ? 0x8000c000U : app_rc);
    if (command_copy(command_tcb+16U,4U,result,1)) return 12U;
    cms_put_word(result,0x40000000U|(app_rc&0x00ffffffU));
    if (command_copy(command_ecb,4U,result,1)) return 12U;
    saved[2]=0; saved[3]=command_tcb; saved[30]=saved[31]=0;
    ++*(volatile unsigned int *)0x4f6cU;
    command_observe(app_rc);
    return 0U;
}

static unsigned int command_wait_detach(unsigned int svc)
{
    volatile unsigned int *saved=(volatile unsigned int *)0x3000U;
    unsigned char word[4];
    TSPADDR address;
    unsigned int handle;
    const TSVFRAME *frame=TSVTOP(&native_invocations);
    if (!frame || frame->personality!=TSV_PDOS || !command_tcb || saved[2])
        return 8U;
    if (svc==1U) {
        if (saved[3]!=command_ecb || command_copy(command_ecb,4U,word,0) ||
            !(cms_word(word)&0x40000000U)) return 8U;
        return 0U;
    }
    if (command_copy(saved[3],4U,word,0) || cms_word(word)!=command_tcb)
        return 8U;
    address.hi=0; address.lo=command_tcb;
    handle=allocation_handle(frame->runtime_owner,address);
    if (!handle || TSMFREE(&storage,frame->runtime_owner,address)!=TSM_OK ||
        TSVFORGET(&native_invocations,frame->token,TSV_ALLOCATION,handle)!=TSV_OK)
        return 12U;
    command_tcb=command_ecb=0U;
    return 0U;
}

static unsigned int pcomm_terminal(TSGREQUEST *request)
{
    TSGREQUEST poll;
    volatile unsigned int *saved=(volatile unsigned int *)0x3000U;
    unsigned char message[256], input[256];
    unsigned int i, n, status;
    if (request->address.hi || request->length>252U) return 8U;
    if (request->address.lo&0x80000000U) {
        /* Suspend on a saved SVC continuation, then retry by explicit SVC.
         * The U adapter never guesses I/O completion from elapsed time. */
        if (!pcomm_input_address) {
            *(volatile unsigned int *)0x4f04U=TSMLOWFREE(&storage);
            *(volatile unsigned int *)0x4f08U=native_invocations.depth;
            *(volatile unsigned int *)0x4f0cU=storage.real.count;
            if (command_probe(request->address.lo&0x7fffffffU,request->length))
                return 8U;
            pcomm_input_address=request->address.lo&0x7fffffffU;
            pcomm_input_capacity=request->length;
            poll=*request; poll.address.hi=poll.address.lo=0;
            poll.length=poll.direction=0;
            status=terminal_read_start(&poll);
            if (status) { pcomm_input_address=0; return status; }
        }
        /* Copy into the adapter's full 256-byte scratch span, which it owns. */
        poll.address.hi=0; poll.address.lo=pcomm_poll_address;
        poll.length=256U; poll.direction=TSG_WRITE; poll.svc=210U;
        status=terminal_read_poll(&poll);
        if (status==1U) {
            saved[35]-=2U; return 0U;
        }
        if (status) { pcomm_input_address=0; return status; }
        if (command_copy(pcomm_poll_address,256U,input,0)) return 8U;
        n=cms_word(input);
        if (n==3U && input[4]==0x7dU) n=0U;
        else if (n>=6U && input[7]==0x11U) n-=6U;
        else { pcomm_input_address=0; return 8U; }
        if (n>pcomm_input_capacity) { pcomm_input_address=0; return 8U; }
        for (i=0;i<n;++i) message[i]=input[i+10U];
        status=n ? command_copy(pcomm_input_address,n,message,1) : 0U;
        pcomm_input_address=0;
        saved[2]=0; saved[3]=n;
        return status;
    }
    if (request->length>130U ||
        command_copy(request->address.lo,request->length,message,0)) return 8U;
    return pcomm_display(message,request->length);
}

static unsigned int pcomm_display(const unsigned char *message, unsigned int length)
{
    unsigned int i,n;
    if (!console_ssid || console_read_phase || !message || length>130U) return 8U;
    if (pcomm_row>=22U) {
        for (n=0;n<1680U;++n) pcomm_lines[n]=pcomm_lines[n+80U];
        for (;n<1760U;++n) pcomm_lines[n]=0x40U;
        pcomm_row=21U;
    }
    for (i=0;i<length;++i) {
        if (pcomm_column==80U) { pcomm_column=0; ++pcomm_row; }
        if (pcomm_row==22U) {
            for (n=0;n<1680U;++n) pcomm_lines[n]=pcomm_lines[n+80U];
            for (;n<1760U;++n) pcomm_lines[n]=0x40U;
            pcomm_row=21U;
        }
        pcomm_lines[pcomm_row*80U+pcomm_column++]=message[i];
    }
    /* Every native output record terminates a line, including the prompt. */
    pcomm_column=0; ++pcomm_row;
    { unsigned char *screen=TSCDATA(&console_channel);
      for (i=0;i<1773U;++i) screen[i]=0x40U;
      screen[0]=0xc3U; screen[1]=0x11U; screen[2]=0x5dU;
      screen[3]=0x7fU; screen[4]=0x1dU; screen[5]=0xf0U;
      for (i=0;i<1760U;++i) screen[6U+i]=pcomm_lines[i];
      screen[1766]=0x1dU; screen[1767]=0; screen[1768]=0x13U;
      screen[1769]=0x3cU; screen[1770]=0x5dU; screen[1771]=0x7fU;
      screen[1772]=0;
      if (TSCBUILDCONSWRITE(&console_channel,1773U)!=TSC_OK ||
          TSCIO(console_ssid,TSCORB(&console_channel),TSCIRB(&console_channel)) ||
          TSCCHECKWRITE(&console_channel)!=TSC_OK) return 12U;
    }
    return 0U;
}
