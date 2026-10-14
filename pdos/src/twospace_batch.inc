/* SPDX-License-Identifier: MIT; K-owned neutral card/printer service.
 * This provider never acquires a console lease or publishes a partial card.
 */
static TBCPOOL batch_endpoints;
static TSCSTATE batch_channels[2];
static unsigned int batch_bound[2];
static unsigned int batch_device_close(unsigned int owner,unsigned int handle)
{
    TBCENDPOINT *e=TBCFIND(&batch_endpoints,owner,handle);TSCSTATE *c;const TIOREQUEST *r;
    if(!e)return 8U;c=&batch_channels[e->kind-1U];r=TIOSTATE(&channel_io,c);
    if(r&&r->phase!=TIO_IDLE&&channel_quiesce(c,e->ssid,owner))return 12U;
    if(c->quarantined||c->occupied)return 12U;
    return TBCCLOSE(&batch_endpoints,owner,handle)?12U:0U;
}
static int batch_overlap(TSPADDR a,unsigned int an,TSPADDR b,unsigned int bn)
{
    TSPADDR ae=a,be=b;
    ae.lo+=an;ae.hi+=ae.lo<a.lo;be.lo+=bn;be.hi+=be.lo<b.lo;
    if(ae.hi<a.hi||be.hi<b.hi)return 1;
    return (a.hi<be.hi||(a.hi==be.hi&&a.lo<be.lo))&&
           (b.hi<ae.hi||(b.hi==ae.hi&&b.lo<ae.lo));
}
static unsigned int batch_transfer(TBCENDPOINT *e,unsigned int command,unsigned int count)
{
    TSCSTATE *c=&batch_channels[e->kind-1U];const TIOREQUEST *operation;
    TBCRESULT result,sense_result;unsigned char original[64];unsigned int i;int status,checked;
    if(TBCBUILD(c,e->kind,command,count))return c->occupied||c->quarantined?16U:8U;
    status=channel_run(c,e->ssid);operation=TIOSTATE(&channel_io,c);
    for(i=0U;i<64U;++i)original[i]=status&&operation&&operation->status_valid?operation->status[i]:TSCIRB(c)[i];
    checked=TBCCHECK(c,original,e->kind,command,count,&result);e->result=result;
    if(!status&&(checked==TBC_OK||checked==TBC_EOF))return 0U;
    /* Preserve the failed transfer, not the subsequent Sense/clear receipt. */
    if(!c->quarantined&&!c->occupied&&(original[8U]&2U)&&!original[9U]&&
       !TBCBUILD(c,e->kind,4U,TBC_SENSE_BYTES)&&!channel_run(c,e->ssid)&&
       TBCCHECK(c,TSCIRB(c),e->kind,4U,TBC_SENSE_BYTES,&sense_result)==TBC_OK){
        e->result.sense_bytes=sense_result.actual;
        for(i=0U;i<sense_result.actual;++i)e->result.sense[i]=TSCDATA(c)[i];
    }
    return 12U;
}
static void batch_report(unsigned char *h,const TBCENDPOINT *e)
{
    unsigned int i;cms_put_word(h+8U,e->handle);cms_put_word(h+12U,e->kind);cms_put_word(h+16U,e->address);
    cms_put_word(h+28U,e->result.actual);cms_put_word(h+40U,e->result.device_status);cms_put_word(h+44U,e->result.channel_status);
    cms_put_word(h+48U,e->result.sense_bytes);for(i=0U;i<TBC_SENSE_BYTES;++i)h[52U+i]=e->result.sense[i];
    cms_put_word(h+60U,e->result.eof?TSA_BATCH_EOF:0U);
}
static unsigned int batch_device_service(TSGREQUEST *request)
{
    const TSVFRAME *f=TSVTOP(&native_invocations);TBCENDPOINT *e;TSCSTATE *c;
    unsigned char h[64],output[TBC_PRINT_BYTES];TSPADDR payload;
    unsigned int op=request->direction,handle,kind,address,command,capacity,i,rc,real;int status;
    if(!f||*(volatile const unsigned int *)0x3000U||*(volatile const unsigned int *)0x3010U||request->length!=TSA_BATCH_BYTES||
       tc_span(request->address,64U,TSG_READ,h,0U)||tc_span(request->address,64U,TSG_WRITE,h,0U)||
       tc_span(request->address,64U,TSG_READ,h,1U))return 8U;
    if(cms_word(h)!=1U||cms_word(h+4U)!=64U)return 20U;
    handle=cms_word(h+8U);kind=cms_word(h+12U);address=cms_word(h+16U);command=cms_word(h+20U);
    capacity=cms_word(h+24U);payload.hi=cms_word(h+32U);payload.lo=cms_word(h+36U);
    /* Output words are ignored after OPEN, so a result can be reused safely. */
    if(op==TSA_IO_BATCH_OPEN){
        if(handle||command||capacity||payload.hi||payload.lo)return 8U;
        for(i=28U;i<64U;++i)if(h[i])return 8U;
        if(kind!=TBC_READER&&kind!=TBC_PRINTER)return 20U;
        if(!address||address>65535U)return 8U;
        if(address==tc_cap.address||address==tc_cap.monitor||address==*(volatile const unsigned int *)0x40b8U)return 20U;
        status=TBCOPEN(&batch_endpoints,f->token,kind,address,&handle);if(status)return status==TBC_BUSY?16U:8U;
        e=TBCFIND(&batch_endpoints,f->token,handle);c=&batch_channels[kind-1U];
        if(TSVOWN(&native_invocations,f->token,TSV_BATCH_DEVICE,handle)!=TSV_OK){TBCCLOSE(&batch_endpoints,f->token,handle);return 4U;}
        rc=12U;
        if(!batch_bound[kind-1U]){
            real=kind==TBC_READER?TBC_READER_REAL:TBC_PRINTER_REAL;
            if(TSCINIT(c,(unsigned char *)TSF_KAPERTURE_VA,TSF_REAL_BYTES,real)||TIOBIND(&channel_io,c))goto open_failed;
            batch_bound[kind-1U]=1U;
        }
        e->ssid=tc_find(address,c);if(!e->ssid){rc=20U;goto open_failed;}
        rc=batch_transfer(e,0xe4U,7U);if(rc)goto open_failed;
        if(TSCDATA(c)[0U]!=0xffU||TSCDATA(c)[4U]!=(kind==TBC_READER?0x35U:0x14U)||
           TSCDATA(c)[5U]!=(kind==TBC_READER?0x05U:0x03U)){rc=20U;goto open_failed;}
        e->result.actual=0U;batch_report(h,e);return tc_span(request->address,64U,TSG_WRITE,h,1U);
open_failed:
        if(batch_device_close(f->token,handle))return 12U;
        if(TSVFORGET(&native_invocations,f->token,TSV_BATCH_DEVICE,handle)!=TSV_OK)return 12U;
        return rc;
    }
    e=TBCFIND(&batch_endpoints,f->token,handle);
    if(!e||TSVHAS(&native_invocations,f->token,TSV_BATCH_DEVICE,handle)!=TSV_OK)return 8U;
    if((kind&&kind!=e->kind)||(address&&address!=e->address))return 8U;
    if(op==TSA_IO_BATCH_CLOSE||op==TSA_IO_BATCH_STATUS){
        if(command||capacity||payload.hi||payload.lo)return 8U;
        batch_report(h,e);
        if(op==TSA_IO_BATCH_CLOSE){rc=batch_device_close(f->token,handle);if(rc)return rc;
            if(TSVFORGET(&native_invocations,f->token,TSV_BATCH_DEVICE,handle)!=TSV_OK)return 12U;}
        return tc_span(request->address,64U,TSG_WRITE,h,1U);
    }
    if(op==TSA_IO_BATCH_READ){
        if(e->kind!=TBC_READER||(command&&command!=2U))return 20U;
        if(capacity!=TBC_CARD_BYTES)return 8U;command=2U;
    }else if(op==TSA_IO_BATCH_WRITE){
        if(e->kind!=TBC_PRINTER||(command!=9U&&command!=0x89U))return 20U;
        if(!capacity||capacity>TBC_PRINT_BYTES)return 8U;
    }else return 20U;
    if(batch_overlap(request->address,64U,payload,capacity)||
       tc_span(payload,capacity,op==TSA_IO_BATCH_READ?TSG_WRITE:TSG_READ,output,0U))return 8U;
    c=&batch_channels[e->kind-1U];if(c->occupied||c->quarantined)return 16U;
    if(op==TSA_IO_BATCH_WRITE){
        if(tc_span(payload,capacity,TSG_READ,output,1U))return 8U;
        /* Builders preserve output bytes. Copy only after whole-span preflight. */
        for(i=0U;i<capacity;++i)TSCDATA(c)[i]=output[i];
    }
    rc=batch_transfer(e,command,capacity);
    if(!rc&&op==TSA_IO_BATCH_READ&&e->result.actual&&tc_span(payload,e->result.actual,TSG_WRITE,TSCDATA(c),1U))return 8U;
    batch_report(h,e);if(tc_span(request->address,64U,TSG_WRITE,h,1U))return 8U;return rc;
}
