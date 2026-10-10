/* SPDX-License-Identifier: MIT; logical records never pass through text stdio. */
#include <string.h>
#include "edit_core.h"
unsigned int EDCLOAD(EDCSTATE *s,const unsigned char *p,unsigned int bytes)
{
    unsigned int at=0U,n,count=0U;
    while(at<bytes){if(bytes-at<2U||count==EDC_LINES)return 8U;
        n=((unsigned int)p[at]<<8)|p[at+1U];at+=2U;
        if(n>EDC_WIDTH||n>bytes-at)return 8U;at+=n;++count;}
    s->count=count;s->current=count?1U:0U;at=0U;count=0U;
    while(at<bytes){n=((unsigned int)p[at]<<8)|p[at+1U];at+=2U;
        s->line[count].length=n;memcpy(s->line[count++].text,p+at,n);at+=n;}
    return 0U;
}
unsigned int EDCPACK(const EDCSTATE *s,unsigned char *p,unsigned int cap,
                     unsigned int format,unsigned int lrecl,unsigned int *bytes)
{
    unsigned int i,n,used=0U,fixed=(format&0x80U)!=0U;
    if(!bytes||!lrecl||(!fixed&&lrecl<4U))return 8U;
    for(i=0U;i<s->count;++i){n=s->line[i].length;
        if(n>(fixed?lrecl:lrecl-4U))return 8U;
        if(fixed)n=lrecl;if(n+2U>cap-used)return 8U;used+=n+2U;}
    used=0U;
    for(i=0U;i<s->count;++i){n=fixed?lrecl:s->line[i].length;
        p[used++]=(unsigned char)(n>>8);p[used++]=(unsigned char)n;
        memcpy(p+used,s->line[i].text,s->line[i].length);
        if(fixed&&n>s->line[i].length)memset(p+used+s->line[i].length,' ',n-s->line[i].length);
        used+=n;}
    *bytes=used;return 0U;
}
unsigned int EDCINSERT(EDCSTATE *s,unsigned int after,const unsigned char *p,unsigned int n)
{
    unsigned int i;
    if(after>s->count||s->count==EDC_LINES||n>EDC_WIDTH)return 8U;
    for(i=s->count;i>after;--i)s->line[i]=s->line[i-1U];
    s->line[after].length=n;memcpy(s->line[after].text,p,n);++s->count;s->current=after+1U;return 0U;
}
unsigned int EDCDELETE(EDCSTATE *s,unsigned int first,unsigned int last)
{
    unsigned int i,n;
    if(!first||first>last||last>s->count)return 8U;n=last-first+1U;
    for(i=first-1U;i+n<s->count;++i)s->line[i]=s->line[i+n];s->count-=n;
    s->current=first>s->count?s->count:first;return 0U;
}
unsigned int EDCFIND(const EDCLINE *line,const unsigned char *p,unsigned int n,unsigned int *at)
{
    unsigned int i;if(!n||n>line->length)return 4U;
    for(i=0U;i<=line->length-n;++i)if(!memcmp(line->text+i,p,n)){*at=i;return 0U;}return 4U;
}
unsigned int EDCCHANGE(EDCSTATE *s,unsigned int number,const unsigned char *from,
                      unsigned int old,const unsigned char *to,unsigned int n)
{
    unsigned int at,length;EDCLINE *line;
    if(!number||number>s->count||n>EDC_WIDTH)return 8U;line=&s->line[number-1U];
    if(EDCFIND(line,from,old,&at))return 4U;
    length=line->length-old+n;if(length>EDC_WIDTH)return 8U;
    memmove(line->text+at+n,line->text+at+old,line->length-at-old);memcpy(line->text+at,to,n);
    line->length=length;s->current=number;return 0U;
}
int EDCEQUAL(const EDCSTATE *a,const EDCSTATE *b)
{
    unsigned int i;if(a->count!=b->count)return 0;
    for(i=0U;i<a->count;++i){
        if(a->line[i].length!=b->line[i].length||
           memcmp(a->line[i].text,b->line[i].text,a->line[i].length))return 0;
    }
    return 1;
}
