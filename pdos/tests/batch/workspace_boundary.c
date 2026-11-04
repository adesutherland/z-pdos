/* SPDX-License-Identifier: MIT; real K workspace gate with checked-copy faults. */
#include <assert.h>
#include <string.h>
#include "twospace_abi.h"
#include "twospace_job.h"
typedef struct {unsigned int hi,lo;} TSPADDR;
typedef struct {unsigned int token;} TSVFRAME;
static unsigned int tp_workspace_owner,tp_workspace_task,tp_workspace_bytes,tp_workspace_has_job;
static unsigned char tp_workspace_context[68],payload[68];
static unsigned int tp_controller=7U,tc_raw_owner=7U,tc_read_owner,tc_attention_pending,copies,renders,copy_error;
static struct {unsigned int device_class;} tc_cap;
#define TSG_READ 1U
static unsigned int cms_word(const unsigned char *p){return TJBWORD(p);}
static void tp_copy(unsigned char *to,const unsigned char *from,unsigned int n){memcpy(to,from,n);}
static unsigned int console_panel_render(void){++renders;return 0U;}
static unsigned int tc_span(TSPADDR p,unsigned int n,unsigned int mode,unsigned char *to,unsigned int copy)
{assert(mode==TSG_READ&&copy==1U);++copies;if(copy_error||p.hi||p.lo!=123U||n>sizeof payload)return 8U;memcpy(to,payload,n);return 0U;}
#include "../../src/twospace_workspace.inc"
int main(void)
{
 unsigned char h[32];TSVFRAME root,child;unsigned int before;
 root.token=7U;child.token=8U;tc_cap.device_class=TSA_DEVICE_3270;
 memset(payload,0xc1,sizeof payload);memset(h,0,sizeof h);
 TJBPUT(h+8U,TSA_UI_WORKSPACE);TJBPUT(h+12U,sizeof payload);TJBPUT(h+20U,123U);TJBPUT(h+24U,TSA_WORKSPACE_BUILD);
 assert(console_workspace_status(&child,h)==16U&&!copies&&!tp_workspace_owner);
 tc_raw_owner=0U;assert(console_workspace_status(&root,h)==16U&&!copies);tc_raw_owner=7U;
 tc_attention_pending=1U;assert(console_workspace_status(&root,h)==16U&&!copies);tc_attention_pending=0U;
 tc_read_owner=7U;assert(console_workspace_status(&root,h)==16U&&!copies);tc_read_owner=0U;
 copy_error=1U;assert(console_workspace_status(&root,h)==8U&&!tp_workspace_owner);copy_error=0U;
 payload[3U]=0xffU;assert(console_workspace_status(&root,h)==8U&&!tp_workspace_owner);payload[3U]=0xc1U;
 assert(!console_workspace_status(&root,h)&&tp_workspace_owner==7U&&tp_workspace_task==TSA_WORKSPACE_BUILD&&tp_workspace_bytes==68U&&!renders);
 assert(!memcmp(payload,tp_workspace_context,68U));before=copies;
 assert(console_workspace_busy(&root,0U,0U)&&console_workspace_busy(&child,0U,0U));
 assert(console_workspace_busy(&root,1U,7U)&&!console_workspace_busy(&child,1U,8U));
 assert(console_workspace_busy(&child,1U,9U)&&console_workspace_busy(&child,1U,0U));
 TJBPUT(h+12U,69U);assert(console_workspace_status(&root,h)==8U&&copies==before&&tp_workspace_owner==7U);
 console_workspace_end(8U);assert(tp_workspace_owner==7U);
 console_workspace_end(7U);assert(!tp_workspace_owner&&!tp_workspace_task&&!tp_workspace_has_job);
 assert(!console_workspace_busy(&root,0U,0U)&&!console_workspace_busy(&child,1U,8U));
 TJBPUT(h+12U,68U);assert(!console_workspace_status(&root,h));
 memset(h,0,sizeof h);TJBPUT(h+8U,TSA_UI_WORKSPACE_END);
 assert(console_workspace_status(&child,h)==16U&&tp_workspace_owner==7U);
 TJBPUT(h+24U,1U);assert(console_workspace_status(&root,h)==8U&&tp_workspace_owner==7U);TJBPUT(h+24U,0U);
 tc_raw_owner=8U;assert(console_workspace_status(&root,h)==16U&&tp_workspace_owner==7U);
 tc_raw_owner=7U;assert(!console_workspace_status(&root,h)&&!tp_workspace_owner&&!renders);
 tc_raw_owner=0U;assert(!console_workspace_status(&root,h)&&renders==1U);
 return 0;
}
