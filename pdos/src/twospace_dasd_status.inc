/* SPDX-License-Identifier: MIT; owned read-only VTOC snapshot over the K gate.
 * All U chunks are preflighted before I/O or publication. One CPU keeps U
 * mappings stable through the synchronous service; no timer establishes this.
 */
static int media_address_before(TSPADDR a,TSPADDR b)
{return a.hi<b.hi||(a.hi==b.hi&&a.lo<b.lo);}
static TSPADDR media_address_plus(TSPADDR a,unsigned int n)
{TSPADDR b;b.lo=a.lo+n;b.hi=a.hi+(b.lo<a.lo);return b;}
static unsigned int media_status_span(TSPADDR p,unsigned int bytes,
                                      unsigned char *data,unsigned int copy)
{
    unsigned int at,n;TSPADDR last;
    if(!bytes)return 0U;
    last=media_address_plus(p,bytes-1U);
    if(bytes>TDA_MAX_TRACKS||media_address_before(last,p))return 8U;
    for(at=0U;at<bytes;at+=n){
        n=bytes-at;if(n>TDS_TRANSFER_BYTES)n=TDS_TRANSFER_BYTES;
        if(tc_span(media_address_plus(p,at),n,TSG_WRITE,0,0U))return 8U;
    }
    if(copy)for(at=0U;at<bytes;at+=n){
        n=bytes-at;if(n>TDS_TRANSFER_BYTES)n=TDS_TRANSFER_BYTES;
        if(tc_span(media_address_plus(p,at),n,TSG_WRITE,data+at,1U))return 8U;
    }
    return 0U;
}
static unsigned int media_status(TSGREQUEST *request)
{
    unsigned char header[64],raw[140],*map;TDAMAP state;TSPADDR pointer,tail,header_tail;
    unsigned int version,volume,capacity,c,h,r,i,pertrack,cylinders,tracks,status=12U;int got;
    if(!media_status_active()||request->length!=64U||
       tc_span(request->address,64U,TSG_READ,header,1U)||
       tc_span(request->address,64U,TSG_WRITE,header,0U))return 8U;
    version=cms_word(header);
    if((version!=1U&&version!=TDA_GEOMETRY_QUERY)||cms_word(header+4U)!=64U)return 20U;
    volume=cms_word(header+8U);capacity=cms_word(header+12U);
    pointer.hi=cms_word(header+56U);pointer.lo=cms_word(header+60U);
    if(capacity>TDA_MAX_TRACKS||(version==TDA_GEOMETRY_QUERY&&capacity))return 8U;
    if(capacity){
        tail=media_address_plus(pointer,capacity-1U);
        header_tail=media_address_plus(request->address,63U);
        if(media_address_before(tail,pointer)||media_address_before(header_tail,request->address)||
           (!media_address_before(tail,request->address)&&!media_address_before(header_tail,pointer))||
           media_status_span(pointer,capacity,0,0U))return 8U;
    }
    if(media_init())return 12U;
    if(volume>=(unsigned int)media_state.volume_count)return 20U;
    got=media_rdblock(media_state.volumes[volume].device,1,0,1,raw,140,0x0e);
    if(got!=140)return 12U;
    if(TDAGEOM(raw,140U,&cylinders,&tracks)||!raw[74U])return 20U;
    if(capacity&&capacity<tracks)return 8U;
    /* Version 1 capacity-zero still returns a complete allocation summary.
     * Version 2 is only a geometry query: no map allocation or VTOC scan. */
    if(version==TDA_GEOMETRY_QUERY){
        cms_put_word(header+8U,(unsigned int)media_state.volume_count);
        cms_put_word(header+12U,(unsigned int)media_state.volumes[volume].address);
        cms_put_word(header+16U,cylinders);cms_put_word(header+20U,TDA_HEADS);cms_put_word(header+24U,tracks);
        for(i=28U;i<48U;++i)header[i]=0U;
        for(i=0U;i<8U;++i)header[48U+i]=i<6U?(unsigned char)media_state.volumes[volume].volser[i]:0x40U;
        return tc_span(request->address,64U,TSG_WRITE,header,1U);
    }
    map=(unsigned char *)media_malloc(tracks);if(!map)return 4U;
    if(TDAINIT(&state,map,tracks,raw,140U)){status=20U;goto done;}
    pertrack=raw[74U];
    for(c=1U;c<=2U;++c)for(h=0U;h<TDA_HEADS;++h)for(r=c==1U&&h==0U?3U:1U;r<=pertrack;++r){
        got=media_rdblock(media_state.volumes[volume].device,(int)c,(int)h,(int)r,raw,140,0x0e);
        if(got!=140)goto done;
        if(!raw[44U]){for(i=0U;i<140U;++i)if(raw[i]){status=8U;goto done;}continue;}
        if(TDAADD(&state,raw,140U)){status=20U;goto done;}
    }
    if(TDASTAT(&state))goto done;
    cms_put_word(header+8U,(unsigned int)media_state.volume_count);
    cms_put_word(header+12U,(unsigned int)media_state.volumes[volume].address);
    cms_put_word(header+16U,cylinders);cms_put_word(header+20U,TDA_HEADS);cms_put_word(header+24U,tracks);
    cms_put_word(header+28U,state.reserved);cms_put_word(header+32U,state.allocated);cms_put_word(header+36U,state.free);
    cms_put_word(header+40U,state.largest);cms_put_word(header+44U,state.datasets);
    for(i=0U;i<8U;++i)header[48U+i]=i<6U?(unsigned char)media_state.volumes[volume].volser[i]:0x40U;
    status=capacity?media_status_span(pointer,tracks,map,1U):0U;
    if(!status)status=tc_span(request->address,64U,TSG_WRITE,header,1U);
done:
    media_free(map);return status;
}
