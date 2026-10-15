/* SPDX-License-Identifier: MIT; forms use the same independent job fixture. */
#define main console_protocol_test
#include "submit.c"
#undef main
#include "twospace_batchui.h"
static TBFSTATE form;
static unsigned int uncertain;
static unsigned int form_service(void *c,unsigned int op,unsigned int n,unsigned char *h,void *p)
{unsigned int rc=service(c,op,n,h,p);if(op==TSA_IO_JOB_SUBMIT&&uncertain&&!rc)return 12U;return rc;}
static void start(void)
{TJFOPS ops;setup();memset(&ops,0,sizeof ops);ops.load=load;ops.service=form_service;ops.input=console;ops.line=message;ops.record=record;
 assert(!TBFINIT(&form,&state,&ops));uncertain=0U;}
static int event(const char *p)
{return TBFEVENT(&form,p,0x7dU);}
static int contains(const char *p)
{unsigned int i;char text[133];for(i=0U;i<form.shown;++i){assert(!TJCDEC(form.rows[i],132U,text));text[132]=0;if(strstr(text,p))return 1;}return 0;}
int main(void)
{
 unsigned int before,ticket,depth,argc;unsigned char frozen_job[TJB_FULL_BYTES],frozen_dd[TJD_EXT_BYTES],wide[132],dd[8];char longfield[133],arguments[101];
 /* Establish the actual console-route descriptor, then compare form preview. */
 assert(!console_protocol_test());memcpy(frozen_job,expected_job,sizeof frozen_job);memcpy(frozen_dd,expected_dd,sizeof frozen_dd);
 start();assert(!event("DECK APP.JCL(CASE)")&&!event("RUNHOLD YES")&&!event("PREVIEW"));
 assert(form.ready&&form.screen==TBF_PREVIEW&&!submitted&&!runs&&!prints);
 assert(!memcmp(frozen_job,state.result.job,sizeof frozen_job)&&!memcmp(frozen_dd,state.result.dd,sizeof frozen_dd));
 assert(!TBFVIEW(&form,3U,78U)&&contains("//CASE JOB"));assert(!event("DOWN")&&!TBFVIEW(&form,3U,78U)&&contains("//BUILD EXEC"));
 ticket=form.ticket;assert(!event("")&&form.ready&&form.ticket==ticket&&!submitted);
 assert(!TBFEVENT(&form,"",0x6dU)&&form.ready&&form.ticket==ticket&&!submitted);
 assert(event("SUBMIT unexpected")==8&&form.ready&&!submitted);
 assert(!event("SUBMIT")&&submitted==1U&&!runs&&form.last_id==1U&&form.attempted==ticket);
 assert(event("SUBMIT")==8&&submitted==1U);assert(!event("")&&submitted==1U);
 assert(!TBFEVENT(&form,"",0x6dU)&&form.attempted==ticket);assert(event("PREVIEW")==8&&submitted==1U);
 assert(!event("RELEASE")&&runs==1U&&!prints);assert(!event("OUTPUT")&&!event("STEP BUILD.RUN")&&!event("DD SYSPRINT"));
 assert(!TBFVIEW(&form,8U,78U)&&contains("RESULT 42"));depth=TJBWORD(TJBFIND(&pool,7U,1U)+24U);
 assert(!TBFVIEW(&form,8U,78U)&&TJBWORD(TJBFIND(&pool,7U,1U)+24U)==depth);
 /* Both horizontal pages expose the exact retained 132-byte record. */
 memset(longfield,'X',132U);longfield[132]=0;assert(!TJCENC(longfield,132U,wide));
 assert(!TJCENC("SYSPRINT",8U,dd));wide[0]=0xc1U;wide[131]=0xe9U;
 assert(!TJBAPP(&pool,7U,1U,dd,9U,wide,sizeof wide));
 depth=TJBWORD(TJBFIND(&pool,7U,1U)+24U);
 assert(!TBFVIEW(&form,8U,78U)&&form.shown==4U&&!memcmp(form.rows[3],wide,78U));
 assert(!event("RIGHT")&&!TBFVIEW(&form,8U,78U)&&!memcmp(form.rows[3],wide+54U,78U));
 assert(!event("LEFT")&&!TBFVIEW(&form,8U,78U)&&!memcmp(form.rows[3],wide,78U));
 assert(TJBWORD(TJBFIND(&pool,7U,1U)+24U)==depth);
 printer_failure=1U;before=runs;assert(event("PRINT")==12&&runs==before&&form.selected==1U);
 printer_failure=0U;assert(!event("PRINT")&&runs==before&&prints==2U);
 assert(!event("PURGE")&&event("CANCEL")==8&&runs==before);
 /* Editing invalidates a preview, while a refused long edit preserves it. */
 assert(!event("NEW")&&!event("PREVIEW"));ticket=form.ticket;
 memset(longfield,'X',100U);longfield[100]=0;strcpy(messages,"DECK ");strcat(messages,longfield);
 assert(event(messages)==8&&form.ready&&form.ticket==ticket&&!strcmp(form.deck,"APP.JCL(CASE)"));
 assert(!event("DECK APP.JCL(CASE)")&&!form.ready&&event("SUBMIT")==8);
 assert(!event("PREVIEW")&&!event("HOLD NO")&&!form.ready);
 assert(!event("PREVIEW")&&!event("NEW")&&!form.ready&&!form.attempted);
 /* Generate the common procedure using actual checked SITE and SRC values. */
 start();assert(!event("SRC APP.SRC(MAIN)")&&!event("PREVIEW"));
 assert(state.result.steps==3U&&state.result.dds>0U&&!submitted);assert(!event("RUNHOLD YES")&&!event("PREVIEW")&&!event("SUBMIT")&&submitted==1U&&!runs);
 assert(!event("NEW")&&!event("PROC CRXC")&&event("PREVIEW")==8&&!form.ready&&submitted==1U);
 assert(!event("OUT APP.OBJ(MAIN)")&&event("PREVIEW")!=0&&!form.ready); /* missing catalog */
 assert(!event("OUT APP.SRC(RESULT)")&&!event("PREVIEW")&&state.result.steps==2U);
 assert(!event("PROC CRXCL")&&!event("PREVIEW")&&state.result.steps==3U);
 assert(!event("OUT ")&&!event("PROC CRXLGO")&&!event("PREVIEW")&&state.result.steps==4U);
 /* Normal arguments produce the same checked procedure card, including
    JCL apostrophe escaping; oversize edits cannot destroy a ready preview. */
 assert(!event("ARGS -a SHARED")&&!strcmp(form.parameters,"ARGS='-a SHARED'")&&!event("PREVIEW")&&state.result.steps==4U);
 assert(!event("ARGS can't")&&!event("PREVIEW"));
 argc=TJBWORD(state.result.job+32U+3U*TJB_STEP_BYTES+16U);
 assert(argc<sizeof arguments&&!TJCDEC(state.result.job+32U+3U*TJB_STEP_BYTES+24U,argc,arguments));arguments[argc]=0;
 assert(!strcmp(arguments,"--no-autoload DD:RXBIN can't"));
 ticket=form.ticket;memset(longfield,'X',61U);longfield[61]=0;strcpy(messages,"ARGS ");strcat(messages,longfield);
 assert(event(messages)==8&&form.ready&&form.ticket==ticket);
 assert(!event("ARGS")&&!strcmp(form.parameters,"ARGS=''")&&!event("PREVIEW"));
 assert(!event("PARAMS WORK=TOOLONG8")&&event("PREVIEW")!=0&&!form.ready);
 assert(!event("PARAMS WORK=SEVEN77")&&!event("PREVIEW")&&form.ready);
 assert(!event("OVERRIDE //COMP.SYSIN DD DUMMY")&&!form.ready&&!event("PREVIEW"));
 for(before=form.overrides;before<8U;++before)assert(!event("OVERRIDE //BAD DD DUMMY"));
 assert(event("OVERRIDE //BAD DD DUMMY")==8&&event("PREVIEW")!=0&&!form.ready);
 /* A service error after publication must never replay the prepared request. */
 start();assert(!event("DECK APP.JCL(CASE)")&&!event("PREVIEW"));uncertain=1U;
 assert(event("SUBMIT")==12&&submitted==1U&&form.attempted&&!form.ready&&!form.last_id);
 assert(event("SUBMIT")==8&&event("PREVIEW")==8&&submitted==1U);
 assert(!event("JOBS")&&!TBFVIEW(&form,8U,78U)&&contains("JOB 1"));
 assert(!event("SELECT 999")&&event("CANCEL")==8&&TJBFIND(&pool,7U,1U));
 assert(event("SELECT 4294967296")==8&&event("SELECT 0")==8);
 assert(!TBFEVENT(&form,"",0xf3U)&&form.exit&&event("NEW")==16&&submitted==1U);
 /* Exhaustion must exit rather than trapping PF3 behind the revision guard. */
 start();assert(!event("DECK APP.JCL(CASE)")&&!event("PREVIEW"));form.revision=0xffffffffU;
 assert(event("SUBMIT")==8&&form.exit&&!form.ready&&!state.result.valid&&!submitted);
 puts("PD033 forms preview/equivalence, generated PROCs, retry identity, job/output and failure controls PASS");return 0;
}
