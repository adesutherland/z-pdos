/* SPDX-License-Identifier: MIT */
#include "twospace_invocation.h"

static void zero_bytes(void *at, unsigned int bytes)
{
    unsigned char *p=(unsigned char *)at;
    unsigned int i;
    for (i=0U; i<bytes; ++i) p[i]=0U;
}

static int valid_mode(unsigned int mode)
{ return mode==24U || mode==31U || mode==64U; }

static TSVFRAME *find_frame(TSVSTACK *stack, unsigned int token)
{
    unsigned int i;
    if (!stack || !token) return 0;
    for (i=0U; i<stack->depth; ++i)
        if (stack->frame[i].token==token) return &stack->frame[i];
    return 0;
}

void TSVINIT(TSVSTACK *stack)
{
    if (!stack) return;
    zero_bytes(stack,(unsigned int)sizeof *stack);
    stack->next_token=1U;
}

int TSVBEGIN(TSVSTACK *stack, unsigned int personality, unsigned int amode,
             unsigned int image_owner, unsigned int runtime_owner,
             const TSVCONTEXT *caller, unsigned int *token)
{
    TSVFRAME *frame;
    unsigned int i;
    if (!stack || !caller || !token || !image_owner || !runtime_owner ||
        (personality!=TSV_CMS && personality!=TSV_TSO) ||
        !valid_mode(amode) || (personality==TSV_CMS && amode==64U))
        return TSV_BAD;
    if (stack->depth>=TSV_MAX_DEPTH || !stack->next_token)
        return TSV_FULL;
    if (stack->depth &&
        stack->frame[stack->depth-1U].state!=TSV_ACTIVE)
        return TSV_BUSY;
    if (stack->depth) stack->frame[stack->depth-1U].state=TSV_SUSPENDED;
    frame=&stack->frame[stack->depth];
    zero_bytes(frame,(unsigned int)sizeof *frame);
    frame->token=stack->next_token++;
    frame->parent=stack->depth ? stack->frame[stack->depth-1U].token : 0U;
    frame->personality=personality;
    frame->amode=amode;
    frame->state=TSV_ACTIVE;
    frame->image_owner=image_owner;
    frame->runtime_owner=runtime_owner;
    for (i=0U; i<16U; ++i) frame->caller.gpr[i]=caller->gpr[i];
    frame->caller.psw=caller->psw;
    frame->caller.psw_address=caller->psw_address;
    frame->caller.asce=caller->asce;
    frame->caller.key=caller->key;
    ++stack->depth;
    *token=frame->token;
    return TSV_OK;
}

const TSVFRAME *TSVTOP(const TSVSTACK *stack)
{
    if (!stack || !stack->depth ||
        stack->frame[stack->depth-1U].state!=TSV_ACTIVE) return 0;
    return &stack->frame[stack->depth-1U];
}

int TSVOWN(TSVSTACK *stack, unsigned int token, unsigned int kind,
           unsigned int handle)
{
    TSVFRAME *frame;
    unsigned int i, j;
    if (!stack || !handle || kind<TSV_IMAGE || kind>TSV_IO)
        return TSV_BAD;
    frame=find_frame(stack,token);
    if (!frame) return TSV_STALE;
    if (frame!=&stack->frame[stack->depth-1U] ||
        frame->state!=TSV_ACTIVE) return TSV_BUSY;
    if (frame->resources>=TSV_MAX_RESOURCES) return TSV_FULL;
    for (i=0U; i<stack->depth; ++i)
        for (j=0U; j<stack->frame[i].resources; ++j)
            if (stack->frame[i].resource[j].kind==kind &&
                stack->frame[i].resource[j].handle==handle) return TSV_BUSY;
    frame->resource[frame->resources].kind=kind;
    frame->resource[frame->resources].handle=handle;
    frame->resource[frame->resources].state=TSV_IO_PENDING;
    ++frame->resources;
    return TSV_OK;
}

int TSVHAS(const TSVSTACK *stack, unsigned int token, unsigned int kind,
           unsigned int handle)
{
    const TSVFRAME *frame;
    unsigned int i;
    if (!stack || !token || !handle || kind<TSV_IMAGE || kind>TSV_IO)
        return TSV_BAD;
    if (!stack->depth) return TSV_STALE;
    frame=&stack->frame[stack->depth-1U];
    if (frame->token!=token) return TSV_BUSY;
    if (frame->state!=TSV_ACTIVE) return TSV_BUSY;
    for (i=0U; i<frame->resources; ++i)
        if (frame->resource[i].kind==kind &&
            frame->resource[i].handle==handle) return TSV_OK;
    return TSV_STALE;
}

int TSVFORGET(TSVSTACK *stack, unsigned int token, unsigned int kind,
              unsigned int handle)
{
    TSVFRAME *frame;
    unsigned int i, j;
    if (!stack || !handle || kind<TSV_IMAGE || kind>TSV_IO)
        return TSV_BAD;
    frame=find_frame(stack,token);
    if (!frame) return TSV_STALE;
    if (frame->state!=TSV_ACTIVE && frame->state!=TSV_REAPING)
        return TSV_BUSY;
    for (i=0U; i<frame->resources; ++i)
        if (frame->resource[i].kind==kind &&
            frame->resource[i].handle==handle) {
            if (kind==TSV_IO &&
                frame->resource[i].state!=TSV_IO_COMPLETE)
                return TSV_BUSY;
            for (j=i+1U; j<frame->resources; ++j)
                frame->resource[j-1U]=frame->resource[j];
            --frame->resources;
            zero_bytes(&frame->resource[frame->resources],
                       (unsigned int)sizeof frame->resource[0]);
            return TSV_OK;
        }
    return TSV_STALE;
}

int TSVCOMPLETE(TSVSTACK *stack, unsigned int token, unsigned int handle)
{
    TSVFRAME *frame;
    unsigned int i;
    if (!stack || !handle) return TSV_BAD;
    frame=find_frame(stack,token);
    if (!frame) return TSV_STALE;
    for (i=0U; i<frame->resources; ++i)
        if (frame->resource[i].kind==TSV_IO &&
            frame->resource[i].handle==handle) {
            if (frame->resource[i].state==TSV_IO_COMPLETE)
                return TSV_STALE;
            frame->resource[i].state=TSV_IO_COMPLETE;
            return TSV_OK;
        }
    return TSV_STALE;
}

int TSVEND(TSVSTACK *stack, unsigned int token, TSVCLEAN clean,
           void *context)
{
    TSVFRAME *frame;
    TSVRESOURCE item;
    if (!stack || !stack->depth || !clean) return TSV_BAD;
    frame=&stack->frame[stack->depth-1U];
    if (frame->token!=token) return TSV_STALE;
    if (frame->state!=TSV_ACTIVE && frame->state!=TSV_REAPING)
        return TSV_BUSY;
    frame->state=TSV_REAPING;
    while (frame->resources) {
        item=frame->resource[frame->resources-1U];
        if (item.kind==TSV_IO && item.state!=TSV_IO_COMPLETE)
            return TSV_BUSY;
        if (clean(token,&item,context)!=0) return TSV_BUSY;
        --frame->resources;
        zero_bytes(&frame->resource[frame->resources],
                   (unsigned int)sizeof frame->resource[0]);
    }
    zero_bytes(frame,(unsigned int)sizeof *frame);
    --stack->depth;
    if (stack->depth)
        stack->frame[stack->depth-1U].state=TSV_ACTIVE;
    return TSV_OK;
}
