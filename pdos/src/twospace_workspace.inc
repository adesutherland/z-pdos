/* SPDX-License-Identifier: MIT; checked presentation context, no input lease.
 * Included by the K panel service and its focused boundary fault check. */
static void console_workspace_end(unsigned int owner)
{
    if(tp_workspace_owner==owner)
        tp_workspace_owner=tp_workspace_task=tp_workspace_has_job=0U;
}
static unsigned int console_workspace_busy(const TSVFRAME *f,unsigned int waiting,unsigned int waiting_owner)
{
    /* A genuine child/operator prompt must retain the existing input field
     * and its owner. Normal batch DD I/O never requests terminal input. */
    return tp_workspace_owner&&!(waiting&&f&&f->token==waiting_owner&&f->token!=tp_workspace_owner);
}
static unsigned int console_workspace_status(const TSVFRAME *f,const unsigned char *h)
{
    unsigned char data[68];TSPADDR p;unsigned int i,n,task,flags;
    if(!f||f->token!=tp_controller)return 16U;
    flags=cms_word(h+8U);n=cms_word(h+12U);
    p.hi=cms_word(h+16U);p.lo=cms_word(h+20U);task=cms_word(h+24U);
    if(flags==TSA_UI_WORKSPACE_END){
        if(n||p.hi||p.lo||task)return 8U;
        if(tc_raw_owner&&tc_raw_owner!=f->token)return 16U;
        console_workspace_end(f->token);
        return tc_raw_owner?0U:console_panel_render();
    }
    if(flags!=TSA_UI_WORKSPACE||!n||n>sizeof data||
       task<TSA_WORKSPACE_SOURCE||task>TSA_WORKSPACE_OUTPUT||
       tc_cap.device_class!=TSA_DEVICE_3270)return 8U;
    if(tc_raw_owner!=f->token||tc_read_owner||tc_attention_pending)return 16U;
    if(tc_span(p,n,TSG_READ,data,1U))return 8U;
    for(i=0U;i<n;++i)if(data[i]<0x40U||data[i]==0xffU)return 8U;
    tp_copy(tp_workspace_context,data,n);tp_workspace_bytes=n;
    tp_workspace_task=task;tp_workspace_owner=f->token;tp_workspace_has_job=0U;
    return 0U;
}
