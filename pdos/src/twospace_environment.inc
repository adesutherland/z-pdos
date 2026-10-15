/* SPDX-License-Identifier: MIT
 * K validates native interfaces and owns synchronous continuations/resources.
 * Command parsing/presentation is the stateless resident U/C dispatcher. */
typedef struct {
    unsigned int token,active,argument,stack,environment,outputs[3],refs[4];
    unsigned int native_kind;
    TSVCONTEXT caller;
} TCMCALL;
static TCMCALL env_calls[TSV_MAX_DEPTH];

static void env_capture(TSVCONTEXT *c)
{
    volatile const unsigned int *s=(volatile const unsigned int *)0x3000U;
    unsigned int i;
    for(i=0U;i<16U;++i){c->gpr[i].hi=s[2U*i];c->gpr[i].lo=s[2U*i+1U];
        c->fpr[i].hi=s[(0xb0U+8U*i)/4U];c->fpr[i].lo=s[(0xb4U+8U*i)/4U];c->access[i]=s[(0x130U+4U*i)/4U];}
    c->psw.hi=s[32U];c->psw.lo=s[33U];c->psw_address.hi=s[34U];c->psw_address.lo=s[35U];
    c->asce.hi=s[36U];c->asce.lo=s[37U];c->fpc=s[0x170U/4U];c->key=(c->psw.hi>>20)&15U;
}
static TCMCALL *env_current(void)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);TCMCALL *c;unsigned int i;
    if(!f||f->amode!=31U||!env_entry[0])return 0;
    c=&env_calls[native_invocations.depth-1U];
    if(c->token!=f->token){
        for(i=0U;i<sizeof *c;++i)((unsigned char *)c)[i]=0U;
        c->token=f->token;
    }
    return c;
}
static unsigned int env_free(unsigned int *va)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);TSPADDR a;unsigned int handle;
    if(!*va)return 0U;if(!f)return 12U;a.hi=0U;a.lo=*va;
    handle=allocation_handle(f->runtime_owner,a);
    if(!handle||TSMFREE(&storage,f->runtime_owner,a)!=TSM_OK||
       TSVFORGET(&native_invocations,f->token,TSV_ALLOCATION,handle)!=TSV_OK)return 12U;
    *va=0U;return 0U;
}
static unsigned int env_launch(unsigned int env,unsigned int address,unsigned int length,
                                unsigned int kind,const unsigned int *outputs)
{
    TCMCALL *c=env_current();unsigned char *args,*stack;unsigned int i,rc;
    if(!c||c->active||!length||length>TCM_MAX||sf_span(address,length,0U,0,0U))return 12U;
    for(i=0U;i<3U;++i)c->outputs[i]=outputs?outputs[i]:0U;
    rc=call_alloc((length+32U+4095U)&~4095U,&c->argument,&args);if(rc)return rc;
    rc=call_alloc(0x10000U,&c->stack,&stack);
    if(rc){if(env_free(&c->argument))return 12U;return rc;}
    for(i=0U;i<256U;++i)stack[i]=0U;
    cms_put_word(stack+76U,c->stack+256U);
    cms_put_word(args,c->argument+16U);cms_put_word(args+16U,env);cms_put_word(args+20U,length);
    if(sf_span(address,length,0U,args+24U,1U)){
        if(env_free(&c->stack)||env_free(&c->argument))return 12U;
        return 12U;
    }
    c->native_kind=kind;env_capture(&c->caller);c->active=1U;
    command_enter(env_entry[0],c->argument,c->stack,0U);
    *(volatile unsigned int *)0x4f54U=1U;return 0U;
}
static unsigned int env_finish(unsigned int fault)
{
    volatile unsigned int *s=(volatile unsigned int *)0x3000U;
    TCMCALL *c=env_current();unsigned char values[8],word[4];
    unsigned int status,rc,kind,i;
    if(!c||!c->active)return 12U;
    kind=c->native_kind;
    if(fault){status=12U;rc=0xffffffffU;}
    else{if(sf_span(c->argument+16U,8U,0U,values,1U))return 12U;
        status=cms_word(values);rc=cms_word(values+4U);}
    /* Outputs were probed before launch and U mappings remain owned/stable. */
    if(kind==2U||kind==3U){
        cms_put_word(word,rc);
        if(sf_span(c->outputs[0],4U,1U,word,1U))return 12U;
        cms_put_word(word,status);if(sf_span(c->outputs[1],4U,1U,word,1U))return 12U;
        /* A contained fault is a facility failure; no unverified ABEND code. */
        cms_put_word(word,0U);if(sf_span(c->outputs[2],4U,1U,word,1U))return 12U;
    }
    if(env_free(&c->stack)||env_free(&c->argument))return 12U;
    c->active=0U;
    command_restore(&c->caller);
    if(kind==1U)s[31U]=status?0xfffffffcU:rc;
    else s[31U]=status?12U:rc?4U:0U;
    for(i=0U;i<3U;++i)c->outputs[i]=0U;
    *(volatile unsigned int *)0x4f54U=1U;
    return 0U;
}
static int env_name(const unsigned char name[8],const char *text)
{unsigned int i;for(i=0U;i<8U;++i){if(name[i]!=(unsigned char)(*text?*text++:' '))return 0;}return !*text;}
static unsigned int env_cms(const TSGREQUEST *request)
{
    volatile const unsigned int *s=(volatile const unsigned int *)0x3000U;
    unsigned char p[32],e[16];unsigned int env,start,end;
    if(s[0U]||s[30U]||sf_span(request->address.lo,16U,0U,p,1U))return 12U;
    if(env_name(p,"SUBCOM")){
        if(s[31U]!=0xa000U||s[1U]||sf_span(request->address.lo,32U,0U,p,1U)||
           cms_word(p+16U)||cms_word(p+20U)!=0xffffffffU||
           cms_word(p+24U)!=0xffffffffU||cms_word(p+28U)!=0xffffffffU)return 12U;
        return env_entry[0]&&(env_name(p+8U,"CMS")||env_name(p+8U,"PDOS"))?0U:1U;
    }
    if(s[31U]!=0x0200b000U||!s[1U]||cms_word(p+8U)!=0xffffffffU||cms_word(p+12U)!=0xffffffffU||
       sf_span(s[1U],16U,0U,e,1U)||cms_word(e)!=request->address.lo||cms_word(e+12U))return 12U;
    env=env_name(p,"CMS")?TCM_CMS:env_name(p,"PDOS")?TCM_PDOS:0U;
    if(!env)return 0xfffffffdU;
    start=cms_word(e+4U);end=cms_word(e+8U);
    if(end<=start||end-start>TCM_MAX)return 12U;
    return env_launch(env,start,end-start,1U,0);
}
static unsigned int env_module(unsigned int svc,unsigned int *handled)
{
    volatile unsigned int *s=(volatile unsigned int *)0x3000U;
    const TSVFRAME *f=TSVTOP(&native_invocations);TCMCALL *c=env_current();
    unsigned char name[8];unsigned int i;
    *handled=0U;
    if(!f||f->personality!=TSV_TSO||f->amode!=31U||!c||s[0U]||s[2U])return 8U;
    if(sf_span(s[1U],8U,0U,name,1U))return 8U;
    for(i=0U;i<4U;++i)if(env_name(name,i==0U?"IRXINIT":i==1U?"IRXSUBCM":i==2U?"IKJEFTSR":"PDOSCMD"))break;
    if(i==4U)return 8U;*handled=1U;
    if(svc==8U){
        if(s[3U]!=0x80000000U||c->refs[i]==0xffffffffU)return 8U;
        ++c->refs[i];s[0U]=0U;s[1U]=env_entry[i+1U]|0x80000000U;s[30U]=s[31U]=0U;
    }else{
        if(s[3U]||!c->refs[i])return 4U;
        --c->refs[i];s[30U]=s[31U]=0U;
    }
    *(volatile unsigned int *)0x4f54U=1U;return 0U;
}
static unsigned int env_parameters(unsigned int count,unsigned int *p)
{
    volatile const unsigned int *s=(volatile const unsigned int *)0x3000U;
    unsigned char bytes[28];unsigned int i,w;
    if(s[2U]||sf_span(s[3U],4U*count,0U,bytes,1U))return 20U;
    for(i=0U;i<count;++i){w=cms_word(bytes+4U*i);
        if(!!(w&0x80000000U)!=(i+1U==count))return 20U;
        p[i]=w&0x7fffffffU;
    }
    return 0U;
}
static unsigned int env_tso(unsigned int op)
{
    TCMCALL *c=env_current();unsigned int p[7],rc,n,i,value;
    unsigned char text[32],word[4],*real;
    if(!c||c->active||!c->refs[op-1U])return 20U;
    rc=env_parameters(op==1U?7U:op==2U?5U:6U,p);if(rc)return rc;
    if(op==1U){
        if(sf_span(p[0],8U,0U,text,1U)||!env_name(text,"FINDENVB")||
           sf_span(p[1],8U,0U,text,1U)||!env_name(text,"")||sf_span(p[5],4U,1U,word,0U)||
           sf_span(p[6],4U,1U,word,0U))return 20U;
        for(i=2U;i<5U;++i)if(sf_span(p[i],4U,0U,word,1U)||cms_word(word))return 20U;
        if(!c->environment){
            rc=call_alloc(4096U,&c->environment,&real);if(rc)return 20U;
            for(i=0U;i<4096U;++i)real[i]=0U;
            cms_put_word(real,0x50444531U);cms_put_word(real+4U,c->token);
        }
        cms_put_word(word,c->environment);if(sf_span(p[5],4U,1U,word,1U))return 20U;
        cms_put_word(word,0U);if(sf_span(p[6],4U,1U,word,1U))return 20U;
        {volatile unsigned int *s=(volatile unsigned int *)0x3000U;
         s[0U]=0U;s[1U]=c->environment;s[30U]=s[31U]=0U;
         *(volatile unsigned int *)0x4f54U=1U;}
        return 0U;
    }
    if(op==2U){
        if(sf_span(p[0],8U,0U,text,1U)||!env_name(text,"QUERY")||
           sf_span(p[4],4U,0U,word,1U)||!c->environment||cms_word(word)!=c->environment||
           sf_span(p[3],8U,0U,text,1U))return 20U;
        if(!env_name(text,"TSO")&&!env_name(text,"PDOS"))return 8U;
        if(sf_span(p[2],4U,0U,word,1U)||cms_word(word)<32U||
           sf_span(p[1],4U,0U,word,1U))return 20U;
        value=cms_word(word);
        {unsigned char name[8];const char *routine;
         if(sf_span(p[3],8U,0U,name,1U))return 20U;
         routine=env_name(name,"PDOS")?"PDOSCMD ":"IKJEFTSR";
         for(i=0U;i<32U;++i)text[i]=0U;
         for(i=0U;i<8U;++i){text[i]=name[i];text[8U+i]=(unsigned char)routine[i];}}
        /* Only the QUERY subset is supplied; no registration/callback table. */
        return sf_span(value,32U,1U,text,1U)?20U:0U;
    }
    if(sf_span(p[0],4U,0U,word,1U)||cms_word(word)!=0x00010001U||
       sf_span(p[2],4U,0U,word,1U))return 20U;
    n=cms_word(word);
    for(i=3U;i<6U;++i)if(sf_span(p[i],4U,1U,word,0U))return 20U;
    return env_launch(op==3U?TCM_TSO:TCM_PDOS,p[1],n,op==3U?2U:3U,p+3U)?20U:0U;
}
static unsigned int env_host(unsigned int op,unsigned int address,unsigned int n)
{
    unsigned char bytes[256],word[8];unsigned int i,rc;
    const TSVFRAME *f=TSVTOP(&native_invocations);
    if(!f||f->amode!=31U)return 8U;
    if(op==TCM_CLOCK){
        unsigned int clock[2];if(n!=8U||sf_span(address,n,1U,word,0U)||TSCNOW(clock))return 12U;
        cms_put_word(word,clock[0]);cms_put_word(word+4U,clock[1]);return sf_span(address,n,1U,word,1U);
    }
    if(op==TCM_WRITE){
        if(job_dd_scope()){
            static const unsigned char output[8]={0xe2,0xe8,0xe2,0xe3,0xe2,0xd7,0xd9,0xe3};
            SFDD *dd=sf_find_dd(output);unsigned char block[140];unsigned int length,k;
            if(!dd||!dd->batch||TJDWORD(dd->batch+4U)!=TJD_SYSOUT||
               TJDWORD(dd->batch+8U)!=TJD_OUTPUT||n>132U||
               (n&&sf_span(address,n,0U,bytes,1U)))return 20U;
            if(TJDWORD(dd->batch+12U)==TJD_FB){
                length=TJDWORD(dd->batch+16U);if(n>length)return 20U;
                for(k=0U;k<length;++k)block[k]=k<n?bytes[k]:0x40U;
            }else{
                length=n+8U;if(n+4U>TJDWORD(dd->batch+16U))return 20U;
                sf_put_half(block,length);block[2]=block[3]=0U;
                sf_put_half(block+4U,n+4U);block[6]=block[7]=0U;
                for(k=0U;k<n;++k)block[8U+k]=bytes[k];
            }
            return job_native_output(dd->batch,block,length);
        }
        if(n>TCM_MAX||(n&&sf_span(address,n,0U,bytes,0U)))return 8U;
        if(!n)return console_v1_line(bytes,0U);
        for(i=0U;i<n;i+=256U){unsigned int take=n-i<256U?n-i:256U;
            if(sf_span(address+i,take,0U,bytes,1U))return 8U;
            rc=console_v1_line(bytes,take);if(rc)return rc;}
        return 0U;
    }
    if(!n||n>sizeof bytes||sf_span(address,n,0U,bytes,1U))return 8U;
    if(op==TCM_STAT_CMS){
        TSIINPUT *input;unsigned int opened;
        if(n!=18U)return 8U;
        if(cms_output_find(bytes,31U,f->token))return 0U;
        input=TSIFINDOWNED(&cms_inputs,bytes,31U,f->token);if(input)return 0U;
        rc=cms_file_open(bytes,31U,&input);if(rc)return rc;
        opened=cms_file_close(input);return opened?12U:0U;
    }
    if(op==TCM_STAT_TSO){
        TSKEXTENT extent;int found;unsigned int device=media_selected_device?media_selected_device:*(volatile const unsigned int *)0x40bcU;
        if(n>44U)return 8U;
        found=TSKFIND(sf_volume_record,&device,bytes,n,&extent);
        return found==TSK_OK?0U:found==TSK_ABSENT?28U:12U;
    }
    if(op==TCM_MEDIA){
        char verb[9];unsigned char parameters[202],name[8];unsigned int end=0U,token=native_invocations.next_token;
        if(n>198U)return 8U;
        while(end<n&&bytes[end]!=0x40U&&end<8U){verb[end]=(char)media_upper(bytes[end]);++end;}
        if(end==8U&&end<n&&bytes[end]!=0x40U)return 8U;verb[end]=0;
        parameters[0]=(unsigned char)((n-end)>>8);parameters[1]=(unsigned char)(n-end);
        for(i=end;i<n;++i)parameters[2U+i-end]=bytes[i];
        if(!token||token==0xffffffffU)return 12U;
        for(i=0U;i<8U;++i)name[i]=i<end?(unsigned char)verb[i]:0x40U;
        ++native_invocations.next_token;console_v1_operator(token,name,8U,0U);
        rc=(unsigned int)media_operator(verb,parameters);
        call_result(0U,0U,0U,1U,rc,token);console_v1_operator(token,name,8U,1U);
        return rc;
    }
    if(op==TCM_RUN){
        unsigned char name[8],parameters[202];TSLIMAGE *image;unsigned int end=0U,start;
        TCMCALL *c=env_current();
        if(!c||!c->active||c->native_kind==2U||n>198U)return 8U;
        while(end<n&&bytes[end]!=0x40U){if(end==8U)return 8U;name[end]=media_upper(bytes[end]);++end;}
        if(!end)return 8U;for(i=end;i<8U;++i)name[i]=0x40U;
        start=end;while(start<n&&bytes[start]==0x40U)++start;
        rc=load_callable(TSV_TSO,31U,name,&image);
        if(rc){env_run_result(rc==TSL_ABSENT?0U:12U,0xfffffffdU);return 0U;}
        /* A new program launch needs pristine writable runtime/image state,
         * just as PCOMM ATTACH does. A mapped image is only a checked cache. */
        rc=load_refresh(image);
        if(rc){env_run_result(rc,0xffffffffU);return rc;}
        parameters[0]=0U;parameters[1]=(unsigned char)(n-start);
        for(i=start;i<n;++i)parameters[2U+i-start]=bytes[i];
        rc=call_launch(image,parameters,n-start+2U,2U);
        if(rc)env_run_result(rc,0xffffffffU);
        return rc;
    }
    return 20U;
}
static void env_run_result(unsigned int status,unsigned int rc)
{
    TCMCALL *c=env_current();unsigned char values[8];
    if(!c||!c->active)return;
    cms_put_word(values,status);cms_put_word(values+4U,rc);
    if(sf_span(c->argument+16U,8U,1U,values,1U))c->native_kind=0U;
}
static unsigned int env_service(TSGREQUEST *request)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);
    unsigned int op=request->length;
    if(!f||native_caller(*(volatile const unsigned int *)0x308cU)!=f||f->amode!=31U||
       (*(volatile const unsigned int *)0x3080U&0x04f10001U)!=0x04810000U)return 8U;
    if(job_active() && (op==4U || op==TCM_MEDIA || op==TCM_RUN || op==TCM_STAT_CMS))return 20U;
    if(op==TCM_DONE)return env_finish(0U);
    if(op>=TCM_CLOCK){unsigned int rc;
        if(request->address.hi)return 8U;
        rc=env_host(op,request->address.lo,request->direction);
        if(op==TCM_RUN&&rc)env_run_result(rc,0xffffffffU);return rc;}
    if(f->personality!=TSV_TSO)return 20U;
    return op>=1U&&op<=4U?env_tso(op):20U;
}
