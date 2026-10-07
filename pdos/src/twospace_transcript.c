/* SPDX-License-Identifier: MIT */
#include "twospace_transcript.h"
static void put(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
int TTXINIT(TTXSTATE *s,unsigned char *data,unsigned int capacity)
{
    if(!s||!data||capacity<64U)return -1;
    s->data=data;s->capacity=capacity;s->used=s->sequence_hi=s->sequence_lo=s->gaps=s->full=0U;
    return 0;
}
int TTXAPPEND(TTXSTATE *s,unsigned int type,unsigned int token,
               unsigned int encoding,unsigned int flags,const unsigned char *payload,
               unsigned int length)
{
    unsigned char *p;unsigned int i;
    if(!s||!s->data||(length&&!payload)||type<TSA_TRANSCRIPT_COMMAND_BEGIN||
       type>TSA_TRANSCRIPT_GAP||s->used>s->capacity||
       (s->sequence_hi==0xffffffffU&&s->sequence_lo==0xffffffffU))return -1;
    if(s->full)return -2;
    if(s->capacity-s->used<32U)return -1;
    if(length>s->capacity-s->used-32U||
       (type!=TSA_TRANSCRIPT_GAP&&length+64U>s->capacity-s->used)){
        /* Preserve a final gap record; never wrap or silently discard text. */
        if(s->capacity-s->used<32U)return -1;
        type=TSA_TRANSCRIPT_GAP;length=0U;flags=TSA_OS_TRANSCRIPT_GAP;s->full=1U;
    }
    if(++s->sequence_lo==0U)++s->sequence_hi;
    p=s->data+s->used;
    for(i=0U;i<32U;++i)p[i]=0U;
    put(p,TSA_TRANSCRIPT_VERSION);put(p+4U,type);
    put(p+8U,s->sequence_hi);put(p+12U,s->sequence_lo);
    put(p+16U,length);put(p+20U,token);put(p+24U,encoding);put(p+28U,flags);
    for(i=0U;i<length;++i)p[32U+i]=payload[i];
    s->used+=32U+length;
    if(type==TSA_TRANSCRIPT_GAP)++s->gaps;
    return s->full?-2:0;
}
