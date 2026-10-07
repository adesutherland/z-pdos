/* SPDX-License-Identifier: MIT
 * Native low-launcher LOAD/DELETE for checked RMODE64 AL8 bodies. Each
 * invocation owns its own mapping; distinct launchers may coexist. */
#define TSL_HIGH_WORK 0x544c4801U
typedef struct {
    unsigned char name[8];
    unsigned int token,task,handle,bytes;
    TSPADDR address;
} TSLHIGH;
static TSLHIGH high_images[TSV_MAX_DEPTH];

static int high_cleanup(unsigned int handle,const TSVFRAME *frame)
{
    unsigned int i;
    for(i=0U;i<TSV_MAX_DEPTH;++i){
        TSLHIGH *image=&high_images[i];
        if(image->handle!=handle||image->token!=frame->token)continue;
        if(image->task!=frame->runtime_owner||
           TSMFREE(&storage,image->task,image->address)!=TSM_OK)return 1;
        image->token=image->handle=image->bytes=0U;
        return 0;
    }
    return 1;
}

static unsigned int native_high_service(unsigned int svc)
{
    const TSVFRAME *frame=TSVTOP(&native_invocations);
    volatile unsigned int *saved=(volatile unsigned int *)0x3000U;
    unsigned char name[8],dsn[12],*stage,*image,*destination;
    unsigned int n,i,rc,length,stage_real=0U,work_real=0U,real=0U;
    unsigned int *relocations;
    TSPADDR minimum,maximum,address;
    TSTINFO info;TSLHIGH *record;
    {volatile unsigned int *trace=(volatile unsigned int *)(TSF_KAPERTURE_VA+0x3b000U);
     unsigned int slot=trace[0U];
     if(slot<32U){
         trace[1U+8U*slot]=svc;trace[2U+8U*slot]=saved[0U];trace[3U+8U*slot]=saved[1U];
         trace[4U+8U*slot]=saved[2U];trace[5U+8U*slot]=saved[3U];
         trace[6U+8U*slot]=saved[32U];trace[7U+8U*slot]=saved[33U];
         trace[8U+8U*slot]=frame?frame->token:0U;trace[0U]=slot+1U;
     }}
    if(!frame||frame->personality!=TSV_TSO||frame->amode!=64U||
       (saved[32U]&1U)||!(saved[33U]&0x80000000U)||(svc==8U&&saved[0U])||
       saved[1U]>=0x80000000U||sf_span(saved[1U],8U,0U,name,1U))return 8U;
    for(n=0U;n<8U&&name[n]!=0x40U;++n)
        if(!((name[n]>=0xc1U&&name[n]<=0xc9U)||
             (name[n]>=0xd1U&&name[n]<=0xd9U)||
             (name[n]>=0xe2U&&name[n]<=0xe9U)||
             (name[n]>=0xf0U&&name[n]<=0xf9U)))return 8U;
    if(!n)return 8U;
    for(i=n;i<8U;++i)if(name[i]!=0x40U)return 8U;
    record=&high_images[native_invocations.depth-1U];
    if(svc==9U){
        if(record->token!=frame->token||!record->handle)return 4U;
        for(i=0U;i<8U;++i)if(record->name[i]!=name[i])return 4U;
        i=record->handle;
        if(TSVHAS(&native_invocations,frame->token,TSV_HIGH_IMAGE,i)!=TSV_OK||
           high_cleanup(i,frame)||
           TSVFORGET(&native_invocations,frame->token,TSV_HIGH_IMAGE,i)!=TSV_OK)return 12U;
        ++*(volatile unsigned int *)0x4f84U;
        return 0U;
    }
    if(record->handle||saved[2U]||(saved[3U]&0x7fffffffU))return 8U;
    for(i=0U;i<n;++i)dsn[i]=name[i];
    dsn[n++]=0x4bU;dsn[n++]=0xc5U;dsn[n++]=0xe7U;dsn[n++]=0xc5U;
    rc=load_tso_stage(dsn,n,64U,1U,&stage,&stage_real,&info,&length);
    if(rc)return 8U;
    if(TSRALLOC(&storage.real,TSL_HIGH_WORK,TST_MAX_IMAGE+4U*TST_MAX_HIGH_RELOCS,
                 TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,&work_real)!=TSR_OK){rc=8U;goto done;}
    image=(unsigned char *)TSF_KAPERTURE_VA+work_real;
    relocations=(unsigned int *)(image+TST_MAX_IMAGE);
    minimum.hi=maximum.hi=1U;minimum.lo=0x30000000U;maximum.lo=0xffffffffU;
    /* First validate the complete body in K, then choose only its measured
     * size. No U page is visible until the final AL8 relocation succeeds. */
    if(TSTIMAGE64HIGH(stage+64U,info.raw_bytes,minimum,image,TST_MAX_IMAGE,
                      relocations,TST_MAX_HIGH_RELOCS,&info)!=TST_OK||
       TSPFIND(&storage.virtuals,64U,minimum,maximum,
               (info.image_bytes+4095U)&~4095U,&address)!=TSP_OK||
       TSTIMAGE64HIGH(stage+64U,info.raw_bytes,address,image,TST_MAX_IMAGE,
                      relocations,TST_MAX_HIGH_RELOCS,&info)!=TST_OK){rc=8U;goto done;}
    if(TSMALLOC(&storage,frame->runtime_owner,64U,address,maximum,
                 info.image_bytes,1,&address)!=TSM_OK){rc=8U;goto done;}
    for(i=0U;i<TSM_ALLOCS;++i)
        if(storage.allocations[i].handle&&storage.allocations[i].task==frame->runtime_owner&&
           storage.allocations[i].address.hi==address.hi&&storage.allocations[i].address.lo==address.lo){
            real=storage.allocations[i].real;record->handle=storage.allocations[i].handle;break;
        }
    if(!real||TSVOWN(&native_invocations,frame->token,TSV_HIGH_IMAGE,record->handle)!=TSV_OK){
        TSMFREE(&storage,frame->runtime_owner,address);record->handle=0U;rc=8U;goto done;
    }
    destination=(unsigned char *)TSF_KAPERTURE_VA+real;
    for(i=0U;i<info.image_bytes;++i)destination[i]=image[i];
    for(i=0U;i<8U;++i)record->name[i]=name[i];
    record->token=frame->token;record->task=frame->runtime_owner;
    record->bytes=info.image_bytes;record->address=address;
    saved[0U]=address.hi;saved[1U]=(address.lo+info.entry_offset)|1U;
    saved[2U]=0U;saved[3U]=(info.image_bytes+7U)/8U;
    saved[30U]=saved[31U]=0U;
    *(volatile unsigned int *)0x4f54U=1U;
    ++*(volatile unsigned int *)0x4f80U;
    *(volatile unsigned int *)0x4f88U=address.hi;
    *(volatile unsigned int *)0x4f8cU=address.lo;
    rc=0U;
done:
    if(work_real&&TSRRELEASE(&storage.real,TSL_HIGH_WORK,work_real)!=TSR_OK)rc=12U;
    if(stage_real&&TSRRELEASE(&storage.real,TSL_STAGE_OWNER,stage_real)!=TSR_OK)rc=12U;
    return rc;
}
