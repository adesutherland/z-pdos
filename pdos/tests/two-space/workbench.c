/* SPDX-License-Identifier: MIT; exact selected dock and adaptable cell geometry. */
#include "twospace_workbench.h"
#include "twospace_display.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static int contains(const TWFSURFACE *s,const char *label)
{unsigned char text[128];unsigned int n,i;if(TWDLabel(label,text,sizeof text,&n))return 0;
 for(i=0U;i+n<=s->geometry.cells;++i)if(!memcmp(s->text+i,text,n))return 1;return 0;}
int main(void)
{
    unsigned char *memory=(unsigned char *)malloc(3U*TWF_MAX_CELLS),history[4096],scratch[16384],title[64],name[8],volume[6],text[64];
    TPHRECORD records[32];TPHSTATE log;TPHVIEW program,shell;T27GEOMETRY g;TWFSURFACE surface;TWBCONTEXT context;TWBPLAN plan;
    unsigned int n,models[4][2]={{24U,80U},{27U,132U},{43U,80U},{62U,160U}},i;
    assert(memory&&!TPHINIT(&log,history,sizeof history,records,32U));
    assert(!TWDLabel("z/PDOS Workbench / PCOMM",title,sizeof title,&n));memset(&context,0,sizeof context);context.title=title;context.title_bytes=n;
    assert(!TWDLabel("RXVM",name,sizeof name,&n));context.name=name;context.name_bytes=n;
    assert(!TWDLabel("PDOS00",volume,sizeof volume,&n));context.volume=volume;context.volume_bytes=n;
    context.job=1U;context.phase=TWB_DONE;context.rc_valid=1U;
    assert(!TWDLabel("Hello from cREXX.",text,sizeof text,&n)&&!TPHAPPEND(&log,1U,11U,TPH_TEXT,text,n));
    assert(!TWDLabel("#1 > RXVM HELLO",text,sizeof text,&n)&&!TPHAPPEND(&log,0xffffffffU,7U,TPH_TEXT,text,n));
    assert(!TWDLabel("OS=0 RC=0",text,sizeof text,&n)&&!TPHAPPEND(&log,0xffffffffU,7U,TPH_TEXT,text,n));
    for(i=0U;i<4U;++i){
        assert(!T27GEOM(&g,models[i][0],models[i][1],T27_BINARY14));
        assert(!TWFINIT(&surface,&g,memory,memory+16384U,memory+32768U)&&!TWBLAYOUT(g.rows,g.columns,&plan));
        TPHFOLLOW(&program,1U);TPHFOLLOW(&shell,0xffffffffU);
        assert(!TWBCOMPOSE(&surface,&plan,&context,&log,&program,&shell,scratch,sizeof scratch));
        assert(surface.capacity==256U&&surface.kind[surface.input-1U]==TWF_ATTRIBUTE&&surface.kind[surface.input+256U]==TWF_ATTRIBUTE);
        assert(surface.text[plan.body*g.columns+1U]==0xc8U);
        assert(contains(&surface,"PF10 Output/Shell")&&!contains(&surface,"PF11")&&!contains(&surface,"PF12"));
        assert(surface.role[2U*g.columns]==TWF_HEADER);
        context.focus=1U;assert(!TWBCOMPOSE(&surface,&plan,&context,&log,&program,&shell,scratch,sizeof scratch));
        assert(surface.role[2U*g.columns]==TWF_FOCUS);context.focus=0U;
        if(i==0U){assert(plan.input==17U*80U+4U&&plan.body_rows==9U&&plan.status==21U&&plan.keys==22U);}
        if(i==1U){assert(plan.input==23U*132U+4U&&plan.body_rows==18U&&plan.body_width==85U&&plan.shell_column==89U);}
        context.help=1U;assert(!TWBCOMPOSE(&surface,&plan,&context,&log,&program,&shell,scratch,sizeof scratch));
        assert(contains(&surface,"PF10 toggles output and shell focus.")&&!contains(&surface,"PF11")&&!contains(&surface,"PF12"));context.help=0U;
        context.joined=7U;assert(!TWBCOMPOSE(&surface,&plan,&context,&log,&program,&shell,scratch,sizeof scratch));
        assert(!TWDLabel("CONTINUE COMMAND",text,sizeof text,&n)&&!memcmp(surface.text+plan.heading*g.columns+1U,text,n));context.joined=0U;
    }
    assert(TWBLAYOUT(24U,79U,&plan)==-1);free(memory);return 0;
}
