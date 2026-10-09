/* SPDX-License-Identifier: MIT
 * Owner and completion ordering controls for the K invocation ledger.
 */
#include "twospace_invocation.h"
#include <assert.h>
#include <stdio.h>

typedef struct {
    unsigned int order[8];
    unsigned int count;
} CLEANTRACE;

static int clean_owned(unsigned int token, const TSVRESOURCE *resource,
                       void *context)
{
    CLEANTRACE *trace=(CLEANTRACE *)context;
    assert(token!=0U && resource && trace);
    if (resource->kind==TSV_IO && resource->state!=TSV_IO_COMPLETE)
        return 1;
    assert(trace->count<8U);
    trace->order[trace->count++]=resource->handle;
    return 0;
}

int main(void)
{
    TSVSTACK stack;
    TSVCONTEXT parent_context, child_context;
    CLEANTRACE trace;
    const TSVFRAME *top;
    unsigned int parent, child, later, i;
    TSVINIT(&stack);
    for (i=0U; i<16U; ++i) {
        parent_context.gpr[i].hi=i;
        parent_context.gpr[i].lo=0x1000U+i;
        child_context.gpr[i].hi=i+16U;
        child_context.gpr[i].lo=0x2000U+i;
        parent_context.fpr[i].hi=0x3ff00000U+i;
        parent_context.fpr[i].lo=0x12345678U+i;
        parent_context.access[i]=0x11110000U+i;
        child_context.fpr[i].hi=0x40000000U+i;
        child_context.fpr[i].lo=0x87654321U+i;
        child_context.access[i]=0x22220000U+i;
    }
    parent_context.psw.hi=0x80000000U;
    parent_context.psw.lo=0x3000000U;
    parent_context.psw_address.hi=1U;
    parent_context.psw_address.lo=0x23456780U;
    parent_context.asce.hi=0U;
    parent_context.asce.lo=0x4000U;
    parent_context.key=8U;
    child_context.psw.hi=0x80000000U;
    child_context.psw.lo=0x5000000U;
    child_context.psw_address.hi=0U;
    child_context.psw_address.lo=0x00abcdefU;
    child_context.asce.hi=0U;
    child_context.asce.lo=0x4000U;
    child_context.key=8U;
    parent_context.fpc=0x00080000U; child_context.fpc=0x00100000U;
    trace.count=0U;
    assert(TSVTOP(&stack)==0);
    assert(TSVBEGIN(&stack,TSV_CMS,64U,1U,2U,&parent_context,
                    &parent)==TSV_BAD);
    assert(TSVBEGIN(&stack,TSV_PDOS,31U,20U,20U,&parent_context,
                    &parent)==TSV_OK);
    assert(TSVTOP(&stack)->personality==TSV_PDOS);
    assert(TSVOWN(&stack,parent,TSV_DEVICE_SESSION,808U)==TSV_OK);
    assert(TSVHAS(&stack,parent,TSV_DEVICE_SESSION,808U)==TSV_OK);
    assert(TSVOWN(&stack,parent,TSV_PANEL,909U)==TSV_OK);
    assert(TSVHAS(&stack,parent,TSV_PANEL,909U)==TSV_OK);
    assert(TSVHAS(&stack,parent+1U,TSV_PANEL,909U)==TSV_BUSY);
    assert(TSVFORGET(&stack,parent,TSV_PANEL,909U)==TSV_OK);
    assert(TSVFORGET(&stack,parent,TSV_DEVICE_SESSION,808U)==TSV_OK);
    assert(TSVHAS(&stack,parent,TSV_DEVICE_SESSION,808U)==TSV_STALE);
    assert(TSVEND(&stack,parent,clean_owned,&trace)==TSV_OK);
    assert(TSVTOP(&stack)==0);
    assert(TSVBEGIN(&stack,TSV_CMS,31U,1U,2U,&parent_context,
                    &parent)==TSV_OK);
    top=TSVTOP(&stack);
    assert(top && top->token==parent && !top->parent &&
           top->caller.gpr[15].lo==0x100fU && top->caller.key==8U &&
           top->caller.psw_address.hi==1U &&
           top->caller.psw_address.lo==0x23456780U);
    assert(top->caller.fpr[15].hi==0x3ff0000fU &&
           top->caller.fpr[15].lo==0x12345687U &&
           top->caller.access[15]==0x1111000fU &&
           top->caller.fpc==0x00080000U);
    assert(TSVOWN(&stack,parent,TSV_FILE,100U)==TSV_OK);
    assert(TSVHAS(&stack,parent,TSV_FILE,100U)==TSV_OK);
    assert(TSVHAS(&stack,parent,TSV_FILE,200U)==TSV_STALE);
    assert(TSVOWN(&stack,parent,TSV_IO,101U)==TSV_OK);
    assert(TSVOWN(&stack,parent,TSV_IO,101U)==TSV_BUSY);
    assert(TSVFORGET(&stack,parent,TSV_IO,101U)==TSV_BUSY);
    assert(TSVOWN(&stack,parent,TSV_ALLOCATION,102U)==TSV_OK);
    assert(TSVFORGET(&stack,parent,TSV_ALLOCATION,102U)==TSV_OK);
    assert(TSVBEGIN(&stack,TSV_CMS,24U,3U,4U,&child_context,
                    &child)==TSV_OK);
    top=TSVTOP(&stack);
    assert(top && top->token==child && top->parent==parent &&
           top->caller.gpr[15].lo==0x200fU &&
           top->caller.psw_address.hi==0U &&
           top->caller.psw_address.lo==0x00abcdefU);
    assert(stack.frame[0].state==TSV_SUSPENDED);
    assert(TSVHAS(&stack,child,TSV_FILE,100U)==TSV_STALE);
    assert(TSVHAS(&stack,parent,TSV_FILE,100U)==TSV_BUSY);
    assert(TSVOWN(&stack,parent,TSV_FILE,102U)==TSV_BUSY);
    assert(TSVCOMPLETE(&stack,parent,101U)==TSV_OK);
    assert(TSVCOMPLETE(&stack,parent,101U)==TSV_STALE);
    assert(TSVOWN(&stack,child,TSV_ALLOCATION,200U)==TSV_OK);
    assert(TSVHAS(&stack,child,TSV_ALLOCATION,200U)==TSV_OK);
    assert(TSVOWN(&stack,child,TSV_IO,201U)==TSV_OK);
    assert(TSVEND(&stack,child,clean_owned,&trace)==TSV_BUSY);
    assert(TSVTOP(&stack)==0 && stack.frame[0].state==TSV_SUSPENDED);
    assert(stack.frame[1].state==TSV_REAPING);
    assert(TSVBEGIN(&stack,TSV_TSO,31U,5U,6U,&child_context,
                    &later)==TSV_BUSY);
    assert(TSVCOMPLETE(&stack,child,201U)==TSV_OK);
    assert(TSVEND(&stack,child,clean_owned,&trace)==TSV_OK);
    assert(trace.count==2U && trace.order[0]==201U &&
           trace.order[1]==200U);
    assert(TSVTOP(&stack) && TSVTOP(&stack)->token==parent &&
           TSVTOP(&stack)->caller.psw_address.hi==1U &&
           TSVTOP(&stack)->caller.psw_address.lo==0x23456780U);
    assert(TSVCOMPLETE(&stack,child,201U)==TSV_STALE);
    assert(TSVBEGIN(&stack,TSV_TSO,31U,5U,6U,&child_context,
                    &later)==TSV_OK && later>child);
    assert(TSVOWN(&stack,later,TSV_IO,201U)==TSV_OK);
    assert(TSVCOMPLETE(&stack,child,201U)==TSV_STALE);
    assert(TSVCOMPLETE(&stack,later,201U)==TSV_OK);
    assert(TSVEND(&stack,later,clean_owned,&trace)==TSV_OK);
    assert(trace.count==3U && trace.order[2]==201U);
    assert(TSVEND(&stack,parent,clean_owned,&trace)==TSV_OK);
    assert(trace.count==5U && trace.order[3]==101U &&
           trace.order[4]==100U);
    assert(TSVTOP(&stack)==0 && stack.depth==0U);
    stack.next_token=0U;
    assert(TSVBEGIN(&stack,TSV_CMS,31U,1U,2U,&parent_context,
                    &parent)==TSV_FULL);
    puts("two-space invocation ownership and late completion pass");
    return 0;
}
