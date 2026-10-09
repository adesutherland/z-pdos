/* SPDX-License-Identifier: MIT; allocation, not logical file content sizes. */
#include "twospace_dasd.h"
static unsigned int half(const unsigned char *p)
{return ((unsigned int)p[0]<<8)|p[1];}
int TDAINIT(TDAMAP *s,unsigned char *map,unsigned int capacity,const unsigned char *f4,unsigned int bytes)
{
    unsigned int i;
    if(!s||!map||!f4||capacity<TDA_TRACKS||bytes!=140U||f4[44U]!=0xf4U||
       half(f4+62U)!=100U||half(f4+64U)!=15U||half(f4+107U)!=1U||half(f4+109U)||
       half(f4+111U)!=2U||half(f4+113U)!=14U)return -1;
    s->map=map;s->tracks=TDA_TRACKS;s->reserved=s->allocated=s->free=s->largest=s->datasets=0U;
    for(i=0U;i<TDA_TRACKS;++i)map[i]=(unsigned char)(i<45U?TDA_RESERVED:0U);
    return 0;
}
int TDAADD(TDAMAP *s,const unsigned char *f1,unsigned int bytes)
{
    unsigned int i,j,n,first[3],last[3];const unsigned char *p;
    if(!s||!s->map||s->tracks!=TDA_TRACKS||!f1||bytes!=140U||f1[44U]!=0xf1U||
       !f1[59U]||f1[59U]>3U)return -1;
    n=f1[59U];
    for(i=0U;i<n;++i){
        p=f1+105U+10U*i;
        if((p[0U]!=1U&&p[0U]!=0x81U)||half(p+2U)>=100U||half(p+4U)>=15U||
           half(p+6U)>=100U||half(p+8U)>=15U)return -1;
        first[i]=half(p+2U)*15U+half(p+4U);last[i]=half(p+6U)*15U+half(p+8U);
        if(first[i]>last[i])return -1;
    }
    /* Validate every extent before publishing any cells. Extent unions count
     * shared/overlapping descriptions once instead of inflating used space. */
    for(i=0U;i<n;++i)for(j=first[i];j<=last[i];++j)s->map[j]|=TDA_ALLOCATED;
    ++s->datasets;return 0;
}
int TDASTAT(TDAMAP *s)
{
    unsigned int i,run=0U;
    if(!s||!s->map||s->tracks!=TDA_TRACKS)return -1;
    s->reserved=s->allocated=s->free=s->largest=0U;
    for(i=0U;i<s->tracks;++i){
        if(s->map[i]&TDA_RESERVED){++s->reserved;run=0U;}
        else if(s->map[i]&TDA_ALLOCATED){++s->allocated;run=0U;}
        else{++s->free;if(++run>s->largest)s->largest=run;}
    }
    return 0;
}
