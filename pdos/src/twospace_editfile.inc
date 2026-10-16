/* SPDX-License-Identifier: MIT; K checked compare-and-save, IPL journal only.
 * No U pointer survives the call and no physical member is overwritten. */
static unsigned int edit_file_service(TSGREQUEST *request)
{
 unsigned char hdr[TEF_BYTES],dscb[96],ttr[3],digest[32],next[32];
 unsigned char *stage,*blocks,*work;SFDD dd;SFFILE file;TEFATTR a;
 TSPADDR pointer,tail,header_tail;
 unsigned int action,cap,i,n,used=0U,bytes=0U,count=0U,exists=1U,journal=0U,got,track;
 int device,rc,status=8;
 const TSVFRAME *frame=TSVTOP(&native_invocations);
 if(!frame||*(volatile const unsigned int *)0x3000U||*(volatile const unsigned int *)0x3010U||
    request->length!=TEF_BYTES||tc_span(request->address,TEF_BYTES,TSG_READ,hdr,1U)||
    tc_span(request->address,TEF_BYTES,TSG_WRITE,hdr,0U))return 8U;
 if(cms_word(hdr)!=1U||cms_word(hdr+4U)!=TEF_BYTES)return 20U;
 action=cms_word(hdr+8U);cap=cms_word(hdr+12U);
 if(action>TEF_NEW||cap>TEF_LIMIT||!TJDDSN(hdr+40U))return 8U;
 for(i=16U;i<40U;++i)if(!(action!=TEF_LOAD&&i>=20U&&i<32U)&&hdr[i])return 8U;
 if(action!=TEF_LOAD){a.format=cms_word(hdr+20U);a.lrecl=cms_word(hdr+24U);a.blksize=cms_word(hdr+28U);if(!TEFVALID(&a))return 8U;}
 for(i=98U;i<104U;++i)if(hdr[i])return 8U;
 n=44U;while(n&&hdr[40U+n-1U]==0x40U)--n;
 media_set(&dd,0,sizeof dd);media_set(&file,0,sizeof file);
 file.member=hdr[84U]!=0x40U;
 if(file.member&&!TJDNAME(hdr+84U))return 8U;
 if(!file.member)for(i=84U;i<92U;++i)if(hdr[i]!=0x40U)return 8U;
 pointer.hi=cms_word(hdr+136U);pointer.lo=cms_word(hdr+140U);
 if(cap){
  if(tc_span(pointer,cap,action==TEF_LOAD?TSG_WRITE:TSG_READ,hdr,0U))return 8U;
  tail.lo=pointer.lo+cap-1U;tail.hi=pointer.hi+(tail.lo<pointer.lo);
  header_tail.lo=request->address.lo+TEF_BYTES-1U;header_tail.hi=request->address.hi+(header_tail.lo<request->address.lo);
  if(!record_before(tail,request->address)&&!record_before(header_tail,pointer))return 8U;
 }
 if(media_init())return 12U;device=media_state.curdev;
 if(hdr[92U]!=0x40U){
  device=0;
  for(i=0U;i<(unsigned int)media_state.volume_count;++i)
   if(!media_cmp(hdr+92U,media_state.volumes[i].volser,6U)){device=media_state.volumes[i].device;break;}
  if(!device)return 4U;
 }else for(i=92U;i<98U;++i)if(hdr[i]!=0x40U)return 8U;
 /* Non-IPL legacy exchange files retain their separate empty-target API. */
 if((unsigned int)device!=*(volatile const unsigned int *)0x40bcU)return 20U;
 dd.device=(unsigned int)device;dd.bytes=n;media_copy(dd.dsn,hdr+40U,44U);
 if(TSKDSCB(sf_volume_record,&device,hdr+40U,n,&dd.extent,dscb)!=TSK_OK)return 4U;
 if(dd.extent.organisation!=(file.member?0x0200U:0x4000U))return 20U;
 a.format=dd.extent.record_format;a.lrecl=dd.extent.logical_length;a.blksize=dd.extent.block_length;
 if(!TEFVALID(&a))return 20U;
 if(action!=TEF_LOAD&&(a.format!=cms_word(hdr+20U)||a.lrecl!=cms_word(hdr+24U)||a.blksize!=cms_word(hdr+28U)))return 16U;
 if(action!=TEF_LOAD){
  if(job_active())return 16U;
  for(i=0U;i<sizeof sf_file/sizeof sf_file[0];++i)
   if(sf_file[i].token&&sf_dd[sf_file[i].dd].device==dd.device&&
      !media_cmp(sf_file[i].key,hdr+40U,52U))return 16U;
 }
 stage=(unsigned char *)media_malloc(TEF_LIMIT*3U+TEF_ID_BYTES);if(!stage)return 4U;
 blocks=stage+TEF_LIMIT;work=blocks+TEF_LIMIT;
 if(store_attach()!=TSS_OK){status=12;goto done;}
 rc=TSSSIZE(&native_store,sf_store_profile(&dd),hdr+40U,52U,&bytes);
 if(rc==TSS_OK){
  if(bytes>TEF_LIMIT){status=20;goto done;}
  rc=TSSREAD(&native_store,sf_store_profile(&dd),hdr+40U,52U,blocks,TEF_LIMIT,&bytes);
  if(rc!=TSS_OK){status=12;goto done;}journal=1U;
 }else if(rc==TSS_ABSENT){
  file.cylinder=dd.extent.start_cylinder;file.head=dd.extent.start_head;file.record=0U;bytes=0U;
  if(file.member){
   rc=sf_member(&dd,hdr+84U,ttr);
   if(rc<0){status=12;goto done;}if(!rc)exists=0U;
   else{track=sf_half(ttr);file.cylinder+=(dd.extent.start_head+track)/15U;
    file.head=(dd.extent.start_head+track)%15U;file.record=ttr[2U]-1U;}
  }
  if(exists)for(;;){
   rc=sf_disk_record(&dd,&file,a.blksize,&got);
   if(rc==1)break;if(rc){status=12;goto done;}
   if(!got||got>TEF_LIMIT-bytes||2U>TEF_LIMIT-bytes-got){status=20;goto done;}
   sf_put_half(blocks+bytes,got);media_copy(blocks+bytes+2U,TSCDATA(&channel),got);bytes+=got+2U;
  }
 }else if(rc==TSS_DELETED){bytes=0U;exists=0U;journal=1U;}
 else{status=12;goto done;}
 status=TEFUNPK(&a,blocks,bytes,stage,TEF_LIMIT,&used,&count);if(status)goto done;
 status=TEFTOK(hdr+40U,&a,exists,blocks,bytes,work,TEF_LIMIT+TEF_ID_BYTES,digest);if(status)goto done;
 if(action==TEF_LOAD){
  if(cap&&used>cap){status=4;goto done;}
  if(cap&&used&&tc_span(pointer,used,TSG_WRITE,stage,1U)){status=8;goto done;}
 }else{
  if(action==TEF_NEW&&count){status=16;goto done;}
  if(cap&&tc_span(pointer,cap,TSG_READ,stage,1U)){status=8;goto done;}
  status=TEFPACK(&a,stage,cap,blocks,TEF_LIMIT,&bytes,&count);if(status)goto done;
  status=TEFTOK(hdr+40U,&a,1U,blocks,bytes,work,TEF_LIMIT+TEF_ID_BYTES,next);if(status)goto done;
  if(media_cmp(digest,next,32U)){
   if(media_cmp(digest,hdr+104U,32U)){status=16;goto done;}
   rc=TSSADMIT(&native_store,sf_store_profile(&dd),hdr+40U,52U,bytes);
   if(rc!=TSS_OK){status=rc==TSS_FULL?4:12;goto done;}
   rc=TSSPUT(&native_store,sf_store_profile(&dd),hdr+40U,52U,blocks,bytes);
   if(rc!=TSS_OK){status=rc==TSS_FULL?4:12;goto done;}
   journal=1U;
  }
  media_copy(digest,next,32U);exists=1U;used=cap;
 }
 cms_put_word(hdr+16U,used);cms_put_word(hdr+20U,a.format);cms_put_word(hdr+24U,a.lrecl);
 cms_put_word(hdr+28U,a.blksize);cms_put_word(hdr+32U,count);
 cms_put_word(hdr+36U,(exists?TEF_EXISTS:0U)|(file.member?TEF_MEMBER:0U)|(journal?TEF_JOURNAL:0U));
 media_copy(hdr+104U,digest,32U);status=(int)tc_span(request->address,TEF_BYTES,TSG_WRITE,hdr,1U);
done:
 media_free(stage);if(media_scratch_real)return 12U;return (unsigned int)status;
}
