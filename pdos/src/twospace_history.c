/* SPDX-License-Identifier: MIT */
#include "twospace_history.h"
static unsigned int slot(const TPHSTATE *s,unsigned int i)
{return (s->first+i)%s->slots;}
static unsigned int byte(const TPHSTATE *s,const TPHRECORD *r,unsigned int i)
{return s->data[(r->offset+i)%s->capacity];}
static int equal(unsigned int ah,unsigned int al,unsigned int bh,unsigned int bl)
{return ah==bh&&al==bl;}
int TPHINIT(TPHSTATE *s,unsigned char *data,unsigned int bytes,TPHRECORD *records,unsigned int slots)
{
    if(!s||!data||bytes<TPH_LINE_BYTES||!records||!slots)return TPH_BAD;
    s->data=data;s->capacity=bytes;s->record=records;s->slots=slots;
    s->used=s->tail=s->first=s->count=s->evicted=s->sequence_hi=s->sequence_lo=0U;return TPH_OK;
}
int TPHAPPEND(TPHSTATE *s,unsigned int group,unsigned int token,unsigned int type,const unsigned char *data,unsigned int bytes)
{
    TPHRECORD *r;unsigned int i;
    if(!s||!s->data||!s->record||(bytes&&!data)||bytes>TPH_LINE_BYTES||
       s->used>s->capacity||s->count>s->slots||
       (s->sequence_hi==0xffffffffU&&s->sequence_lo==0xffffffffU))return TPH_BAD;
    while(s->count&&(s->count==s->slots||bytes>s->capacity-s->used)){
        r=&s->record[s->first];s->used-=r->bytes;s->first=(s->first+1U)%s->slots;--s->count;
        if(s->evicted!=0xffffffffU)++s->evicted;
    }
    if(bytes>s->capacity-s->used)return TPH_BAD;
    r=&s->record[slot(s,s->count)];r->offset=s->tail;r->bytes=bytes;r->group=group;r->token=token;r->type=type;
    if(++s->sequence_lo==0U)++s->sequence_hi;r->sequence_hi=s->sequence_hi;r->sequence_lo=s->sequence_lo;
    for(i=0U;i<bytes;++i){s->data[s->tail]=data[i];if(++s->tail==s->capacity)s->tail=0U;}
    s->used+=bytes;++s->count;return TPH_OK;
}
void TPHFOLLOW(TPHVIEW *v,unsigned int group)
{if(v){v->group=group;v->follow=1U;v->sequence_hi=v->sequence_lo=v->row=v->evicted=0U;}}
static unsigned int rows(const TPHSTATE *s,const TPHRECORD *r,unsigned int width)
{
    unsigned int i,n=1U,column=0U,c;
    for(i=0U;i<r->bytes;++i){c=byte(s,r,i);
        if(c==0x15U||c==0x25U){if(i+1U<r->bytes)++n;column=0U;}
        else{if(column==width){++n;column=0U;}++column;}
    }
    return n;
}
static int match(const TPHRECORD *r,const TPHVIEW *v)
{return r->type==TPH_TEXT&&r->group==v->group;}
static unsigned int gather(const TPHSTATE *s,TPHVIEW *v,unsigned int width,unsigned int height,TPHROW *out)
{
    unsigned int i,n=0U,j,count,found=0U,at=0U,row=0U;
    if(v->follow){
        for(i=s->count;i&&n<height;){TPHRECORD *r=&s->record[slot(s,--i)];if(!match(r,v))continue;
            count=rows(s,r,width);while(count&&n<height){out[n].slot=slot(s,i);out[n++].row=--count;}}
        for(i=0U;i<n/2U;++i){TPHROW tmp=out[i];out[i]=out[n-1U-i];out[n-1U-i]=tmp;}
        return n;
    }
    for(i=0U;i<s->count;++i){TPHRECORD *r=&s->record[slot(s,i)];if(!match(r,v))continue;
        if(!found){at=i;found=1U;}
        if(equal(r->sequence_hi,r->sequence_lo,v->sequence_hi,v->sequence_lo)){at=i;row=v->row;found=2U;break;}
    }
    if(!found)return 0U;
    if(found!=2U){v->evicted=1U;row=0U;}
    {const TPHRECORD *r=&s->record[slot(s,at)];count=rows(s,r,width);if(row>=count)row=count-1U;v->sequence_hi=r->sequence_hi;v->sequence_lo=r->sequence_lo;v->row=row;}
    for(i=at;i<s->count&&n<height;++i){TPHRECORD *r=&s->record[slot(s,i)];if(!match(r,v))continue;
        count=rows(s,r,width);for(j=row;j<count&&n<height;++j){out[n].slot=slot(s,i);out[n++].row=j;}row=0U;}
    return n;
}
static void render_row(const TPHSTATE *s,const TPHRECORD *r,unsigned int row,unsigned int width,unsigned char *out)
{
    unsigned int i,column=0U,current=0U,c;
    for(i=0U;i<r->bytes;++i){c=byte(s,r,i);
        if(c==0x15U||c==0x25U){if(current==row)break;++current;column=0U;continue;}
        if(column==width){if(current==row)break;++current;column=0U;}
        if(current==row)out[column]=(unsigned char)(c<0x40U?0x40U:c);++column;
    }
}
int TPHRENDER(const TPHSTATE *s,TPHVIEW *v,unsigned int width,unsigned int height,unsigned char *out,unsigned int capacity,unsigned int *used)
{
    TPHROW row[TPH_VIEW_ROWS];unsigned int i,n;
    if(!s||!s->data||!v||!width||!height||height>TPH_VIEW_ROWS||!out||!used||height>capacity/width)return TPH_BAD;
    for(i=0U;i<width*height;++i)out[i]=0x40U;
    n=gather(s,v,width,height,row);
    for(i=0U;i<n;++i)render_row(s,&s->record[row[i].slot],row[i].row,width,out+i*width);
    *used=n;return TPH_OK;
}
int TPHCOMPACT(const TPHSTATE *s,const TPHVIEW *v,unsigned int width,unsigned int height,unsigned char *out,unsigned int capacity,unsigned int *used)
{
    unsigned int selected[TPH_VIEW_ROWS],i,j,n=0U,bytes,c;const TPHRECORD *r;
    if(!s||!s->data||!v||!width||!height||height>TPH_VIEW_ROWS||!out||!used||height>capacity/width)return TPH_BAD;
    for(i=0U;i<width*height;++i)out[i]=0x40U;
    for(i=s->count;i&&n<height;){--i;r=&s->record[slot(s,i)];if(match(r,v))selected[n++]=slot(s,i);}
    for(i=0U;i<n;++i){r=&s->record[selected[n-1U-i]];bytes=r->bytes<width?r->bytes:width;
        for(j=0U;j<bytes;++j){c=byte(s,r,j);out[i*width+j]=(unsigned char)(c<0x40U?0x40U:c);}
        if(r->bytes>width&&width>=3U)out[(i+1U)*width-1U]=out[(i+1U)*width-2U]=out[(i+1U)*width-3U]=0x4bU;
    }
    *used=n;return TPH_OK;
}
int TPHPAGE(const TPHSTATE *s,TPHVIEW *v,unsigned int width,unsigned int height,int direction)
{
    TPHROW visible[TPH_VIEW_ROWS];unsigned int n,i,at=0U,row,remaining;
    if(!s||!v||!width||!height||height>TPH_VIEW_ROWS||(direction!=-1&&direction!=1))return TPH_BAD;
    n=gather(s,v,width,height,visible);if(!n)return TPH_EMPTY;
    if(direction>0&&v->follow)return TPH_OK;
    for(i=0U;i<s->count;++i)if(slot(s,i)==visible[0U].slot){at=i;break;}
    row=visible[0U].row;remaining=height;
    while(remaining){
        if(direction<0){
            if(row){--row;--remaining;continue;}
            while(at&& !match(&s->record[slot(s,at-1U)],v))--at;
            if(!at)break;--at;row=rows(s,&s->record[slot(s,at)],width)-1U;--remaining;
        }else{
            if(row+1U<rows(s,&s->record[slot(s,at)],width)){++row;--remaining;continue;}
            do{++at;}while(at<s->count&&!match(&s->record[slot(s,at)],v));
            if(at==s->count){v->follow=1U;return TPH_OK;}row=0U;--remaining;
        }
    }
    v->follow=0U;v->sequence_hi=s->record[slot(s,at)].sequence_hi;v->sequence_lo=s->record[slot(s,at)].sequence_lo;v->row=row;return TPH_OK;
}
