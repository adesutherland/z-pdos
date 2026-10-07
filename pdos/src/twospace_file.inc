/* SPDX-License-Identifier: MIT
 * Selected native TSO file lists, copied into the owning K service.
 * The producer's PDPCLIB layouts and U callbacks remain the application ABI. */
#define SFDDS 8U
#define SFFILES 8U
#define SF_DD_HANDLE 0x200U
#define SF_FILE_HANDLE 0x400U
#define SF_REAL_OWNER 0x54534600U
typedef struct {
    unsigned int token,bytes;
    unsigned char dd[8],dsn[44],dscb[96];
    TSKEXTENT extent;
} SFDD;
typedef struct {
    unsigned int token,dcb,dd,mode,cylinder,head,record;
    unsigned int real,length,cursor,member,loaded,ecb;
    unsigned char key[52];
} SFFILE;
static SFDD sf_dd[TSV_MAX_DEPTH*SFDDS];
static SFFILE sf_file[TSV_MAX_DEPTH*SFFILES];
static unsigned int sf_next_dd;
static unsigned int sf_exit_token[TSV_MAX_DEPTH];
static unsigned int sf_exit_saved[TSV_MAX_DEPTH][128];

static unsigned int sf_half(const unsigned char *p)
{return ((unsigned int)p[0]<<8)|p[1];}
static void sf_put_half(unsigned char *p,unsigned int v)
{p[0]=(unsigned char)(v>>8);p[1]=(unsigned char)v;}
static int sf_equal(const unsigned char *p,const unsigned char *q,unsigned int n)
{unsigned int i;for(i=0U;i<n&&p[i]==q[i];++i){}return i==n;}
static unsigned int sf_span(unsigned int address,unsigned int n,unsigned int write,
                             unsigned char *bytes,unsigned int perform)
{
    TSGCONTEXT gate;TSGREQUEST r;unsigned int at,take;
    if(!address||address>=0x80000000U||!n||n>0x80000000U-address)return 8U;
    gate.u_tables=&u_tables;gate.real_aperture=(unsigned char *)TSF_KAPERTURE_VA;
    gate.real_bytes=TSF_REAL_BYTES;gate.rights=user_rights;gate.rights_context=0;
    r.address.hi=0U;r.direction=write?TSG_WRITE:TSG_READ;r.svc=0U;
    for(at=0U;at<n;at+=take){take=n-at;if(take>TSG_MAX_COPY)take=TSG_MAX_COPY;
        r.address.lo=address+at;r.length=take;
        if(TSGPROBE(&gate,&r)!=TSG_OK)return 8U;}
    if(!perform)return 0U;
    if(!bytes)return 8U;
    for(at=0U;at<n;at+=take){take=n-at;if(take>TSG_MAX_COPY)take=TSG_MAX_COPY;
        r.address.lo=address+at;r.length=take;
        if(TSGCOPY(&gate,&r,bytes+at,take)!=TSG_OK)return 8U;}
    return 0U;
}
static SFDD *sf_find_dd(const unsigned char name[8])
{
    const TSVFRAME *f=TSVTOP(&native_invocations);unsigned int i;
    if(!f)return 0;
    for(i=0U;i<TSV_MAX_DEPTH*SFDDS;++i)
        if(sf_dd[i].token==f->token&&sf_equal(sf_dd[i].dd,name,8U))return &sf_dd[i];
    return 0;
}
static SFFILE *sf_find_file(unsigned int dcb)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);unsigned int i;
    if(!f)return 0;
    for(i=0U;i<TSV_MAX_DEPTH*SFFILES;++i)
        if(sf_file[i].token==f->token&&sf_file[i].dcb==dcb)return &sf_file[i];
    return 0;
}
static void sf_compat(void)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);unsigned char *p,*entry,*j,*u;
    unsigned int i,n=0U,slot,queue,ucb;
    if(!f||!cms31_lowcore_real)return;
    p=(unsigned char *)TSF_KAPERTURE_VA+cms31_lowcore_real;
    cms_put_word(p+0x21cU,0xc00U);cms_put_word(p+0xc0cU,0xc80U);
    for(i=0xc80U;i<0xd50U;++i)p[i]=0U;
    for(i=0U;i<TSV_MAX_DEPTH*SFDDS;++i)if(sf_dd[i].token==f->token){
        slot=n++;queue=0x10000U+slot*192U;ucb=0x10c00U+slot*64U;
        entry=p+0xc98U+slot*20U;entry[0]=20U;
        {unsigned int k;for(k=0U;k<8U;++k)entry[4U+k]=sf_dd[i].dd[k];}
        entry[12]=(unsigned char)(queue>>16);entry[13]=(unsigned char)(queue>>8);
        entry[14]=(unsigned char)queue;entry[17]=(unsigned char)(ucb>>16);
        entry[18]=(unsigned char)(ucb>>8);entry[19]=(unsigned char)ucb;
        j=p+4096U+slot*192U;
        {unsigned int k;for(k=0U;k<192U;++k)j[k]=0U;
         for(k=0U;k<44U;++k)j[16U+k]=k<sf_dd[i].bytes?sf_dd[i].dsn[k]:0x40U;}
        j+=16U;sf_put_half(j+98U,sf_dd[i].extent.organisation);
        j[100U]=(unsigned char)sf_dd[i].extent.record_format;
        sf_put_half(j+102U,sf_dd[i].extent.block_length);
        sf_put_half(j+104U,sf_dd[i].extent.logical_length);
        u=p+4096U+0xc00U+slot*64U;
        {unsigned int k;for(k=0U;k<64U;++k)u[k]=0U;}
        u[17U]=0x10U;u[18U]=0x20U;
        u[28U]=0xd7U;u[29U]=0xc4U;u[30U]=0xd6U;u[31U]=0xe2U;u[32U]=0xf0U;u[33U]=0xf0U;
    }
    /* Supervisor instruction veneers are never exposed to U. */
    p[0xb00U]=0x0aU;p[0xb01U]=250U;p[0xb02U]=0x07U;p[0xb03U]=0xfeU;
    p[0xb04U]=0x0aU;p[0xb05U]=251U;p[0xb06U]=0x07U;p[0xb07U]=0xfeU;
    p[0xb08U]=0x07U;p[0xb09U]=0xfeU;
    p[0xb10U]=0x41U;p[0xb11U]=0x10U;p[0xb12U]=0U;p[0xb13U]=0U;
    p[0xb14U]=0x41U;p[0xb15U]=0x10U;p[0xb16U]=0U;p[0xb17U]=0U;
    p[0xb18U]=0x07U;p[0xb19U]=0xfeU;
    p[0xb80U]=0x0aU;p[0xb81U]=246U;p[0xb82U]=0x07U;p[0xb83U]=0xfeU;
    p[0xb40U]=0x0aU;p[0xb41U]=252U;p[0xb42U]=0x07U;p[0xb43U]=0xfeU;
    /* The frozen native AOPEN calls CVT+232, entry+12, for TRKCALC.
     * Preserve the qualified one-space conservative result: R0=3, R15=0.
     * This compatibility helper is unprivileged and touches no K resource. */
    cms_put_word(p+0x500U,0x500U);cms_put_word(p+0x5e8U,0xb60U);
    for(i=0U;i<12U;++i)p[0xb60U+i]=0U;
    p[0xb6cU]=0x41U;p[0xb6dU]=0U;p[0xb6eU]=0U;p[0xb6fU]=3U;
    p[0xb70U]=0x41U;p[0xb71U]=0xf0U;p[0xb72U]=0U;p[0xb73U]=0U;
    p[0xb74U]=0x07U;p[0xb75U]=0xfeU;
}
static int sf_cleanup(unsigned int handle,const TSVFRAME *frame)
{
    unsigned int i;
    if(handle>SF_DD_HANDLE&&handle<=SF_DD_HANDLE+TSV_MAX_DEPTH*SFDDS){
        i=handle-SF_DD_HANDLE-1U;if(sf_dd[i].token!=frame->token)return 1;
        sf_dd[i].token=0U;return 0;
    }
    if(handle>SF_FILE_HANDLE&&handle<=SF_FILE_HANDLE+TSV_MAX_DEPTH*SFFILES){
        i=handle-SF_FILE_HANDLE-1U;if(sf_file[i].token!=frame->token)return 1;
        if(sf_file[i].real&&TSRRELEASE(&storage.real,SF_REAL_OWNER+i,
                                      sf_file[i].real)!=TSR_OK)return 1;
        sf_file[i].token=sf_file[i].real=0U;
        {unsigned int depth=(unsigned int)(frame-native_invocations.frame);
         if(depth<TSV_MAX_DEPTH&&sf_exit_token[depth]==frame->token)sf_exit_token[depth]=0U;}
        return 0;
    }
    return 1;
}
static unsigned int sf_dynalloc(void)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);
    volatile unsigned int *saved=(volatile unsigned int *)0x3000U;
    unsigned char ptr[4],rb[20],tu[50],dd[8],dsn[44],back[8];
    unsigned int address,rbaddr,list,at,n,key,raw,ret=0U,have=0U,dsbytes=0U,disp=0U;
    unsigned int i,slot,handle;SFDD *binding;TSKEXTENT extent;unsigned char dscb[96];
    if(!f||sf_span(saved[3]&0x7fffffffU,4U,0U,ptr,1U))return 12U;
    rbaddr=cms_word(ptr)&0x7fffffffU;
    if(sf_span(rbaddr,20U,0U,rb,1U)||rb[0]!=20U||
       sf_span(rbaddr+4U,4U,1U,0,0U))return 12U;
    list=cms_word(rb+8U)&0x7fffffffU;
    for(i=0U;i<8U;++i)dd[i]=0x40U;
    for(i=0U;i<16U;++i){
        if(sf_span(list+4U*i,4U,0U,ptr,1U))goto fail;
        raw=cms_word(ptr);address=raw&0x7fffffffU;
        if(sf_span(address,6U,0U,tu,1U))goto fail;
        key=sf_half(tu);n=sf_half(tu+4U);
        if(key==0x1cU&&sf_half(tu+2U)==0U){if(raw&0x80000000U)break;continue;}
        if(sf_half(tu+2U)!=1U||!n||n>44U||sf_span(address+6U,n,0U,tu+6U,1U))goto fail;
        if(key==1U&&n==8U){for(at=0U;at<8U;++at)dd[at]=tu[6U+at];have=1U;}
        else if(key==2U){dsbytes=n;for(at=0U;at<n;++at)dsn[at]=tu[6U+at];}
        else if(key==0x55U&&n==8U){ret=address+6U;if(sf_span(ret,8U,1U,0,0U))goto fail;}
        else if(key==4U&&n==1U)disp=tu[6U];
        else if(key!=0x49U&&key!=0x42U)goto fail;
        if(raw&0x80000000U)break;
    }
    if(i==16U||!(raw&0x80000000U))goto fail;
    if(rb[1U]==2U&&have){
        binding=sf_find_dd(dd);if(!binding)goto fail;
        slot=(unsigned int)(binding-sf_dd);
        for(i=0U;i<TSV_MAX_DEPTH*SFFILES;++i)
            if(sf_file[i].token==f->token&&sf_file[i].dd==slot)goto fail;
        handle=SF_DD_HANDLE+slot+1U;
        if(TSVFORGET(&native_invocations,f->token,TSV_FILE,handle)!=TSV_OK)goto fail;
        binding->token=0U;sf_compat();goto success;
    }
    if(rb[1U]!=1U||!dsbytes||disp!=8U)goto fail;
    if(!have&&ret){
        if(sf_next_dd>=999999U)goto fail;++sf_next_dd;n=sf_next_dd;
        dd[0U]=0xd7U;dd[1U]=0xc4U;
        for(i=8U;i>2U;){--i;dd[i]=(unsigned char)(0xf0U+n%10U);n/=10U;}have=1U;
    }
    if(!have||sf_find_dd(dd)||TSKDSCB(channel_record,&channel,dsn,dsbytes,&extent,dscb)!=TSK_OK)
        goto fail;
    for(slot=(native_invocations.depth-1U)*SFDDS;
        slot<native_invocations.depth*SFDDS&&sf_dd[slot].token;++slot){}
    if(slot==native_invocations.depth*SFDDS)goto fail;
    handle=SF_DD_HANDLE+slot+1U;
    if(TSVOWN(&native_invocations,f->token,TSV_FILE,handle)!=TSV_OK)goto fail;
    binding=&sf_dd[slot];binding->token=f->token;binding->bytes=dsbytes;
    for(i=0U;i<8U;++i)binding->dd[i]=dd[i];
    for(i=0U;i<dsbytes;++i)binding->dsn[i]=dsn[i];
    for(i=0U;i<96U;++i)binding->dscb[i]=dscb[i];binding->extent=extent;
    sf_compat();if(ret&&sf_span(ret,8U,1U,dd,1U))return 12U;
success:
    for(i=0U;i<4U;++i)back[i]=0U;
    if(sf_span(rbaddr+4U,4U,1U,back,1U))return 12U;
    saved[0U]=saved[1U]=0U;return 0U;
fail:
    back[0U]=0U;back[1U]=12U;back[2U]=back[3U]=0U;
    sf_span(rbaddr+4U,4U,1U,back,1U);saved[0U]=0U;saved[1U]=12U;return 12U;
}

static unsigned int sf_dcb(unsigned int svc)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);volatile unsigned int *saved=(volatile unsigned int *)0x3000U;
    unsigned char list[4],dcb[88],exits[4],jfcb[176],extra[4];
    unsigned int va,mode,exlist,i,exittarget=0U,jfcbtarget=0U,slot,handle,n=0U;
    SFDD *dd;SFFILE *file;
    if(!f||sf_span(saved[3]&0x7fffffffU,4U,0U,list,1U))return 12U;
    va=cms_word(list)&0xffffffU;mode=list[0U];
    if(sf_span(va,88U,0U,dcb,1U)||sf_span(va,88U,1U,0,0U))return 12U;
    dd=sf_find_dd(dcb+40U);if(!dd)return 12U;
    file=sf_find_file(va);
    if(svc==20U){
        if(!file)return 12U;slot=(unsigned int)(file-sf_file);
        if(file->mode==0x8fU&&file->real){
            unsigned char *data=(unsigned char *)TSF_KAPERTURE_VA+file->real;
            if(store_attach()!=TSS_OK||TSSPUT(&native_store,0x54534fU,file->key,52U,
                                              data,file->length)!=TSS_OK)return 12U;
        }
        handle=SF_FILE_HANDLE+slot+1U;
        if(sf_cleanup(handle,f)||TSVFORGET(&native_invocations,f->token,TSV_FILE,handle)!=TSV_OK)
            return 12U;dcb[48U]&=0xefU;
        return sf_span(va,88U,1U,dcb,1U)?12U:0U;
    }
    /* Dataset attributes override the native template's U/default fields.
     * The selected F/256 PDS-directory request is an explicit exception. */
    if(svc==64U || !(mode==0x80U && dd->extent.organisation==0x0200U &&
       dcb[36U]==0x80U && sf_half(dcb+62U)==256U && sf_half(dcb+82U)==256U)) {
        dcb[36U]=(unsigned char)dd->extent.record_format;
        sf_put_half(dcb+62U,dd->extent.block_length);
        sf_put_half(dcb+82U,dd->extent.logical_length);
    }
    exlist=cms_word(dcb+36U)&0xffffffU;
    if(exlist){
        for(i=0U;i<8U;++i){
            if(sf_span(exlist+4U*i,4U,0U,exits,1U))return 12U;
            n=cms_word(exits)&0xffffffU;
            if((exits[0U]&0x7fU)==5U)exittarget=n;
            if((exits[0U]&0x7fU)==7U&&svc==64U){
                if(jfcbtarget||sf_span(n,176U,1U,0,0U))return 12U;
                jfcbtarget=n;
            }
            if(exits[0U]&0x80U)break;
        }
        if(i==8U)return 12U;
    }
    if(jfcbtarget){
        for(i=0U;i<176U;++i)jfcb[i]=0U;
        for(i=0U;i<44U;++i)jfcb[i]=i<dd->bytes?dd->dsn[i]:0x40U;
        sf_put_half(jfcb+98U,dd->extent.organisation);jfcb[100U]=(unsigned char)dd->extent.record_format;
        sf_put_half(jfcb+102U,dd->extent.block_length);sf_put_half(jfcb+104U,dd->extent.logical_length);
        if(sf_span(jfcbtarget,176U,1U,jfcb,1U))return 12U;
    }
    if(svc==64U)return sf_span(va,88U,1U,dcb,1U)?12U:0U;
    if(mode!=0x80U&&mode!=0x8fU&&mode!=0x83U)return 12U;
    if(file)return 12U;
    if(exittarget&&sf_span(exittarget,2U,0U,extra,1U))return 12U;
    for(slot=(native_invocations.depth-1U)*SFFILES;
        slot<native_invocations.depth*SFFILES&&sf_file[slot].token;++slot){}
    if(slot==native_invocations.depth*SFFILES)return 4U;
    file=&sf_file[slot];file->real=0U;
    if(mode==0x8fU && TSRALLOC(&storage.real,SF_REAL_OWNER+slot,TSS_LIMIT,
                                TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,&file->real)!=TSR_OK)return 4U;
    handle=SF_FILE_HANDLE+slot+1U;
    if(TSVOWN(&native_invocations,f->token,TSV_FILE,handle)!=TSV_OK){
        if(file->real)TSRRELEASE(&storage.real,SF_REAL_OWNER+slot,file->real);file->real=0U;return 4U;}
    file->token=f->token;file->dcb=va;file->dd=(unsigned int)(dd-sf_dd);file->mode=mode;
    file->length=file->cursor=file->member=file->loaded=file->ecb=0U;
    file->cylinder=dd->extent.start_cylinder;file->head=dd->extent.start_head;file->record=0U;
    for(i=0U;i<52U;++i)file->key[i]=i<dd->bytes?dd->dsn[i]:0x40U;
    if(dd->extent.organisation==0x0200U&&mode==0x8fU){
        if(sf_span((saved[3]&0x7fffffffU)+0x90U,8U,0U,file->key+44U,1U)||
           file->key[44U]==0x40U){sf_cleanup(handle,f);TSVFORGET(&native_invocations,f->token,TSV_FILE,handle);return 12U;}
        file->member=1U;
    }
    sf_compat();cms_put_word(dcb+48U,mode==0x8fU?0x10000b00U:0x10000b04U);
    cms_put_word(dcb+52U,0xb08U);cms_put_word(dcb+84U,0xb10U);
    if(sf_span(va,88U,1U,dcb,1U))return 12U;
    if(exittarget){
        unsigned int depth=native_invocations.depth-1U;
        if(sf_exit_token[depth])return 12U;
        for(i=0U;i<128U;++i)sf_exit_saved[depth][i]=saved[i];
        sf_exit_token[depth]=f->token;saved[3U]=va;saved[29U]=0xb40U;
        saved[31U]=exittarget;saved[0x88U/4U]=0U;saved[0x8cU/4U]=exittarget;
        saved[0x80U/4U]&=~1U;saved[0x84U/4U]&=0x7fffffffU;
        *(volatile unsigned int *)0x4f54U=1U;
    }
    return 0U;
}
static unsigned int sf_exit_finish(void)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);unsigned int depth,i;
    volatile unsigned int *saved=(volatile unsigned int *)0x3000U;
    if(!f)return 12U;depth=native_invocations.depth-1U;
    if(sf_exit_token[depth]!=f->token||saved[0x8cU/4U]!=0xb42U)return 12U;
    for(i=0U;i<128U;++i)saved[i]=sf_exit_saved[depth][i];
    sf_exit_token[depth]=0U;saved[30U]=saved[31U]=0U;
    *(volatile unsigned int *)0x4f54U=1U;return 0U;
}

static int sf_record(SFFILE *file,unsigned int capacity,unsigned int *got)
{
    SFDD *dd=&sf_dd[file->dd];unsigned int subchannel=*(volatile const unsigned int *)0x40bcU;
    unsigned int attempt;
    for(attempt=0U;attempt<2U;++attempt){
        if(++file->record>255U){file->record=1U;if(++file->head==15U){file->head=0U;++file->cylinder;}}
        if(!TSKWITHIN(&dd->extent,file->cylinder,file->head))return 1;
        if(TSCBUILDREAD(&channel,file->cylinder,file->head,file->record,
                        dd->extent.organisation==0x0200U?0x06U:0x0eU,capacity)!=TSC_OK)return -1;
        if(TSCIO(subchannel,TSCORB(&channel),TSCIRB(&channel))==0){
            if(TSCCHECKREAD(&channel,capacity,got)==TSC_OK)return *got?0:1;
            if(TSCCHECKEND(&channel,capacity)==TSC_OK){*got=0U;return 1;}
        }
        /* A missing next CKD record ends this track. The extent still bounds
         * the following read. No elapsed-time success or fabricated record. */
        file->record=0U;if(++file->head==15U){file->head=0U;++file->cylinder;}
    }
    return -1;
}
static unsigned int sf_io(unsigned int write)
{
    const TSVFRAME *frame=TSVTOP(&native_invocations);
    volatile unsigned int *saved=(volatile unsigned int *)0x3000U;
    unsigned char decb[20],dcb[88],iob[16];unsigned char *data;
    unsigned int address,va,buffer,length,slot,got,i,n,status,iobva;
    SFFILE *file;int rc;
    if(!frame)return 12U;address=saved[3U]&0x7fffffffU;
    if(sf_span(address,20U,0U,decb,1U)||sf_span(address,20U,1U,0,0U))return 12U;
    va=cms_word(decb+8U)&0x7fffffffU;buffer=cms_word(decb+12U)&0x7fffffffU;
    length=sf_half(decb+6U);file=sf_find_file(va);
    if(!file||!length||length>TSC_MAX_RECORD||
       sf_span(va,88U,0U,dcb,1U)||sf_span(buffer,length,write?0U:1U,0,0U))return 12U;
    slot=(unsigned int)(file-sf_file);iobva=0x10a00U+(slot%SFFILES)*32U;
    if(sf_span(iobva,16U,1U,0,0U))return 12U;
    if(write){
        if(file->mode!=0x8fU&&file->mode!=0x83U)return 12U;
        if(!file->real||file->length>TSS_LIMIT-length-2U)return 12U;
        data=(unsigned char *)TSF_KAPERTURE_VA+file->real;
        sf_put_half(data+file->length,length);
        if(sf_span(buffer,length,0U,data+file->length+2U,1U))return 12U;
        file->length+=length+2U;got=length;
    }else{
        if(file->mode!=0x80U&&file->mode!=0x83U)return 12U;
        if(!file->loaded&&!file->real){
            status=(unsigned int)store_attach();
            if((int)status==TSS_OK){
                rc=TSSSIZE(&native_store,0x54534fU,file->key,52U,&n);
                if(rc==TSS_DELETED)return 12U;
                if(rc==TSS_OK){
                    if(n>TSS_LIMIT)return 12U;
                    file->loaded=1U;file->length=0U;
                    if(n){
                        if(TSRALLOC(&storage.real,SF_REAL_OWNER+slot,
                             (n+4095U)&~4095U,TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,&file->real)!=TSR_OK)return 4U;
                        data=(unsigned char *)TSF_KAPERTURE_VA+file->real;
                        if(TSSREAD(&native_store,0x54534fU,file->key,52U,data,n,&file->length)!=TSS_OK)return 12U;
                    }
                }else if(rc!=TSS_ABSENT)return 12U;
            }else if((int)status!=TSS_ABSENT)return 12U;
        }
        if(file->real||file->loaded==1U){
            data=(unsigned char *)TSF_KAPERTURE_VA+file->real;
            if(file->cursor==file->length)got=0U;
            else{
                if(file->cursor>file->length-2U)return 12U;
                got=sf_half(data+file->cursor);
                if(!got||got>length||got>file->length-file->cursor-2U)return 12U;
                data+=file->cursor+2U;
                if(sf_span(buffer,got,1U,data,1U))return 12U;
                file->cursor+=got+2U;
            }
        }else{
            rc=sf_record(file,length,&got);if(rc<0)return 12U;
            if(rc>0)got=0U;
            if(got&&sf_span(buffer,got,1U,TSCDATA(&channel),1U))return 12U;
            ++file->cursor;
        }
        if(!got){
            va=cms_word(dcb+32U)&0xffffffU;
            if(!va||sf_span(va,2U,0U,iob,1U))return 12U;
            saved[0x88U/4U]=0U;saved[0x8cU/4U]=va;
            *(volatile unsigned int *)0x4f54U=1U;return 0U;
        }
    }
    for(i=0U;i<16U;++i)iob[i]=0U;iob[12U]=0x0cU;sf_put_half(iob+14U,length-got);
    if(sf_span(iobva,16U,1U,iob,1U))return 12U;
    cms_put_word(decb,0x40000000U);cms_put_word(decb+16U,iobva);
    if(sf_span(address,20U,1U,decb,1U))return 12U;
    file->ecb=address;return 0U;
}

/* Native CHECK reaches WAIT after the synchronous, event-verified read/write.
 * Only this invocation's last posted file ECB can satisfy that request. */
static unsigned int sf_wait(void)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);
    volatile unsigned int *saved=(volatile unsigned int *)0x3000U;
    unsigned char ecb[4];unsigned int i,address=saved[3U]&0x7fffffffU;
    if(!f||saved[1U]!=1U||sf_span(address,4U,0U,ecb,1U))return 8U;
    for(i=0U;i<TSV_MAX_DEPTH*SFFILES;++i)
        if(sf_file[i].token==f->token&&sf_file[i].ecb==address)
            return (cms_word(ecb)&0x40000000U)?0U:16U;
    return 8U;
}

static int sf_member(SFDD *dd,const unsigned char member[8],unsigned char ttr[3])
{
    unsigned int track,record,cylinder,head,count,used,at,size,i;
    unsigned int subchannel=*(volatile const unsigned int *)0x40bcU;
    unsigned char *block;
    if(dd->extent.organisation!=0x0200U)return -1;
    for(track=0U;track<128U;++track){
        cylinder=dd->extent.start_cylinder+(dd->extent.start_head+track)/15U;
        head=(dd->extent.start_head+track)%15U;
        if(!TSKWITHIN(&dd->extent,cylinder,head))return -1;
        for(record=1U;record<100U;++record){
            if(TSCBUILDREAD(&channel,cylinder,head,record,0x06U,256U)!=TSC_OK)return -1;
            if(TSCIO(subchannel,TSCORB(&channel),TSCIRB(&channel))||
               TSCCHECKREAD(&channel,256U,&count)!=TSC_OK)break;
            if(count!=256U)return -1;block=TSCDATA(&channel);used=sf_half(block);
            if(used<2U||used>256U)return -1;
            for(at=2U;at<used;at+=size){
                if(used-at<8U)return -1;
                for(i=0U;i<8U&&block[at+i]==0xffU;++i){}if(i==8U)return 0;
                if(used-at<12U)return -1;
                if(sf_equal(block+at,member,8U)){
                    for(i=0U;i<3U;++i)ttr[i]=block[at+8U+i];
                    if(!ttr[2U]||sf_half(ttr)>=128U)return -1;return 1;
                }
                size=12U+2U*(block[at+11U]&31U);if(size>used-at)return -1;
            }
        }
    }
    return -1;
}
static unsigned int sf_lookup(unsigned int svc)
{
    volatile unsigned int *saved=(volatile unsigned int *)0x3000U;
    unsigned char bytes[96],name[44],dcb[88],ttr[3],cam[16],old_member[8];
    unsigned int r0=saved[1U],r1=saved[3U],i,n,dcba,type_d,track;
    SFDD *dd;SFFILE *file;TSKEXTENT extent;int rc;
    if(svc==24U){
        if(sf_span(r1&0x7fffffffU,8U,0U,name,1U)||sf_span(r0&0x7fffffffU,8U,1U,0,0U))return 8U;
        dd=sf_find_dd(name);if(!dd)return 8U;
        bytes[0U]=0x30U;bytes[1U]=0x50U;bytes[2U]=0x20U;bytes[3U]=0x0bU;
        bytes[4U]=bytes[5U]=0U;bytes[6U]=0x7fU;bytes[7U]=0xffU;
        return sf_span(r0&0x7fffffffU,8U,1U,bytes,1U)?8U:0U;
    }
    if(svc==27U){
        if(sf_span(r1&0x7fffffffU,16U,0U,cam,1U)||cms_word(cam)!=0xc1000000U)return 8U;
        if(sf_span(cms_word(cam+4U)&0x7fffffffU,44U,0U,name,1U)||
           sf_span(cms_word(cam+8U)&0x7fffffffU,6U,0U,bytes,1U)||
           !sf_equal(bytes,(const unsigned char *)"\xd7\xc4\xd6\xe2\xf0\xf0",6U))return 8U;
        n=44U;while(n&&name[n-1U]==0x40U)--n;
        if(TSKDSCB(channel_record,&channel,name,n,&extent,bytes)!=TSK_OK)return 8U;
        return sf_span(cms_word(cam+12U)&0x7fffffffU,96U,1U,bytes,1U)?8U:0U;
    }
    type_d=(r1&0x80000000U)!=0U;dcba=type_d?0U-r1:r1;dcba&=0x7fffffffU;
    if(sf_span(dcba,88U,0U,dcb,1U))return 8U;
    dd=sf_find_dd(dcb+40U);file=sf_find_file(dcba);if(!dd||!file)return 8U;
    if(sf_span(r0&0x7fffffffU,type_d?8U:16U,0U,name,1U)||
       (!type_d&&(sf_half(name)!=1U||sf_half(name+2U)<16U)))return 8U;
    if(!type_d&&sf_span(r0&0x7fffffffU,16U,1U,0,0U))return 8U;
    for(i=0U;i<8U;++i){old_member[i]=file->key[44U+i];file->key[44U+i]=name[(type_d?0U:4U)+i];}
    /* A committed journal member takes precedence over its original PDS TTR. */
    rc=store_attach();
    if(rc==TSS_OK)rc=TSSSIZE(&native_store,0x54534fU,file->key,52U,&n);
    if(rc==TSS_OK){ttr[0U]=0U;ttr[1U]=0U;ttr[2U]=1U;rc=1;}
    else if(rc==TSS_DELETED)rc=0;
    else if(rc==TSS_ABSENT)rc=sf_member(dd,file->key+44U,ttr);
    else rc=-1;
    if(rc!=1){for(i=0U;i<8U;++i)file->key[44U+i]=old_member[i];
        saved[0U]=0U;saved[1U]=16U;return rc==0?4U:8U;}
    if(type_d){
        track=sf_half(ttr);file->cylinder=dd->extent.start_cylinder+(dd->extent.start_head+track)/15U;
        file->head=(dd->extent.start_head+track)%15U;file->record=ttr[2U]-1U;
        file->cursor=0U;file->member=1U;
    }else{
        for(i=0U;i<3U;++i)name[12U+i]=ttr[i];name[15U]=0U;
        if(sf_span(r0&0x7fffffffU,16U,1U,name,1U))return 8U;
    }
    saved[0U]=saved[1U]=0U;return 0U;
}
static unsigned int sf_dispatch(unsigned int svc)
{
    /* These are the selected AMODE24/31 native lists. LP64 bridges enter
     * that mode before invoking them; a direct AMODE64 list is unsupported. */
    if(*(volatile unsigned int *)0x3080U&1U)return 12U;
    if(svc==1U)return sf_wait();
    if(svc==99U)return sf_dynalloc();
    if(svc==64U||svc==22U||svc==20U)return sf_dcb(svc);
    if(svc==250U||svc==251U)return sf_io(svc==250U);
    if(svc==18U||svc==24U||svc==27U)return sf_lookup(svc);
    if(svc==252U)return sf_exit_finish();
    return 12U;
}
/* Bounded diagnostic receipt for the selected native forms. It records
 * completion status, not a second service path or application-visible ABI. */
static unsigned int sf_service(unsigned int svc)
{
    volatile unsigned int *saved=(volatile unsigned int *)0x3000U;
    volatile unsigned int *receipt=(volatile unsigned int *)(TSF_KAPERTURE_VA+0x30200U);
    unsigned int count=receipt[0U],address=saved[3U],result;
    unsigned int code=saved[0x8cU/4U],r0=saved[1U],flags=saved[31U];
    result=sf_dispatch(svc);
    if(count<128U){
        volatile unsigned int *row=receipt+8U+count*8U;
        row[0U]=svc;row[1U]=code;row[2U]=address;row[3U]=r0;
        row[4U]=flags;row[5U]=result;
        {unsigned char dcb[88];unsigned int va=address;
         if(svc==64U||svc==22U||svc==20U){unsigned char list[4];
            if(sf_span(address&0x7fffffffU,4U,0U,list,1U)==0U)va=cms_word(list)&0xffffffU;}
         if((svc==64U||svc==22U||svc==20U)&&sf_span(va,88U,0U,dcb,1U)==0U){
            row[6U]=cms_word(dcb+36U);row[7U]=(sf_half(dcb+62U)<<16)|sf_half(dcb+82U);
         }else{row[6U]=row[7U]=0U;}}
        receipt[0U]=count+1U;
    }
    return result;
}

/* Native line reads wait for real attention/data events in K. Preserve an
 * empty Enter and exact EBCDIC bytes; no whitespace trimming is done here. */
static unsigned int native_line_input(unsigned int address,unsigned int capacity,
                                       unsigned int *count)
{
    TSGREQUEST request;unsigned char raw[256],text[256];unsigned int rc,n,i;
    if(!count||!capacity||capacity>256U||sf_span(address,capacity,1U,0,0U))return 8U;
    request.address.hi=request.address.lo=request.length=request.direction=0U;
    request.svc=209U;rc=terminal_read_start(&request);if(rc)return rc;
    request.length=256U;request.direction=TSG_WRITE;
    for(;;){
        rc=terminal_read_poll_into(&request,raw);
        if(rc!=1U)break;
        if(TSCWAITR(console_ssid,TSCIRB(&console_channel))!=0){
            terminal_read_cancel(console_read_owner);return 12U;}
        console_irb_ready=1U;
    }
    if(rc)return rc;n=cms_word(raw);
    if(n==3U&&raw[4U]==0x7dU)n=0U;
    else if(n>=6U&&raw[7U]==0x11U)n-=6U;
    else return 8U;
    if(n>capacity)return 8U;
    for(i=0U;i<capacity;++i)text[i]=i<n?raw[10U+i]:0x40U;
    if(sf_span(address,capacity,1U,text,1U))return 8U;
    *count=n;return 0U;
}

/* These two selectors are diagnostic receipts, not a native application ABI.
 * Keep the earlier version proof separate from the full unchanged file run. */
static unsigned int sf_probe_receipt(unsigned int finish)
{
    volatile unsigned char *version=(volatile unsigned char *)0x4700U;
    volatile unsigned char *kept=(volatile unsigned char *)(TSF_KAPERTURE_VA+0x30000U);
    unsigned int i;
    if(TSVTOP(&native_invocations))return 8U;
    if(!finish){
        *(volatile unsigned int *)(TSF_KAPERTURE_VA+0x31400U)=1U;
        *(volatile unsigned int *)(TSF_KAPERTURE_VA+0x31404U)=0U;
        *(volatile unsigned int *)(TSF_KAPERTURE_VA+0x31408U)=0U;
        unsigned char *image=(unsigned char *)TSF_KAPERTURE_VA+
            *(volatile unsigned int *)0x4600U;
        volatile unsigned char *sample=(volatile unsigned char *)(TSF_KAPERTURE_VA+0x32000U);
        for(i=0U;i<256U;++i)kept[i]=version[i];
        for(i=0U;i<0x390U;++i)sample[i]=image[i];
        *(volatile unsigned int *)(sample+0x390U)=0x50325631U;
    }
    else{
        *(volatile unsigned int *)(TSF_KAPERTURE_VA+0x12698U)=*(volatile unsigned int *)(TSF_KAPERTURE_VA+0x31404U);
        *(volatile unsigned int *)(TSF_KAPERTURE_VA+0x1269cU)=*(volatile unsigned int *)(TSF_KAPERTURE_VA+0x31408U);
        *(volatile unsigned int *)(TSF_KAPERTURE_VA+0x31400U)=0U;
        for(i=0U;i<256U;++i)kept[256U+i]=version[i];
        for(i=0U;i<256U;++i)version[i]=kept[i];
    }
    return 0U;
}
