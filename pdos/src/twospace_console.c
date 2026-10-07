/* SPDX-License-Identifier: MIT
 * IBM 3270 basic SF/SBA/IC/RA coding; see TWO-SPACE-ABI.md's primary sources.
 * Screen dimensions are validated configuration, never an assumed 24x80. */
#include "twospace_console.h"
static const unsigned char coded[64]={
 0x40,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
 0x50,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
 0x60,0x61,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
 0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f};
static void put(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static int valid(const TTCCAP *c)
{return c&&c->device_class==TSA_DEVICE_3270&&c->rows&&c->columns&&
        c->rows*c->columns<=TTC_MAX_CELLS&&c->encoding==TSA_ADDRESS_CODED12;}
int TTCCONFIG(TTCCAP *c,unsigned int type,unsigned int address,
               unsigned int model,unsigned int monitor)
{
    unsigned int rows,columns;
    if(!c||!address||address>65535U||monitor>65535U||monitor==address)return TTC_BAD;
    rows=columns=0U;
    if(type==TSA_DEVICE_3270){
        columns=model==5U?132U:80U;
        rows=model==2U?24U:model==3U?32U:model==4U?43U:model==5U?27U:0U;
        if(!rows)return TTC_UNSUPPORTED;
    }else if(type!=TSA_DEVICE_LINE||model)return TTC_UNSUPPORTED;
    c->device_class=type;c->address=address;c->model=model;c->rows=rows;c->columns=columns;
    c->encoding=type==TSA_DEVICE_3270?TSA_ADDRESS_CODED12:0U;c->generation=1U;
    c->monitor=monitor;
    c->features=TSA_FEATURE_LINE_INPUT|TSA_FEATURE_LINE_OUTPUT|TSA_FEATURE_TRANSCRIPT;
    if(type==TSA_DEVICE_3270)c->features|=TSA_FEATURE_SCREEN;
    if(monitor)c->features|=TSA_FEATURE_MONITOR|TSA_FEATURE_INPUT_HANDOFF;
    return TTC_OK;
}
int TTCCAPS(const TTCCAP *c,unsigned char *out)
{
    unsigned int i;
    if(!c||!out)return TTC_BAD;
    for(i=0U;i<TSA_CAP_BYTES;++i)out[i]=0U;
    put(out+TSA_CAP_VERSION,TSA_VERSION);put(out+TSA_CAP_LENGTH,TSA_CAP_BYTES);
    put(out+TSA_CAP_DEVICE_CLASS,c->device_class);put(out+TSA_CAP_DEVICE_ADDRESS,c->address);
    put(out+TSA_CAP_ROWS,c->rows);put(out+TSA_CAP_COLUMNS,c->columns);
    if(c->device_class==TSA_DEVICE_3270){
        put(out+TSA_CAP_DEFAULT_ROWS,24U);put(out+TSA_CAP_DEFAULT_COLUMNS,80U);
        put(out+TSA_CAP_ALTERNATE_ROWS,c->rows);put(out+TSA_CAP_ALTERNATE_COLUMNS,c->columns);
        put(out+TSA_CAP_ATTRIBUTES,TTC_BASIC_ATTRIBUTES);
    }
    put(out+TSA_CAP_ADDRESS_FORMAT,c->encoding);put(out+TSA_CAP_FEATURES,c->features);
    put(out+TSA_CAP_LINE_LIMIT,c->device_class==TSA_DEVICE_LINE?148U:256U);
    put(out+TSA_CAP_SCREEN_LIMIT,valid(c)?TTC_MAX_STREAM:0U);
    put(out+TSA_CAP_GENERATION,c->generation);return TTC_OK;
}
int TTCADDR(const TTCCAP *c,unsigned int address,unsigned char *out)
{
    if(!valid(c)||!out||address>=c->rows*c->columns)return TTC_BAD;
    out[0]=coded[address>>6];out[1]=coded[address&63U];return TTC_OK;
}
int TTCDECODE(const TTCCAP *c,const unsigned char *input,unsigned int *address)
{
    unsigned int a,b,i;
    if(!valid(c)||!input||!address)return TTC_BAD;
    a=b=64U;
    if(!(input[0]&0xc0U)){
        *address=((unsigned int)(input[0]&0x3fU)<<8)|input[1];
    }else{
        for(i=0U;i<64U;++i){if(coded[i]==input[0])a=i;if(coded[i]==input[1])b=i;}
        if(a==64U||b==64U)return TTC_BAD;
        *address=a*64U+b;
    }
    return *address<c->rows*c->columns?TTC_OK:TTC_BAD;
}
int TTCENCODE(const TTCCAP *c,const TTCFIELD *fields,unsigned int count,
               unsigned int cr,unsigned int cc,unsigned char *out,
               unsigned int capacity,unsigned int *length)
{
    unsigned int i,j,k,address,total,needed=5U,positions[64],ends[64],at;
    if(!valid(c)||!fields||!count||count>TSA_SCREEN_FIELDS||!out||!length||
       cr>=c->rows||cc>=c->columns)return TTC_BAD;
    total=c->rows*c->columns;
    for(i=0U;i<count;++i){
        const TTCFIELD *f=&fields[i];
        if(f->row>=c->rows||f->column>=c->columns||
           (f->attributes!=TTC_INPUT&&f->attributes!=TTC_PROTECTED&&f->attributes!=TTC_BRIGHT))return TTC_BAD;
        address=f->row*c->columns+f->column;
        if(f->length>total-address||(f->length&&!f->text))return TTC_BAD;
        positions[i]=address?address-1U:total-1U;ends[i]=address+f->length;
        for(j=0U;j<f->length;++j)if(f->text[j]<0x40U)return TTC_BAD;
        for(j=0U;j<i;++j){
            unsigned int other=fields[j].row*c->columns+fields[j].column;
            if(positions[i]==positions[j]||
               (positions[i]>=other&&positions[i]<ends[j])||
               (positions[j]>=address&&positions[j]<ends[i])||
               (address<ends[j]&&other<ends[i]))return TTC_BAD;
        }
        if(needed>TTC_MAX_STREAM-5U-f->length)return TTC_BAD;
        needed+=5U+f->length;
    }
    if(needed>capacity)return TTC_BAD;
    at=0U;out[at++]=0xc3U;
    for(i=0U;i<count;++i){
        out[at++]=0x11U;TTCADDR(c,positions[i],out+at);at+=2U;
        out[at++]=0x1dU;out[at++]=coded[fields[i].attributes];
        for(k=0U;k<fields[i].length;++k)out[at++]=fields[i].text[k];
    }
    out[at++]=0x11U;TTCADDR(c,cr*c->columns+cc,out+at);at+=2U;out[at++]=0x13U;
    *length=at;return TTC_OK;
}
int TTCRAW(const TTCCAP *c,const unsigned char *raw,unsigned int length,unsigned int *command)
{
    unsigned int at=3U,address=0U,n,target;
    if(!valid(c)||!raw||!command||length<3U||length>TTC_MAX_STREAM||raw[0]!=0x27U)return TTC_BAD;
    if(raw[1]==0xf1U)*command=1U;
    else if(raw[1]==0xf5U)*command=5U;
    else if(raw[1]==0x7eU)*command=13U;
    else return TTC_UNSUPPORTED;
    while(at<length){
        n=raw[at++];
        if(n==0x11U){
            if(length-at<2U||TTCDECODE(c,raw+at,&address)!=TTC_OK)return TTC_BAD;at+=2U;
        }else if(n==0x1dU){
            if(at==length)return TTC_BAD;++at;address=(address+1U)%(c->rows*c->columns);
        }else if(n==0x13U){
            /* IC consumes no buffer position. */
        }else if(n==0x3cU){
            if(length-at<3U||TTCDECODE(c,raw+at,&target)!=TTC_OK||raw[at+2U]<0x40U)return TTC_BAD;
            at+=3U;address=target;
        }else{
            if(n<0x40U&&n!=0U)return TTC_UNSUPPORTED;
            address=(address+1U)%(c->rows*c->columns);
        }
    }
    return TTC_OK;
}
int TTCINPUT(const TTCCAP *c,const unsigned char *in,unsigned int bytes,
              unsigned int field,unsigned char *out,unsigned int capacity,
              unsigned int *length,unsigned int *aid)
{
    unsigned int cursor,address,n,i;
    if(!valid(c)||!in||bytes<3U||!out||!length||!aid||
       TTCDECODE(c,in+1U,&cursor)!=TTC_OK)return TTC_BAD;
    *aid=in[0U];
    if(bytes==3U){*length=0U;return TTC_OK;}
    if(bytes<6U||in[3U]!=0x11U||TTCDECODE(c,in+4U,&address)!=TTC_OK||address!=field)return TTC_BAD;
    n=bytes-6U;if(n>capacity)return TTC_BAD;
    for(i=0U;i<n;++i)if(in[6U+i]==0x11U)return TTC_UNSUPPORTED;
    for(i=0U;i<n;++i)out[i]=in[6U+i];
    *length=n;return TTC_OK;
}
int TTCVINIT(TTCVIEW *v,const TTCCAP *c)
{
    unsigned int i;
    if(!v||!valid(c)||c->rows<8U)return TTC_BAD;
    v->cap=*c;v->body_row=1U;v->body_rows=c->rows-7U;v->entry_row=c->rows-4U;
    v->entry_capacity=256U;v->row=v->column=0U;
    for(i=0U;i<TTC_MAX_CELLS;++i)v->cells[i]=0x40U;
    return TTC_OK;
}
static void next(TTCVIEW *v)
{
    unsigned int i,n=v->body_rows*v->cap.columns;
    v->column=0U;++v->row;
    if(v->row==v->body_rows){
        for(i=0U;i<n-v->cap.columns;++i)v->cells[i]=v->cells[i+v->cap.columns];
        for(;i<n;++i)v->cells[i]=0x40U;--v->row;
    }
}
int TTCVLINE(TTCVIEW *v,const unsigned char *text,unsigned int length)
{
    unsigned int i;
    if(!v||!valid(&v->cap)||(length&&!text))return TTC_BAD;
    for(i=0U;i<length;++i){
        unsigned int ch=text[i];
        if(ch==0x15U||ch==0x25U){next(v);continue;}
        if(ch==0x0dU){v->column=0U;continue;}
        if(ch<0x40U)ch=0x40U;
        if(v->column==v->cap.columns)next(v);
        v->cells[v->row*v->cap.columns+v->column++]=(unsigned char)ch;
    }
    if(!length||text[length-1U]!=0x15U)next(v);
    return TTC_OK;
}
int TTCVSCREEN(const TTCVIEW *v,const unsigned char *header,unsigned int hb,
                const unsigned char *footer,unsigned int fb,unsigned char *out,
                unsigned int capacity,unsigned int *length)
{
    TTCFIELD fields[4];unsigned int cols;
    if(!v||!valid(&v->cap))return TTC_BAD;cols=v->cap.columns;
    fields[0].row=0U;fields[0].column=0U;fields[0].attributes=TTC_BRIGHT;
    fields[0].length=hb<cols?hb:cols-1U;fields[0].text=header;
    fields[1].row=v->body_row;fields[1].column=0U;fields[1].attributes=TTC_PROTECTED;
    fields[1].length=v->body_rows*cols;fields[1].text=v->cells;
    fields[2].row=v->entry_row-1U;fields[2].column=0U;fields[2].attributes=TTC_PROTECTED;
    fields[2].length=fb<cols?fb:cols-1U;fields[2].text=footer;
    fields[3].row=v->entry_row;fields[3].column=1U;fields[3].attributes=TTC_INPUT;
    fields[3].length=0U;fields[3].text=0;
    return TTCENCODE(&v->cap,fields,4U,v->entry_row,1U,out,capacity,length);
}
