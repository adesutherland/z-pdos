/* SPDX-License-Identifier: MIT
 * Checked native image registry and placement. Invocations lease these K
 * records; no caller PC or U-provided placement selects an image identity. */
#define TSL_IMAGES 32U
#define TSL_STAGE_OWNER 0x544c5301U
#define TSL_IMAGE_OWNER 0x544c5302U
#define TSL_ABSENT 28U
typedef struct {
    unsigned char name[8];
    unsigned int personality,mode,owner,runtime,bytes,entry;
    TSPADDR address;
    unsigned int mapped,staged_real,overlay,overlay_task;
} TSLIMAGE;
static TSLIMAGE native_images[TSL_IMAGES];

static void load_snapshot(void)
{
    volatile unsigned int *out=(volatile unsigned int *)(TSF_KAPERTURE_VA+0x35000U);
    unsigned int i,j,n=0U,live=0U,real;
    for(i=0U;i<TSM_ALLOCS;++i)
        if(storage.allocations[i].handle&&storage.allocations[i].task>=256U)++live;
    out[0U]=0x4c4f4144U;out[1U]=live;out[2U]=TSMLOWFREE(&storage);out[3U]=storage.depth;
    for(i=0U;i<TSL_IMAGES;++i){
        const TSLIMAGE *image=&native_images[i];
        if(!image->bytes||!image->mapped)continue;
        real=0U;
        for(j=0U;j<TSM_ALLOCS;++j)if(storage.allocations[j].handle&&
           storage.allocations[j].task==image->owner&&
           storage.allocations[j].address.hi==image->address.hi&&
           storage.allocations[j].address.lo==image->address.lo)real=storage.allocations[j].real;
        out[8U+n*8U]=image->owner;out[9U+n*8U]=image->mode;
        out[10U+n*8U]=image->address.hi;out[11U+n*8U]=image->address.lo;
        out[12U+n*8U]=image->bytes;out[13U+n*8U]=real;
        out[14U+n*8U]=native_image_active[image->owner];out[15U+n*8U]=image->entry;
        for(j=0U;j<8U;++j)
            ((volatile unsigned char *)out)[0x800U+8U*n+j]=image->name[j];
        ++n;
    }
    out[4U]=n;
    n=0U;
    for(i=0U;i<TSI_SLOTS;++i)if(cms_inputs.slot[i].real)++n;
    out[5U]=n;n=0U;
    for(i=0U;i<CMS_OUTPUT_SLOTS;++i)if(cms_outputs[i].real&&!cms_outputs[i].closed)++n;
    out[6U]=n;
}

static int load_name(const unsigned char name[8],unsigned char *dsn,
                       unsigned int personality,unsigned int mode,unsigned int *bytes)
{
    unsigned int i,n=0U;
    static const unsigned char cms[3]={0xc3U,0xd4U,0xe2U};
    static const unsigned char tso[3]={0xe3U,0xe2U,0xd6U};
    for(i=0U;i<3U;++i)dsn[n++]=(personality==TSV_CMS?cms:tso)[i];
    dsn[n++]=mode==24U?0xf2U:mode==31U?0xf3U:0xf6U;
    dsn[n++]=mode==24U?0xf4U:mode==31U?0xf1U:0xf4U;dsn[n++]=0x4bU;
    for(i=0U;i<8U&&name[i]!=0x40U;++i){
        if(!name[i]||name[i]==0x4bU||name[i]==0x61U)return 0;
        dsn[n++]=name[i];
    }
    if(!i)return 0;
    while(i<8U)if(name[i++]!=0x40U)return 0;
    *bytes=n;return 1;
}
static TSLIMAGE *load_find(unsigned int personality,unsigned int mode,const unsigned char name[8])
{
    unsigned int i,j;
    for(i=0U;i<TSL_IMAGES;++i){
        TSLIMAGE *image=&native_images[i];
        if(!image->bytes||image->personality!=personality||image->mode!=mode)continue;
        for(j=0U;j<8U&&image->name[j]==name[j];++j){}
        if(j==8U)return image;
    }
    return 0;
}
static int load_reserved_owner(unsigned int owner)
{return owner==4U||owner==5U||owner==8U||owner==15U||owner==17U||owner==18U||owner==20U||owner==22U;}

static unsigned int load_tso_stage(const unsigned char *name,unsigned int name_bytes,
                                    unsigned int mode,unsigned int high,unsigned char **stage,
                                    unsigned int *real,TSTINFO *info,unsigned int *length)
{
    TSKEXTENT extent;const unsigned char *record;
    unsigned int bytes,blocks,span,cylinder,head,number,i,j;int count,rc;
    rc=TSKFIND(channel_record,&channel,name,name_bytes,&extent);
    if(rc==TSK_ABSENT)return 4U;
    if(rc!=TSK_OK||extent.record_format!=0x80U||extent.block_length!=TST_BLOCK||
       extent.logical_length!=TST_BLOCK)return 12U;
    cylinder=extent.start_cylinder;head=extent.start_head;number=1U;
    count=channel_record(&channel,cylinder,head,number,TST_BLOCK,&record);
    if(count!=(int)TST_BLOCK||(mode==24U?TSTSTAGEHEADER24(record,TST_BLOCK,&bytes,&blocks):
       mode==31U?TSTSTAGEHEADER(record,TST_BLOCK,&bytes,&blocks):
       TSTSTAGEHEADER64(record,TST_BLOCK,&bytes,&blocks))!=TST_OK)return 12U;
    span=(blocks*TST_BLOCK+4095U)&~4095U;
    if(TSRALLOC(&storage.real,TSL_STAGE_OWNER,span,TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,real)!=TSR_OK)return 4U;
    *stage=(unsigned char *)TSF_KAPERTURE_VA+*real;
    for(j=0U;j<blocks;++j){
        if(!TSKWITHIN(&extent,cylinder,head))goto bad;
        if(j){count=channel_record(&channel,cylinder,head,number,TST_BLOCK,&record);
            if(count!=(int)TST_BLOCK)goto bad;}
        for(i=0U;i<TST_BLOCK;++i)(*stage)[j*TST_BLOCK+i]=record[i];
        if(++number==4U){number=1U;if(++head==15U){head=0U;++cylinder;}}
    }
    if((high?TSTSTAGEHIGH(*stage,blocks*TST_BLOCK,info):
        TSTSTAGEVALIDATE(*stage,blocks*TST_BLOCK,mode,info))!=TST_OK||info->raw_bytes!=bytes)goto bad;
    *length=blocks*TST_BLOCK;return 0U;
bad:
    return TSRRELEASE(&storage.real,TSL_STAGE_OWNER,*real)==TSR_OK?12U:16U;
}

static unsigned int load_native(unsigned int personality,unsigned int mode,
                                 const unsigned char name[8],TSLIMAGE **loaded)
{
    unsigned char dsn[14],*stage=0,*image=0,*destination;
    unsigned int n,stage_real=0U,image_real=0U,blocks=0U,length=0U;
    unsigned int owner,i,entry=0U,bytes=0U,rc,span,real=0U,overlay=0U,overlay_task=0U;
    TSHINFO cms;TSTINFO tso;TSLIMAGE *record;
    TSKEXTENT extent;
    TSPADDR minimum,maximum,address;
    const TSVFRAME *parent=TSVTOP(&native_invocations);
    if(!loaded||(personality!=TSV_CMS&&personality!=TSV_TSO)||
       (mode!=24U&&mode!=31U&&mode!=64U)||!load_name(name,dsn,personality,mode,&n))return 8U;
    record=load_find(personality,mode,name);
    if(record){*loaded=record;return 0U;}
    rc=TSKFIND(channel_record,&channel,dsn,n,&extent);
    if(rc==TSK_ABSENT)return TSL_ABSENT;
    if(rc!=TSK_OK)return 12U;
    for(owner=1U;owner<TSL_IMAGES;++owner)
        if(!load_reserved_owner(owner)&&!native_images[owner].bytes)break;
    if(owner==TSL_IMAGES)return 4U;
    if(personality==TSV_CMS){
        if(mode==64U)return 8U;
        rc=cms_stage_read(dsn,n,mode,&stage,&stage_real,&cms,&blocks);
        if(rc)return rc;bytes=cms.image_bytes;length=blocks*18452U;
    }else{
        rc=load_tso_stage(dsn,n,mode,0U,&stage,&stage_real,&tso,&length);
        if(rc)return rc;bytes=TST_MAX_IMAGE;
    }
    span=(bytes+4095U)&~4095U;
    if(!bytes||TSRALLOC(&storage.real,TSL_IMAGE_OWNER,span,TSF_CORE_BYTES,
                       TSF_REAL_BYTES,TSR_RUN,&image_real)!=TSR_OK){rc=4U;goto done;}
    image=(unsigned char *)TSF_KAPERTURE_VA+image_real;
    if(personality==TSV_TSO){
        rc=(mode==24U?TSTIMAGE24(stage+64U,tso.raw_bytes,0x20000U,image,span,&tso):
            mode==31U?TSTIMAGE31(stage+64U,tso.raw_bytes,0x20000000U,image,span,&tso):
            TSTIMAGE64ANY(stage+64U,tso.raw_bytes,0x20000000U,image,span,&tso))==TST_OK?0U:12U;
        if(rc)goto done;
        bytes=tso.image_bytes;
    }
    minimum.hi=maximum.hi=0U;
    minimum.lo=personality==TSV_CMS&&(mode==24U||cms.format)?cms.origin:mode==24U?0x20000U:0x20000000U;
    maximum.lo=mode==24U?0xffffffU:0x7fffffffU;
    if(personality==TSV_CMS&&(mode==24U||cms.format)){
        TSPADDR translated;
        if(TSDLOOKUP(&u_tables,minimum,&translated)==TSD_OK){
            for(i=0U;i<TSM_ALLOCS;++i)if(storage.allocations[i].handle&&
                storage.allocations[i].address.hi==minimum.hi&&
                storage.allocations[i].address.lo==minimum.lo)
                overlay_task=storage.allocations[i].task;
            if(!overlay_task||overlay_task>=32U||
               (native_image_active[overlay_task]&&(!parent||parent->personality!=TSV_CMS||
                parent->amode!=mode||parent->image_owner!=overlay_task))){rc=4U;goto done;}
            overlay=1U;address=minimum;
        }
    }
    if(!overlay){
        if(personality==TSV_CMS&&(mode==24U||cms.format))address=minimum;
        else if(TSPFIND(&storage.virtuals,mode,minimum,maximum,
                        (bytes+4095U)&~4095U,&address)!=TSP_OK){rc=4U;goto done;}
    }
    if(personality==TSV_CMS){
        rc=TSHIMAGE(stage,length,mode,address.lo,image,span,&entry)==TSH_OK?0U:12U;
    }else{
        rc=(mode==24U?TSTIMAGE24(stage+64U,tso.raw_bytes,address.lo,image,span,&tso):
            mode==31U?TSTIMAGE31(stage+64U,tso.raw_bytes,address.lo,image,span,&tso):
            TSTIMAGE64ANY(stage+64U,tso.raw_bytes,address.lo,image,span,&tso))==TST_OK?0U:12U;
        entry=address.lo+tso.entry_offset;
    }
    if(rc)goto done;
    if(!overlay){
        rc=TSMALLOC(&storage,owner,mode,address,maximum,bytes,1,&address);
        if(rc!=TSM_OK){rc=4U;goto done;}
        for(i=0U;i<TSM_ALLOCS;++i)if(storage.allocations[i].handle&&
            storage.allocations[i].task==owner&&storage.allocations[i].address.lo==address.lo)
            real=storage.allocations[i].real;
        if(!real){TSMFREE(&storage,owner,address);rc=12U;goto done;}
        destination=(unsigned char *)TSF_KAPERTURE_VA+real;
        for(i=0U;i<bytes;++i)destination[i]=image[i];
    }
    record=&native_images[owner];for(i=0U;i<8U;++i)record->name[i]=name[i];
    record->personality=personality;record->mode=mode;record->owner=owner;
    record->runtime=0x100U+owner;record->bytes=bytes;record->entry=entry;
    record->address=address;record->mapped=!overlay;record->overlay=overlay;
    record->overlay_task=overlay_task;
    record->staged_real=overlay?image_real:0U;
    if(overlay)image_real=0U;
    *loaded=record;rc=0U;
done:
    if(image_real&&TSRRELEASE(&storage.real,TSL_IMAGE_OWNER,image_real)!=TSR_OK)rc=16U;
    if(stage_real&&TSRRELEASE(&storage.real,personality==TSV_CMS?CMS_STAGE_OWNER:TSL_STAGE_OWNER,
                              stage_real)!=TSR_OK)rc=16U;
    *(volatile unsigned int *)0x4098U=u_tables.used;
    return rc;
}

static unsigned int load_callable(unsigned int personality,unsigned int mode,
                                   const unsigned char name[8],TSLIMAGE **image)
{
    unsigned int rc;
    rc=load_native(personality,mode,name,image);
    if(rc!=TSL_ABSENT)return rc;
    if(mode!=31U&&personality==TSV_TSO){
        rc=load_native(personality,31U,name,image);
        if(rc!=TSL_ABSENT)return rc;
    }
    if(mode!=24U){
        rc=load_native(personality,24U,name,image);
        if(rc!=TSL_ABSENT)return rc;
    }
    if(personality==TSV_TSO&&mode!=64U)
        return load_native(personality,64U,name,image);
    return rc;
}
