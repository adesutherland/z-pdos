/* SPDX-License-Identifier: MIT; real record/edit boundary controls. */
#include <assert.h>
#include <string.h>
#include "apps/edit_core.h"
static EDCSTATE state,prior,round;
int main(void)
{
    unsigned char raw[]={0,3,'a',' ',' ',0,0,0,1,'z'};
    unsigned char packed[100],longline[256];unsigned int n,i;
    assert(!EDCLOAD(&state,raw,sizeof raw));assert(state.count==3U);
    assert(state.line[0].length==3U&&state.line[1].length==0U);
    assert(!EDCPACK(&state,packed,sizeof packed,0x50U,8U,&n));
    assert(n==sizeof raw&&!memcmp(packed,raw,n));
    prior=state;assert(EDCINSERT(&state,4U,raw,1U));assert(EDCEQUAL(&state,&prior));
    assert(!EDCINSERT(&state,0U,(unsigned char *)"new",3U));
    assert(!EDCCHANGE(&state,1U,(unsigned char *)"new",3U,(unsigned char *)"NEW",3U));
    assert(!EDCDELETE(&state,1U,1U));assert(EDCEQUAL(&state,&prior));
    assert(!EDCPACK(&state,packed,sizeof packed,0x90U,5U,&n));
    assert(!EDCLOAD(&round,packed,n));assert(round.line[0].length==5U);
    assert(!memcmp(round.line[0].text,"a    ",5U));
    assert(!memcmp(round.line[1].text,"     ",5U));
    assert(EDCPACK(&state,packed,sizeof packed,0x50U,6U,&n));
    assert(EDCPACK(&state,packed,2U,0x50U,8U,&n));
    assert(EDCLOAD(&state,raw,sizeof raw-1U));assert(EDCEQUAL(&state,&prior));
    memset(&state,0,sizeof state);memset(longline,'x',sizeof longline);
    for(i=0U;i<EDC_LINES;++i)assert(!EDCINSERT(&state,i,longline,sizeof longline));
    prior=state;assert(EDCINSERT(&state,0U,longline,1U));assert(EDCEQUAL(&state,&prior));
    assert(EDCCHANGE(&state,1U,longline,1U,longline,2U));assert(EDCEQUAL(&state,&prior));
    return 0;
}
