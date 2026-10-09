/* SPDX-License-Identifier: MIT; independent wire and failure controls. */
#include "twospace_3270.h"
#include "twospace_console.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void addresses(void)
{
    T27GEOMETRY g,old;
    unsigned char b[2]={0xa5,0xa5};unsigned int n=123U;
    assert(!T27GEOM(&g,24U,80U,T27_CODED12));
    assert(!T27ADDR(&g,1919U,b)&&b[0]==0x5dU&&b[1]==0x7fU);
    assert(!T27DECD(&g,b,&n)&&n==1919U);
    old=g;assert(T27GEOM(&g,0U,80U,T27_CODED12)==T27_BAD&&!memcmp(&g,&old,sizeof g));
    assert(T27GEOM(&g,0xffffffffU,0xffffffffU,T27_BINARY16)==T27_BAD);
    assert(T27GEOM(&g,62U,160U,T27_CODED12)==T27_BAD);
    assert(!T27GEOM(&g,62U,160U,T27_BINARY14));
    assert(!T27ADDR(&g,9919U,b)&&b[0]==0x26U&&b[1]==0xbfU);
    assert(!T27DECD(&g,b,&n)&&n==9919U);
    b[0]=0x40U;b[1]=0xc1U;assert(!T27DECD(&g,b,&n)&&n==1U);
    b[0]=0x80U;b[1]=0U;n=123U;
    assert(T27DECD(&g,b,&n)==T27_BAD&&n==123U);
    assert(!T27GEOM(&g,128U,128U,T27_BINARY14));
    assert(!T27ADDR(&g,16383U,b)&&b[0]==0x3fU&&b[1]==0xffU);
    assert(!T27GEOM(&g,256U,256U,T27_BINARY16));
    assert(!T27ADDR(&g,65535U,b)&&b[0]==0xffU&&b[1]==0xffU);
    assert(!T27DECD(&g,b,&n)&&n==65535U);
    assert(T27ADDR(&g,65536U,b)==T27_BAD);
}
static void fields(void)
{
    static const unsigned char sf[]={0,5,1,0xff,2,0,0,0x0f,0x0f,0,0x11,0xff};
    static const unsigned char broken[]={0,4,0x0f};
    T27FIELD f,old;unsigned int at=0U,n=0U;unsigned char out[32];
    assert(!T27WALK(sf,sizeof sf,&at,&f)&&at==5U&&f.id==1U&&f.header==3U);
    assert(!T27WALK(sf,sizeof sf,&at,&f)&&at==sizeof sf&&f.id==0x0f0fU&&f.length==7U);
    assert(T27WALK(sf,sizeof sf,&at,&f)==T27_END);
    old=f;at=0U;assert(T27WALK(broken,sizeof broken,&at,&f)==T27_BAD&&at==0U&&!memcmp(&f,&old,sizeof f));
    memset(out,0xa5,sizeof out);
    assert(T27PACK(0x0f0fU,sf,5U,out,8U,&n)==T27_BAD&&out[0]==0xa5U&&n==0U);
    assert(!T27PACK(0x0f0fU,sf,5U,out,sizeof out,&n)&&n==9U);
    at=0U;assert(!T27WALK(out,n,&at,&f)&&f.id==0x0f0fU&&f.length==9U);
    assert(T27PACK(0x81U,sf,5U,out,sizeof out,&n)==T27_BAD);
    assert(!T27QUERY(out,sizeof out,&n)&&n==5U&&!memcmp(out,sf,5U));
}
static void writes(void)
{
    static const unsigned char orders[]={
      0x11,0x40,0x40,0x29,2,0xc0,0xf0,0x42,0xf2,0xc1,
      0x28,0x41,0xf4,0x08,0x5a,0x2c,1,0xc0,0xf8,0x05,0x13,
      0x3c,0x40,0xc9,0x08,0x5b,0x12,0x40,0xc1,
      0,0x0c,0x0d,0x15,0x19,0x1c,0x1e,0x3f,0xff,0x0e,0x42,0x43,0x0f};
    static const unsigned char commands[8]={0xf1,0xf5,0x7e,0x6f,0xf3,0xf2,0xf6,0x6e};
    static const unsigned int channels[8]={1,5,13,15,17,2,6,14};
    unsigned int i,c,r;T27GEOMETRY g;TTCCAP cap;unsigned char raw[128];
    assert(!T27GEOM(&g,24U,80U,T27_CODED12));
    assert(!T27WRITE(&g,orders,sizeof orders));
    assert(T27WRITE(&g,orders,3U)==T27_OK);
    assert(T27WRITE(&g,orders,4U)==T27_BAD);
    assert(T27WRITE(&g,orders+10U,1U)==T27_BAD);
    for(i=0U;i<8U;++i){assert(!T27CMAP(commands[i],&c,&r)&&c==channels[i]&&r==(i>4U));}
    c=r=99U;assert(T27CMAP(0U,&c,&r)==T27_UNSUPPORTED&&c==99U&&r==99U);
    assert(!TTCCONFIG(&cap,1U,9U,2U,0U));
    raw[0]=0x27U;raw[1]=0xf1U;raw[2]=0xc3U;memcpy(raw+3U,orders,sizeof orders);
    assert(!TTCRAW(&cap,raw,3U+sizeof orders,&c)&&c==1U);
    raw[1]=0xf3U;raw[2]=0U;raw[3]=5U;raw[4]=1U;raw[5]=0xffU;raw[6]=2U;
    assert(!TTCRAW(&cap,raw,7U,&c)&&c==17U);
    raw[3]=6U;c=99U;assert(TTCRAW(&cap,raw,7U,&c)==TTC_BAD&&c==99U);
    raw[1]=0x6fU;assert(!TTCRAW(&cap,raw,2U,&c)&&c==15U);
    raw[1]=0xf6U;assert(TTCRAW(&cap,raw,2U,&c)==TTC_UNSUPPORTED);
}
static void queries(void)
{
    unsigned char reply[80];T27CAPABILITIES c,old;unsigned int n=1U;
    static const unsigned char area[]={0,21,0x81,0x81,1,0,0,160,0,62,0,0,0,0,1,0,0,0,1,9,12};
    static const unsigned char sizes[]={0,17,0x81,0xa6,0,0,11,1,0,0,80,0,24,0,160,0,62};
    static const unsigned char opaque[]={0,7,0x81,0xb4,0,0,0xff};
    static const unsigned char printer[]={0,17,0x81,0xa6,0,0,11,3,0,7,128,0,0,14,0,0,0};
    reply[0]=0x88U;memcpy(reply+n,sizes,sizeof sizes);n+=sizeof sizes;
    memcpy(reply+n,opaque,sizeof opaque);n+=sizeof opaque;
    memcpy(reply+n,area,sizeof area);n+=sizeof area;
    assert(!T27QRPLY(reply,n,&c)&&c.rows==62U&&c.columns==160U&&c.usable_valid);
    assert(c.implicit_valid&&c.default_rows==24U&&c.default_columns==80U&&c.alternate_rows==62U&&c.alternate_columns==160U);
    assert(T27HAS(&c,0xb4U)&&T27HAS(&c,0x81U)&&!T27HAS(&c,0x86U));
    old=c;reply[n-13U]=0U;reply[n-12U]=0U;
    assert(T27QRPLY(reply,n,&c)==T27_BAD&&!memcmp(&c,&old,sizeof c));
    reply[0]=0x88U;memcpy(reply+1U,printer,sizeof printer);
    assert(!T27QRPLY(reply,1U+sizeof printer,&c)&&c.printer_valid&&!c.implicit_valid&&c.default_buffer==1920U&&c.alternate_buffer==3584U);
    reply[2]=3U;old=c;assert(T27QRPLY(reply,1U+sizeof printer,&c)==T27_BAD&&!memcmp(&c,&old,sizeof c));
}
static void inputs(void)
{
    unsigned char record[300];T27GEOMETRY g;T27INPUTEVENT e,old;
    T27INPUTFIELD fields[70],saved[70];unsigned int i,n=3U;
    static const unsigned char short_reads[4]={0x6b,0x6c,0x6d,0x6e};
    assert(!T27GEOM(&g,24U,80U,T27_CODED12));
    record[0]=0xf7U;record[1]=0x40U;record[2]=0xc2U;
    for(i=0U;i<70U;++i){record[n++]=0x11U;assert(!T27ADDR(&g,10U+i,record+n));n+=2U;record[n++]=0xc1U;}
    assert(!T27INPUT(&g,record,n,&e,fields,70U)&&e.kind==T27_EVENT_KEY&&e.aid==0xf7U&&e.cursor_valid&&e.cursor==2U&&e.fields==70U);
    assert(fields[69U].address==79U&&fields[69U].length==1U);
    old=e;memcpy(saved,fields,sizeof fields);record[n++]=0x11U;
    assert(T27INPUT(&g,record,n,&e,fields,70U)==T27_BAD&&!memcmp(&e,&old,sizeof e)&&!memcmp(fields,saved,sizeof fields));
    for(i=0U;i<4U;++i){record[0]=short_reads[i];assert(!T27INPUT(&g,record,1U,&e,0,0U)&&!e.cursor_valid&&!e.fields&&e.aid==short_reads[i]);}
    record[0]=0x7dU;record[1]=record[2]=0x40U;
    assert(!T27INPUT(&g,record,3U,&e,0,0U)&&e.cursor_valid&&!e.fields);
    record[3]=0xc1U;assert(!T27INPUT(&g,record,4U,&e,fields,70U)&&e.kind==T27_EVENT_UNFORMATTED&&fields[0].offset==3U);
    record[0]=0x88U;record[1]=0;record[2]=4;record[3]=0x81;record[4]=0xff;
    assert(!T27INPUT(&g,record,5U,&e,0,0U)&&e.kind==T27_EVENT_STRUCTURED&&!e.cursor_valid);
}
static void retained(void)
{
    TTCCAP cap,saved;TTCFIELD f;unsigned char old[1920],out[64],text[12];
    unsigned int n,i;
    memset(old,0x40,sizeof old);memset(text,0x40,sizeof text);
    assert(!TTCCONFIG(&cap,1U,9U,2U,0U));
    f.row=1U;f.column=0U;f.attributes=TTC_PROTECTED;f.text=text;f.length=sizeof text;
    assert(!TTCDIFF(&cap,&f,1U,old,out,sizeof out,&n)&&n==0U);
    text[2U]=0xc1U;text[5U]=0xc2U;
    assert(!TTCDIFF(&cap,&f,1U,old,out,sizeof out,&n)&&n==8U&&out[0]==0x40U&&out[1]==0x11U);
    assert(out[4]==0xc1U&&out[5]==0x40U&&out[6]==0x40U&&out[7]==0xc2U);
    for(i=0U;i<n;++i)assert(out[i]!=0x13U&&out[i]!=0x1dU);
    f.attributes=TTC_INPUT;out[0]=0xa5U;
    assert(TTCDIFF(&cap,&f,1U,old,out,sizeof out,&n)==TTC_BAD&&out[0]==0xa5U);
    assert(TTCDIFF(&cap,&f,65U,old,out,sizeof out,&n)==TTC_BAD&&out[0]==0xa5U);
    assert(!TTCGEOM(&cap,24U,80U,62U,160U)&&cap.generation==2U&&cap.encoding==TSA_ADDRESS_BINARY14);
    assert(!TTCADDR(&cap,9919U,out)&&out[0]==0x26U&&out[1]==0xbfU);
    saved=cap;assert(TTCGEOM(&cap,24U,80U,0xffffffffU,160U)==TTC_BAD&&!memcmp(&cap,&saved,sizeof cap));
}
int main(void)
{
    addresses();fields();writes();queries();inputs();retained();
    puts("3270 family: address modes, all basic commands/orders, structured framing, capabilities, printer sizes and atomic input pass");
    return 0;
}
