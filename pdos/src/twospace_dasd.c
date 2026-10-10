/* SPDX-License-Identifier: MIT; allocation, not logical file content sizes. */
#include "twospace_dasd.h"
static unsigned int half(const unsigned char *p)
{return ((unsigned int)p[0]<<8)|p[1];}
int TDAGEOM(const unsigned char *f4,unsigned int bytes,unsigned int *cylinders,
            unsigned int *tracks)
{
    unsigned int n;
    if(!f4||!cylinders||!tracks||cylinders==tracks||bytes!=140U||f4[44U]!=0xf4U||
       half(f4+64U)!=TDA_HEADS||half(f4+107U)!=1U||half(f4+109U)||
       half(f4+111U)!=2U||half(f4+113U)!=14U)return -1;
    n=half(f4+62U);if(n<3U||n>TDA_MAX_CYLINDERS)return -1;
    *cylinders=n;*tracks=n*TDA_HEADS;return 0;
}
static int geometry(const TDAMAP *s)
{
    return s&&s->cylinders>=3U&&s->cylinders<=TDA_MAX_CYLINDERS&&
        s->heads==TDA_HEADS&&s->tracks==s->cylinders*s->heads;
}
int TDARANGE(const TDAMAP *s,unsigned int fc,unsigned int fh,
             unsigned int lc,unsigned int lh,unsigned int *first,unsigned int *last)
{
    unsigned int a,b;
    if(!geometry(s)||!first||!last||first==last||fc>=s->cylinders||
       lc>=s->cylinders||fh>=s->heads||lh>=s->heads)return -1;
    a=fc*s->heads+fh;b=lc*s->heads+lh;if(a>b)return -1;
    *first=a;*last=b;return 0;
}
int TDAINIT(TDAMAP *s,unsigned char *map,unsigned int capacity,const unsigned char *f4,unsigned int bytes)
{
    unsigned int i,cylinders,tracks;
    if(!s||!map||TDAGEOM(f4,bytes,&cylinders,&tracks)||capacity<tracks)return -1;
    s->map=map;s->tracks=tracks;s->cylinders=cylinders;s->heads=TDA_HEADS;
    s->reserved=s->allocated=s->free=s->largest=s->datasets=0U;
    for(i=0U;i<tracks;++i)map[i]=(unsigned char)(i<45U?TDA_RESERVED:0U);
    return 0;
}
int TDAADD(TDAMAP *s,const unsigned char *f1,unsigned int bytes)
{
    unsigned int i,j,n,first[3],last[3];const unsigned char *p;
    if(!geometry(s)||!s->map||!f1||bytes!=140U||f1[44U]!=0xf1U||
       !f1[59U]||f1[59U]>3U)return -1;
    n=f1[59U];
    for(i=0U;i<n;++i){
        p=f1+105U+10U*i;
        if((p[0U]!=1U&&p[0U]!=0x81U)||TDARANGE(s,half(p+2U),half(p+4U),
           half(p+6U),half(p+8U),&first[i],&last[i]))return -1;
    }
    /* Validate every extent before publishing any cells. Extent unions count
     * shared/overlapping descriptions once instead of inflating used space. */
    for(i=0U;i<n;++i)for(j=first[i];j<=last[i];++j)s->map[j]|=TDA_ALLOCATED;
    ++s->datasets;return 0;
}
int TDASTAT(TDAMAP *s)
{
    unsigned int i,run=0U;
    if(!geometry(s)||!s->map)return -1;
    s->reserved=s->allocated=s->free=s->largest=0U;
    for(i=0U;i<s->tracks;++i){
        if(s->map[i]&TDA_RESERVED){++s->reserved;run=0U;}
        else if(s->map[i]&TDA_ALLOCATED){++s->allocated;run=0U;}
        else{++s->free;if(++run>s->largest)s->largest=run;}
    }
    return 0;
}
