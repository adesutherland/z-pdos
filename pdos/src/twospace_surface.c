/* SPDX-License-Identifier: MIT; IBM GA23-0059-07 field and character attributes. */
#include "twospace_surface.h"
static int surface(const TWFSURFACE *s)
{T27GEOMETRY g;return s&&s->text&&s->kind&&s->role&&s->geometry.cells<=TWF_MAX_CELLS&&
 T27GEOM(&g,s->geometry.rows,s->geometry.columns,s->geometry.addressing)==T27_OK&&g.cells==s->geometry.cells;}
int TWFINIT(TWFSURFACE *s,const T27GEOMETRY *g,unsigned char *text,unsigned char *kind,unsigned char *role)
{
    unsigned int i;T27GEOMETRY checked;
    if(!s||!g||!text||!kind||!role||g->cells>TWF_MAX_CELLS||T27GEOM(&checked,g->rows,g->columns,g->addressing)||checked.cells!=g->cells)return -1;
    s->geometry=*g;s->text=text;s->kind=kind;s->role=role;s->input=s->capacity=s->cursor=s->cursor_valid=0U;
    for(i=0U;i<g->cells;++i){text[i]=0x40U;kind[i]=i%g->columns?TWF_DATA:TWF_ATTRIBUTE;role[i]=TWF_NORMAL;}
    return 0;
}
int TWFINPUT(TWFSURFACE *s,unsigned int address,unsigned int bytes)
{
    unsigned int i;
    if(!surface(s)||!address||!bytes||address>=s->geometry.cells||bytes>=s->geometry.cells-address||s->capacity)return -1;
    s->input=address;s->capacity=bytes;s->cursor=address;s->cursor_valid=1U;
    s->kind[address-1U]=TWF_ATTRIBUTE;s->text[address-1U]=0U;s->role[address-1U]=TWF_ACTION;
    for(i=0U;i<bytes;++i){s->kind[address+i]=TWF_INPUT;s->text[address+i]=0U;s->role[address+i]=TWF_ACTION;}
    s->kind[address+bytes]=TWF_ATTRIBUTE;s->role[address+bytes]=TWF_NORMAL;return 0;
}
int TWFTEXT(TWFSURFACE *s,unsigned int row,unsigned int column,unsigned int width,const unsigned char *text,unsigned int bytes,unsigned int role)
{
    unsigned int i,at;
    if(!surface(s)||row>=s->geometry.rows||column>=s->geometry.columns||!width||width>s->geometry.columns-column||
       bytes>width||(bytes&&!text)||role>TWF_FOCUS)return -1;
    at=row*s->geometry.columns+column;
    for(i=0U;i<width;++i)if(s->kind[at+i]!=TWF_DATA)return -1;
    for(i=0U;i<bytes;++i)if(text[i]<0x40U||text[i]==0xffU)return -1;
    for(i=0U;i<width;++i){s->text[at+i]=i<bytes?text[i]:0x40U;s->role[at+i]=(unsigned char)role;}
    return 0;
}
int TWFRECT(TWFSURFACE *s,unsigned int row,unsigned int column,unsigned int height,unsigned int width,unsigned int role)
{
    unsigned int r,c,at;
    if(!surface(s)||row>=s->geometry.rows||column>=s->geometry.columns||!height||height>s->geometry.rows-row||
       !width||width>s->geometry.columns-column||role>TWF_FOCUS)return -1;
    for(r=0U;r<height;++r)for(c=0U;c<width;++c){at=(row+r)*s->geometry.columns+column+c;if(s->kind[at]!=TWF_DATA)return -1;}
    for(r=0U;r<height;++r)for(c=0U;c<width;++c){at=(row+r)*s->geometry.columns+column+c;s->text[at]=0x40U;s->role[at]=(unsigned char)role;}
    return 0;
}
static int changed(const TWFSURFACE *s,const TWFSURFACE *old,unsigned int i)
{return !old||s->text[i]!=old->text[i]||s->role[i]!=old->role[i]||s->kind[i]!=old->kind[i];}
static unsigned int colour(const TWFPALETTE *p,unsigned int role)
{
    unsigned int id=role==TWF_HEADER?5U:role==TWF_SHELL?4U:role==TWF_ACTION||role==TWF_FOCUS?6U:role==TWF_ERROR?2U:7U;
    return p&&(p->valid&(1U<<id))?p->colour[id]:0U;
}
static int colour_present(const TWFPALETTE *p,unsigned int role)
{unsigned int id=role==TWF_HEADER?5U:role==TWF_SHELL?4U:role==TWF_ACTION||role==TWF_FOCUS?6U:role==TWF_ERROR?2U:7U;return p&&(p->valid&(1U<<id));}
static unsigned int basic(const TWFSURFACE *s,unsigned int i)
{unsigned int j;if(s->capacity&&i==s->input-1U){for(j=0U;j<s->capacity;++j)if(s->text[s->input+j])return 1U;return 0U;}return s->role[i]==TWF_HEADER||s->role[i]==TWF_ACTION||s->role[i]==TWF_FOCUS?0x38U:0x30U;}
typedef struct {unsigned char *out;unsigned int at,capacity;} OUTPUT;
static int emit(OUTPUT *o,unsigned int byte)
{if(o->at==o->capacity)return -1;if(o->out)o->out[o->at]=(unsigned char)byte;++o->at;return 0;}
static int address(OUTPUT *o,const T27GEOMETRY *g,unsigned int i)
{unsigned char a[2];if(T27ADDR(g,i,a)||emit(o,0x11U)||emit(o,a[0U])||emit(o,a[1U]))return -1;return 0;}
static int attribute(OUTPUT *o,const TWFSURFACE *s,unsigned int i,const TWFPALETTE *p)
{
    T27GEOMETRY g;unsigned char a[2];unsigned int value=basic(s,i),c=colour(p,s->role[i]);
    if(T27GEOM(&g,1U,64U,T27_CODED12)||T27ADDR(&g,value,a))return -1;
    if(colour_present(p,s->role[i])){if(emit(o,0x29U)||emit(o,2U)||emit(o,0xc0U)||emit(o,a[1U])||emit(o,0x42U)||emit(o,c))return -1;}
    else if(emit(o,0x1dU)||emit(o,a[1U]))return -1;
    return 0;
}
static int encode(const TWFSURFACE *s,const TWFSURFACE *old,unsigned int full,const TWFPALETTE *p,OUTPUT *o)
{
    unsigned int i,base=0U,last_base=0xffffffffU,at=0xffffffffU,role=0xffffffffU,c,last_c=0xffffffffU,highlight,last_highlight=0xffffffffU;
    if(emit(o,full?0xc3U:0x40U))return -1;
    for(i=0U;i<s->geometry.cells;++i){
        if(s->kind[i]==TWF_ATTRIBUTE)base=i;
        if(s->kind[i]==TWF_INPUT&&(!full||!s->text[i]))continue;
        if(!full&&!changed(s,old,i))continue;
        /* Establish the governing protected field in each delta record. Some
         * clients keep stale write attributes after SBA; input FA/MDT is never
         * touched. Character attributes are explicit after every position. */
        if(!full&&s->kind[i]!=TWF_ATTRIBUTE&&base!=last_base){
            if(s->capacity&&base==s->input-1U)return -1;
            if(address(o,&s->geometry,base)||attribute(o,s,base,p))return -1;
            at=base+1U;last_base=base;role=last_c=last_highlight=0xffffffffU;
        }
        if(at!=i){if(address(o,&s->geometry,i))return -1;at=i;}
        if(s->kind[i]==TWF_ATTRIBUTE){if(attribute(o,s,i,p))return -1;++at;last_base=i;role=last_c=last_highlight=0xffffffffU;continue;}
        if(role!=s->role[i]){
            c=colour(p,s->role[i]);highlight=p&&p->highlighting&2U&&s->role[i]==TWF_FOCUS?0xf2U:0U;
            if(c!=last_c&&p&&p->valid){if(emit(o,0x28U)||emit(o,0x42U)||emit(o,c))return -1;last_c=c;}
            if(highlight!=last_highlight&&p&&p->highlighting){if(emit(o,0x28U)||emit(o,0x41U)||emit(o,highlight))return -1;last_highlight=highlight;}
            role=s->role[i];
        }
        if(emit(o,s->text[i]))return -1;++at;
    }
    if(full&&s->cursor_valid){if(address(o,&s->geometry,s->cursor)||emit(o,0x13U))return -1;}
    return 0;
}
int TWFENCODE(const TWFSURFACE *s,const TWFSURFACE *old,unsigned int full,const TWFPALETTE *p,unsigned char *out,unsigned int capacity,unsigned int *length)
{
    OUTPUT o;unsigned int i,any=full;
    if(!surface(s)||!out||!length||full>1U||(!full&&(!surface(old)||old->geometry.cells!=s->geometry.cells||old->geometry.rows!=s->geometry.rows||old->geometry.columns!=s->geometry.columns||old->input!=s->input||old->capacity!=s->capacity)))return -1;
    if(s->capacity&&(s->input==0U||s->input>=s->geometry.cells||s->capacity>=s->geometry.cells-s->input||s->kind[s->input-1U]!=TWF_ATTRIBUTE||s->kind[s->input+s->capacity]!=TWF_ATTRIBUTE))return -1;
    if(!full&&s->capacity&&(changed(s,old,s->input-1U)||changed(s,old,s->input+s->capacity)))return -1;
    if(s->cursor_valid&&s->cursor>=s->geometry.cells)return -1;
    for(i=0U;i<s->geometry.cells;++i){
        if(s->kind[i]<TWF_DATA||s->kind[i]>TWF_INPUT||s->role[i]>TWF_FOCUS)return -1;
        if(s->kind[i]==TWF_INPUT){if(i<s->input||i-s->input>=s->capacity||(s->text[i]&&(s->text[i]<0x40U||s->text[i]==0xffU)))return -1;}
        else if(i>=s->input&&s->capacity&&i-s->input<s->capacity)return -1;
        if(s->kind[i]==TWF_DATA&&(s->text[i]<0x40U||s->text[i]==0xffU))return -1;
        if(!full&&s->kind[i]!=TWF_INPUT&&changed(s,old,i))any=1U;
    }
    if(!any){*length=0U;return 0;}
    o.out=0;o.at=0U;o.capacity=capacity;if(encode(s,old,full,p,&o))return -1;
    o.out=out;o.at=0U;if(encode(s,old,full,p,&o))return -1;
    *length=o.at;return 0;
}
int TWFCOMMIT(TWFSURFACE *target,const TWFSURFACE *source)
{
    unsigned int i;
    if(!surface(target)||!surface(source)||target->geometry.cells!=source->geometry.cells)return -1;
    target->geometry=source->geometry;target->input=source->input;target->capacity=source->capacity;
    for(i=0U;i<source->geometry.cells;++i){target->text[i]=source->text[i];target->kind[i]=source->kind[i];target->role[i]=source->role[i];}
    return 0;
}
