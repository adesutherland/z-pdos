/* SPDX-License-Identifier: MIT; checked PS record transfer over common I/O.
 * Source/target admission and entire payload validation precede mutation.
 * No address, block, dataset extent or completion is trusted from U.
 */
static unsigned int record_track_cost(unsigned int n)
{
    unsigned int intervals=(n+237U)/232U;
    return ((646U+n+6U+6U*intervals+33U)/34U)*34U;
}
static int record_before(TSPADDR a,TSPADDR b)
{return a.hi<b.hi||(a.hi==b.hi&&a.lo<b.lo);}
static unsigned int record_file_service(TSGREQUEST *request)
{
    unsigned char hdr[TRI_BYTES],raw[32768],dscb[140],verify[32768];
    unsigned char *stage,*blocks;DSCB1 ds;TSPADDR pointer,tail,header_tail;
    unsigned int op,capacity,n,format,lrecl,blksize,c,h,at=0U,used=0U,
        count=0U,blocks_used=0U,block_count=0U,total=0U,r,i,j,length,part;
    int vc,vh,vr,got,device,status=8;
    const TSVFRAME *frame=TSVTOP(&native_invocations);
    if(!frame||*(volatile const unsigned int *)0x3000U||*(volatile const unsigned int *)0x3010U||
       request->length!=TRI_BYTES||tc_span(request->address,TRI_BYTES,TSG_READ,hdr,1U)||
       tc_span(request->address,TRI_BYTES,TSG_WRITE,hdr,0U))return 8U;
    if(cms_word(hdr)!=1U||cms_word(hdr+4U)!=TRI_BYTES)return 20U;
    op=cms_word(hdr+8U);capacity=cms_word(hdr+12U);n=cms_word(hdr+84U);
    pointer.hi=cms_word(hdr+88U);pointer.lo=cms_word(hdr+92U);
    if(op>TRI_SAVE_EMPTY||capacity>TRI_LIMIT||!n||n>44U||cms_word(hdr+36U)>1U||
       (op==TRI_PROBE&&capacity)||
       (capacity&&tc_span(pointer,capacity,op==TRI_LOAD?TSG_WRITE:TSG_READ,raw,0U)))return 8U;
    if(capacity){
        tail.lo=pointer.lo+capacity-1U;tail.hi=pointer.hi+(tail.lo<pointer.lo);
        header_tail.lo=request->address.lo+TRI_BYTES-1U;
        header_tail.hi=request->address.hi+(header_tail.lo<request->address.lo);
        if(!record_before(tail,request->address)&&!record_before(header_tail,pointer))return 8U;
    }
    if(media_init())return 12U;device=media_state.curdev;
    if(cms_word(hdr+36U)){
        device=0;
        for(i=0U;i<(unsigned int)media_state.volume_count;++i)
            if(!media_cmp(hdr+20U,media_state.volumes[i].volser,6U)){device=media_state.volumes[i].device;break;}
        if(!device)return 4U;
    }
    for(i=0U;i<n;++i){int ch=hdr[40U+i];
        if(!(media_alpha(ch)||media_digit(ch)||ch=='.'||ch=='#'||ch=='$'||ch=='@'))return 8U;
    }
    for(i=0U;i<n;++i)raw[i]=hdr[40U+i];raw[n]=0U;
    if(!media_dscb(device,(const char *)raw,&ds,&vc,&vh,&vr))return 4U;
    c=((unsigned char)ds.startcchh[0]<<8)|(unsigned char)ds.startcchh[1];
    h=((unsigned char)ds.startcchh[2]<<8)|(unsigned char)ds.startcchh[3];
    format=(unsigned char)ds.ds1recfm;lrecl=(unsigned short)ds.ds1lrecl;blksize=(unsigned short)ds.ds1blkl;
    if(ds.ds1dsorg!=0x4000||ds.ds1noepv!=1||c<3U||c>=100U||h||
       ds.startcchh[0]!=ds.endcchh[0]||ds.startcchh[1]!=ds.endcchh[1]||
       ds.endcchh[2]||((unsigned char)ds.endcchh[3])!=14U||
       (format!=0x90U&&format!=0x50U)||
       !lrecl||!blksize||blksize>32760U||lrecl>blksize||
       ((format&0x40U)&&!(format&0x80U)&&(lrecl<4U||lrecl+4U>blksize))||
       ((format&0x80U)&&blksize%lrecl))return 20U;
    cms_put_word(hdr+20U,format);cms_put_word(hdr+24U,lrecl);cms_put_word(hdr+28U,blksize);
    if(op==TRI_PROBE)return tc_span(request->address,TRI_BYTES,TSG_WRITE,hdr,1U);
    if(!capacity&&op==TRI_LOAD)return 8U;
    if(op==TRI_SAVE_EMPTY&&pdosVolumeIndex(&media_state,device)<=0)return 20U;
    /* Reads share the completion engine with the caller's loaded image.
     * Only mutation needs volume exclusivity; an active EDIT image must
     * be able to read source records from its own volume. */
    if(op==TRI_SAVE_EMPTY&&media_busy((unsigned int)device))return 16U;
    stage=(unsigned char *)media_malloc(TRI_LIMIT*2U);if(!stage)return 4U;
    blocks=stage+TRI_LIMIT;
    if(op==TRI_LOAD){
        for(r=1U;r<=65U;++r){
            got=media_rdblock(device,(int)c,0,(int)r,raw,(int)blksize,0x0e);
            if(!got){status=0;break;}if(got<0||r==65U){status=12;break;}
            length=(unsigned int)got;total+=record_track_cost(length);
            if(total+record_track_cost(0U)>58786U){status=20;break;}
            if(format&0x80U){if(length%lrecl){status=12;break;}at=0U;}
            else {if(length<4U||sf_half(raw)!=length||raw[2]||raw[3]){status=12;break;}at=4U;}
            while(at<length){
                part=lrecl;j=at;
                if(!(format&0x80U)){
                    if(length-at<4U||sf_half(raw+at)<4U||sf_half(raw+at)>lrecl||
                       sf_half(raw+at)>length-at||raw[at+2U]||raw[at+3U])break;
                    part=sf_half(raw+at)-4U;j+=4U;
                }
                if(used+2U+part>capacity)break;
                sf_put_half(stage+used,part);used+=2U;
                for(i=0U;i<part;++i)stage[used++]=raw[j+i];++count;at=j+part;
            }
            if(at!=length){status=20;break;}
        }
        if(!status&&tc_span(pointer,used,TSG_WRITE,stage,1U))status=8;
    }else{
        if(capacity&&tc_span(pointer,capacity,TSG_READ,stage,1U))goto done;
        /* Construct every physical block in K, including RDW/BDW, before
         * checking emptiness and writing. FB source padding stays exact. */
        at=0U;length=(format&0x80U)?0U:4U;
        for(i=0U;i<4U;++i)raw[i]=0U;
        while(at<capacity){
            if(capacity-at<2U)goto done;part=sf_half(stage+at);at+=2U;
            if(part>capacity-at||(format&0x80U?part!=lrecl:part+4U>lrecl))goto done;
            j=part+((format&0x80U)?0U:4U);
            if(length+j>blksize){
                if(!(format&0x80U))sf_put_half(raw,length);
                if(blocks_used+length+2U>TRI_LIMIT||++block_count>64U)goto done;
                sf_put_half(blocks+blocks_used,length);blocks_used+=2U;
                for(r=0U;r<length;++r)blocks[blocks_used++]=raw[r];total+=record_track_cost(length);
                length=(format&0x80U)?0U:4U;for(r=0U;r<4U;++r)raw[r]=0U;
            }
            if(!(format&0x80U)){sf_put_half(raw+length,part+4U);raw[length+2U]=raw[length+3U]=0U;length+=4U;}
            for(r=0U;r<part;++r)raw[length++]=stage[at++];++count;
        }
        if(count&&length){
            if(!(format&0x80U))sf_put_half(raw,length);
            if(blocks_used+length+2U>TRI_LIMIT||++block_count>64U)goto done;
            sf_put_half(blocks+blocks_used,length);blocks_used+=2U;
            for(r=0U;r<length;++r)blocks[blocks_used++]=raw[r];total+=record_track_cost(length);
        }
        if(total+record_track_cost(0U)>58786U){status=20;goto done;}
        got=media_rdblock(device,(int)c,0,1,raw,(int)blksize,0x0e);
        if(got){status=got<0?12:16;goto done;}
        at=0U;
        for(r=1U;r<=block_count+1U;++r){
            length=r<=block_count?sf_half(blocks+at):0U;
            sf_put_half(raw,c);raw[2]=raw[3]=raw[5]=0U;raw[4]=(unsigned char)r;sf_put_half(raw+6U,length);
            for(i=0U;i<length;++i)raw[8U+i]=blocks[at+2U+i];
            if(media_wrblock(device,(int)c,0,(int)r-1,raw,(int)length+8,0x1d)!=(int)length+8){status=12;goto done;}
            got=media_rdblock(device,(int)c,0,(int)r,verify,(int)blksize,0x0e);
            if(got!=(int)length||(length&&media_cmp(verify,blocks+at+2U,length))){status=12;goto done;}
            at+=length+2U;
        }
        ds.ds1lstar[0]=ds.ds1lstar[1]=0;ds.ds1lstar[2]=(char)(block_count+1U);
        media_copy(dscb,&ds,sizeof ds);
        if(media_wrblock(device,vc,vh,vr,dscb+44U,96,0x05)!=96){status=12;goto done;}
        if(media_rdblock(device,vc,vh,vr,verify,140,0x0e)!=140||media_cmp(dscb,verify,140U)){status=12;goto done;}
        used=capacity;status=0;
    }
done:
    media_free(stage);
    if(media_scratch_real)return 12U;
    if(!status){cms_put_word(hdr+16U,used);cms_put_word(hdr+32U,count);status=(int)tc_span(request->address,TRI_BYTES,TSG_WRITE,hdr,1U);}
    return (unsigned int)status;
}
