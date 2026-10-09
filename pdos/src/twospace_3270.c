/* SPDX-License-Identifier: MIT
 * Independently implemented from IBM GA23-0059-07, chapters 3-6 and
 * Appendix C. Opaque SF payloads are forwarded, never executed in K. */
#include "twospace_3270.h"
static const unsigned char t27coded[64]={
 0x40,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
 0x50,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
 0x60,0x61,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
 0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f};
static unsigned int half(const unsigned char *p)
{return ((unsigned int)p[0]<<8)|p[1];}
static int geometry(const T27GEOMETRY *g)
{
    unsigned int limit;
    if(!g||!g->rows||!g->columns)return 0;
    limit=g->addressing==T27_CODED12?4096U:g->addressing==T27_BINARY14?16384U:
          g->addressing==T27_BINARY16?65536U:0U;
    return limit&&g->columns<=limit&&g->rows<=limit/g->columns&&
           g->cells==g->rows*g->columns;
}
int T27GEOM(T27GEOMETRY *g,unsigned int rows,unsigned int columns,unsigned int mode)
{
    T27GEOMETRY next;
    if(!g)return T27_BAD;
    next.rows=rows;next.columns=columns;next.addressing=mode;
    next.cells=0U;
    if(columns&&rows<=65536U/columns)next.cells=rows*columns;
    if(!geometry(&next))return T27_BAD;
    *g=next;return T27_OK;
}
int T27ADDR(const T27GEOMETRY *g,unsigned int address,unsigned char *out)
{
    if(!geometry(g)||!out||address>=g->cells)return T27_BAD;
    if(g->addressing==T27_CODED12){out[0]=t27coded[address>>6];out[1]=t27coded[address&63U];}
    else{out[0]=(unsigned char)(address>>8);out[1]=(unsigned char)address;}
    return T27_OK;
}
int T27DECD(const T27GEOMETRY *g,const unsigned char *in,unsigned int *out)
{
    unsigned int address,a=64U,b=64U,i;
    if(!geometry(g)||!in||!out)return T27_BAD;
    if(g->addressing==T27_BINARY16)address=half(in);
    else if(!(in[0]&0xc0U))address=half(in);
    else{
        if((in[0]&0xc0U)==0x80U)return T27_BAD;
        for(i=0U;i<64U;++i){if(t27coded[i]==in[0])a=i;if(t27coded[i]==in[1])b=i;}
        if(a==64U||b==64U)return T27_BAD;
        address=a*64U+b;
    }
    if(address>=g->cells)return T27_BAD;
    *out=address;return T27_OK;
}
int T27CMAP(unsigned int command,unsigned int *channel,unsigned int *input)
{
    unsigned int c,r=0U;
    if(!channel||!input)return T27_BAD;
    switch(command){
    case 0xf1U:c=1U;break;
    case 0xf5U:c=5U;break;
    case 0x7eU:c=13U;break;
    case 0x6fU:c=15U;break;
    case 0xf3U:c=17U;break;
    case 0xf2U:c=2U;r=1U;break;
    case 0xf6U:c=6U;r=1U;break;
    case 0x6eU:c=14U;r=1U;break;
    default:return T27_UNSUPPORTED;
    }
    *channel=c;*input=r;return T27_OK;
}
int T27WALK(const unsigned char *in,unsigned int bytes,unsigned int *offset,T27FIELD *out)
{
    unsigned int at,n,h,id;
    T27FIELD f;
    if(!in||!offset||!out||*offset>bytes)return T27_BAD;
    at=*offset;if(at==bytes)return T27_END;
    if(bytes-at<3U)return T27_BAD;
    n=half(in+at);if(!n)n=bytes-at;
    h=(in[at+2U]==0x0fU||in[at+2U]==0x10U||in[at+2U]==0x81U)?4U:3U;
    if(n<h||n>bytes-at)return T27_BAD;
    id=in[at+2U];if(h==4U)id=(id<<8)|in[at+3U];
    f.id=id;f.offset=at;f.length=n;f.header=h;f.data=in+at;
    *out=f;*offset=at+n;return T27_OK;
}
int T27PACK(unsigned int id,const unsigned char *data,unsigned int bytes,
              unsigned char *out,unsigned int capacity,unsigned int *length)
{
    unsigned int h,n,i;
    if(!out||!length||(bytes&&!data)||id>65535U)return T27_BAD;
    h=id>255U?4U:3U;
    if((h==4U&&id>>8!=0x0fU&&id>>8!=0x10U&&id>>8!=0x81U)||
       (h==3U&&(id==0x0fU||id==0x10U||id==0x81U))||bytes>65535U-h)return T27_BAD;
    n=h+bytes;if(n>capacity)return T27_BAD;
    out[0]=(unsigned char)(n>>8);out[1]=(unsigned char)n;
    out[2]=(unsigned char)(h==4U?id>>8:id);if(h==4U)out[3]=(unsigned char)id;
    for(i=0U;i<bytes;++i)out[h+i]=data[i];
    *length=n;return T27_OK;
}
static int character(unsigned int c)
{
    return c>=0x40U||c==0U||c==0x0cU||c==0x0dU||c==0x0eU||c==0x0fU||
           c==0x15U||c==0x19U||c==0x1cU||c==0x1eU||c==0x3fU;
}
int T27WRITE(const T27GEOMETRY *g,const unsigned char *in,unsigned int bytes)
{
    unsigned int at=0U,c,n,address;
    if(!geometry(g)||(bytes&&!in))return T27_BAD;
    while(at<bytes){
        c=in[at++];
        if(c==0x11U||c==0x12U||c==0x3cU){
            if(bytes-at<2U||T27DECD(g,in+at,&address))return T27_BAD;
            at+=2U;
            if(c==0x3cU){
                if(at==bytes)return T27_BAD;c=in[at++];
                if(c==0x08U){if(at==bytes||in[at]<0x40U||in[at]==0xffU)return T27_BAD;++at;}
                else if(!character(c))return T27_BAD;
            }
        }else if(c==0x1dU){if(at==bytes)return T27_BAD;++at;}
        else if(c==0x29U||c==0x2cU){
            if(at==bytes)return T27_BAD;n=in[at++];
            if(n>(bytes-at)/2U)return T27_BAD;at+=2U*n;
        }else if(c==0x28U){if(bytes-at<2U)return T27_BAD;at+=2U;}
        else if(c==0x08U){
            if(at==bytes||in[at]<0x40U||in[at]==0xffU)return T27_BAD;++at;
        }else if(c!=0x05U&&c!=0x13U&&!character(c))return T27_UNSUPPORTED;
    }
    return T27_OK;
}
int T27QUERY(unsigned char *out,unsigned int capacity,unsigned int *length)
{
    static const unsigned char query[5]={0U,5U,1U,0xffU,2U};unsigned int i;
    if(!out||!length||capacity<5U)return T27_BAD;
    for(i=0U;i<5U;++i)out[i]=query[i];*length=5U;return T27_OK;
}
int T27PRWRT(const unsigned char *in,unsigned int bytes)
{
    unsigned int i;
    if(!in||!bytes)return T27_BAD;
    for(i=1U;i<bytes;++i)if(in[i]<0x40U&&in[i]!=0U&&in[i]!=0x0cU&&in[i]!=0x0dU&&in[i]!=0x15U&&in[i]!=0x19U)return T27_UNSUPPORTED;
    return T27_OK;
}
int T27PRINT(const unsigned char *text,unsigned int bytes,unsigned char *out,unsigned int capacity,unsigned int *length)
{
    unsigned int i;
    if(!text||!bytes||!out||!length||bytes>65533U||capacity<bytes+2U)return T27_BAD;
    for(i=0U;i<bytes;++i)if(text[i]<0x40U&&text[i]!=0x0cU&&text[i]!=0x0dU&&text[i]!=0x15U)return T27_UNSUPPORTED;
    out[0]=0xc8U;for(i=0U;i<bytes;++i)out[1U+i]=text[i];out[bytes+1U]=0x19U;*length=bytes+2U;return T27_OK;
}
int T27REPLY(unsigned int mode,const unsigned char *attrs,unsigned int bytes,unsigned char *out,unsigned int capacity,unsigned int *length)
{
    unsigned int i;
    if(mode>2U||bytes>16U||(bytes&&!attrs)||!out||!length||capacity<5U+bytes||(mode!=2U&&bytes))return T27_BAD;
    for(i=0U;i<bytes;++i)if(attrs[i]!=0x41U&&attrs[i]!=0x42U&&attrs[i]!=0x43U)return T27_UNSUPPORTED;
    out[0]=0U;out[1]=(unsigned char)(5U+bytes);out[2]=9U;out[3]=0U;out[4]=(unsigned char)mode;
    for(i=0U;i<bytes;++i)out[5U+i]=attrs[i];*length=5U+bytes;return T27_OK;
}
int T27HAS(const T27CAPABILITIES *cap,unsigned int code)
{return cap&&code<256U&&(cap->replies[code/8U]&(1U<<(code%8U)))!=0U;}
int T27QRPLY(const unsigned char *in,unsigned int bytes,T27CAPABILITIES *out)
{
    T27CAPABILITIES cap;
    T27FIELD f;
    T27GEOMETRY g;
    unsigned int at=1U,i,code,n,mode;
    if(!in||!out||bytes<4U||in[0]!=0x88U)return T27_BAD;
    for(i=0U;i<32U;++i)cap.replies[i]=0U;
    cap.usable_valid=cap.rows=cap.columns=cap.addressing=cap.hardcopy=cap.page_printer=0U;
    cap.implicit_valid=cap.default_rows=cap.default_columns=cap.alternate_rows=cap.alternate_columns=0U;
    cap.printer_valid=cap.default_buffer=cap.alternate_buffer=0U;
    cap.colours_valid=cap.highlighting=0U;
    for(i=0U;i<16U;++i)cap.colours[i]=0U;
    while(at<bytes){
        if(T27WALK(in,bytes,&at,&f)!=T27_OK||f.id<0x8100U||f.id>0x81ffU)return T27_BAD;
        code=f.id&255U;
        cap.replies[code/8U]|=(unsigned char)(1U<<(code%8U));
        if(code==0x81U){
            if(f.length<21U)return T27_BAD;
            cap.hardcopy=(f.data[4U]&0x10U)!=0U;cap.page_printer=(f.data[4U]&0x40U)!=0U;
            mode=f.data[4U]&15U;
            if(mode!=1U&&mode!=3U&&mode!=15U)return T27_BAD;
            cap.addressing=mode==3U?T27_BINARY16:mode==1U?T27_BINARY14:0U;
            /* Pels and unmapped printer pages are retained as raw replies;
             * do not reinterpret them as a character grid. */
            if(!(f.data[5U]&0x20U)&&mode!=15U){
                n=mode==3U?T27_BINARY16:T27_BINARY14;
                if(T27GEOM(&g,half(f.data+8U),half(f.data+6U),n))return T27_BAD;
                if(cap.usable_valid&&(cap.rows!=g.rows||cap.columns!=g.columns))return T27_BAD;
                cap.rows=g.rows;cap.columns=g.columns;cap.usable_valid=1U;
            }
        }else if(code==0x86U){
            if(f.length<6U||f.data[5U]>(f.length-6U)/2U)return T27_BAD;
            for(i=0U;i<f.data[5U];++i){
                unsigned int value=f.data[6U+2U*i],colour=f.data[7U+2U*i];
                if(colour>=0xf0U){
                    n=colour-0xf0U;
                    if(!(cap.colours_valid&(1U<<n))||value==colour)cap.colours[n]=(unsigned char)value;
                    cap.colours_valid|=1U<<n;
                }
            }
        }else if(code==0x87U){
            if(f.length<5U||f.data[4U]>(f.length-5U)/2U)return T27_BAD;
            for(i=0U;i<f.data[4U];++i){
                unsigned int value=f.data[5U+2U*i],action=f.data[6U+2U*i];
                if(value==action&&(value==0xf1U||value==0xf2U||value==0xf4U))cap.highlighting|=value&15U;
            }
        }else if(code==0xa6U){
            if(f.length<6U)return T27_BAD;
            i=6U;
            while(i<f.length){
                n=f.data[i];if(n<2U||n>f.length-i)return T27_BAD;
                if(f.data[i+1U]==1U){
                    if(n!=11U||f.data[i+2U])return T27_BAD;
                    if(!half(f.data+i+3U)||!half(f.data+i+5U)||!half(f.data+i+7U)||!half(f.data+i+9U))return T27_BAD;
                    cap.default_columns=half(f.data+i+3U);cap.default_rows=half(f.data+i+5U);
                    cap.alternate_columns=half(f.data+i+7U);cap.alternate_rows=half(f.data+i+9U);cap.implicit_valid=1U;
                }else if(f.data[i+1U]==3U){
                    if(n!=11U||f.data[i+2U])return T27_BAD;
                    cap.default_buffer=half(f.data+i+3U);cap.alternate_buffer=half(f.data+i+7U);
                    if(!cap.default_buffer||!cap.alternate_buffer)return T27_BAD;
                    cap.printer_valid=1U;
                }
                i+=n;
            }
        }
    }
    if(cap.implicit_valid){
        /* Advertised 16-bit support belongs to explicitly created partitions.
         * The implicit partition always retains 12/14-bit addressing. */
        mode=T27_BINARY14;
        if(T27GEOM(&g,cap.default_rows,cap.default_columns,mode)||
           T27GEOM(&g,cap.alternate_rows,cap.alternate_columns,mode))return T27_BAD;
    }
    *out=cap;return T27_OK;
}
int T27DXQR(const unsigned char *in,unsigned int bytes,T27CAPABILITIES *out)
{
    /* Observed DX3270 1.7.5 profile; these replies omit the SF81 prefix and
     * use a nonstandard 18-byte usable-area body. Match the whole signature
     * before projecting only its validated character grid/palette. */
    static const unsigned char signature[68]={
        0x88U,0U,8U,0x81U,0x80U,0x81U,0x84U,0x86U,0x87U,
        0U,18U,0x80U,1U,0U,0U,0x84U,0U,0x1bU,1U,0U,0x60U,0U,0x70U,9U,12U,0x0dU,0xecU,
        0U,21U,0x86U,0U,8U,0U,0xf4U,0xf1U,0xf1U,0xf2U,0xf2U,0xf3U,0xf3U,0xf4U,0xf4U,0xf5U,0xf5U,0xf6U,0xf6U,0xf7U,0xf7U,
        0U,14U,0x87U,5U,0U,0U,0xf1U,0xf1U,0xf2U,0xf2U,0xf4U,0xf4U,0xf8U,0xf8U,
        0U,6U,0x84U,0U,1U,2U};
    unsigned char standard[67];unsigned int i;T27GEOMETRY g;
    if(!in||!out||bytes!=sizeof signature)return T27_BAD;
    for(i=0U;i<bytes;++i){
        if(i==12U||(i>=14U&&i<=17U)||i==25U||i==26U)continue;
        if(in[i]!=signature[i])return T27_BAD;
    }
    if(T27GEOM(&g,half(in+16U),half(in+14U),T27_BINARY14)||
       half(in+25U)!=g.cells||in[12U]!=(g.cells>4095U?0U:1U))return T27_BAD;
    for(i=0U;i<sizeof standard;++i)standard[i]=0U;
    for(i=0U;i<9U;++i)standard[i]=in[i];
    standard[10U]=21U;standard[11U]=standard[12U]=0x81U;standard[13U]=1U;
    standard[15U]=in[14U];standard[16U]=in[15U];standard[17U]=in[16U];standard[18U]=in[17U];
    standard[31U]=22U;standard[32U]=0x81U;
    for(i=0U;i<19U;++i)standard[33U+i]=in[29U+i];
    standard[53U]=15U;standard[54U]=0x81U;
    for(i=0U;i<12U;++i)standard[55U+i]=in[50U+i];
    return T27QRPLY(standard,sizeof standard,out);
}
static int short_key(unsigned int aid)
{return aid==0x6bU||aid==0x6cU||aid==0x6dU||aid==0x6eU;}
static int input_pass(const T27GEOMETRY *g,const unsigned char *in,unsigned int bytes,
                        T27INPUTEVENT *event,T27INPUTFIELD *fields,unsigned int capacity)
{
    T27INPUTEVENT e;
    T27FIELD sf;
    unsigned int at=0U,address,count=0U,start=0U;
    if(!geometry(g)||!in||!bytes||!event)return T27_BAD;
    e.aid=in[0];e.cursor=e.cursor_valid=e.fields=0U;e.kind=T27_EVENT_KEY;
    if(e.aid==0x88U){
        at=1U;while(at<bytes)if(T27WALK(in,bytes,&at,&sf)!=T27_OK)return T27_BAD;
        if(bytes==1U)return T27_BAD;e.kind=T27_EVENT_STRUCTURED;*event=e;return T27_OK;
    }
    if(bytes==1U&&short_key(e.aid)){*event=e;return T27_OK;}
    if(bytes<3U||T27DECD(g,in+1U,&e.cursor))return T27_BAD;
    e.cursor_valid=1U;at=3U;
    if(at==bytes){*event=e;return T27_OK;}
    if(short_key(e.aid))return T27_BAD;
    if(in[at]!=0x11U){
        e.kind=T27_EVENT_UNFORMATTED;count=1U;
        if(count>capacity)return T27_BAD;
        if(fields){fields[0].address=0U;fields[0].offset=3U;fields[0].length=bytes-3U;}
    }else while(at<bytes){
        if(bytes-at<3U||in[at++]!=0x11U||T27DECD(g,in+at,&address))return T27_BAD;
        at+=2U;start=at;
        while(at<bytes&&in[at]!=0x11U){
            if(in[at]==0x28U){if(bytes-at<3U)return T27_BAD;at+=3U;}
            else if(in[at]==0x08U){if(bytes-at<2U||in[at+1U]<0x40U||in[at+1U]==0xffU)return T27_BAD;at+=2U;}
            else{if(!character(in[at]))return T27_BAD;++at;}
        }
        if(count==capacity)return T27_BAD;
        if(fields){
            fields[count].address=address;fields[count].offset=start;fields[count].length=at-start;
        }
        ++count;
    }
    e.fields=count;*event=e;return T27_OK;
}
int T27INPUT(const T27GEOMETRY *g,const unsigned char *in,unsigned int bytes,
               T27INPUTEVENT *event,T27INPUTFIELD *fields,unsigned int capacity)
{
    T27INPUTEVENT e;
    int rc;
    if(!event||(capacity&&!fields))return T27_BAD;
    rc=input_pass(g,in,bytes,&e,0,capacity);if(rc)return rc;
    /* The record is immutable and K/caller owned. A complete first pass
     * validates every span, so the publication pass cannot fail halfway. */
    if(e.fields){
        rc=input_pass(g,in,bytes,&e,fields,capacity);if(rc)return rc;
    }
    *event=e;return T27_OK;
}
int T27EVENT(const T27GEOMETRY *g,const unsigned char *in,unsigned int bytes,T27INPUTEVENT *event)
{return input_pass(g,in,bytes,event,0,65536U);}
static int buffer_pass(const T27GEOMETRY *g,const unsigned char *in,unsigned int bytes,unsigned int field,
                         unsigned char *out,unsigned int capacity,unsigned int *length,T27INPUTEVENT *event)
{
    unsigned int at=3U,address=0U,active=0U,found=0U,n=0U,last=0U,code,pairs,i,attr;
    T27INPUTEVENT e;
    if(!geometry(g)||!in||bytes<3U||!field||field>=g->cells||!length||!event||T27DECD(g,in+1U,&e.cursor))return T27_BAD;
    e.aid=in[0U];e.kind=T27_EVENT_BUFFER;e.cursor_valid=1U;e.fields=0U;
    while(at<bytes){
        code=in[at++];
        if(code==0x28U){if(bytes-at<2U)return T27_BAD;at+=2U;continue;}
        if(address==g->cells)return T27_BAD;
        if(code==0x1dU||code==0x29U){
            attr=0xffffffffU;
            if(code==0x1dU){if(at==bytes)return T27_BAD;attr=in[at++]&0x3fU;}
            else{if(at==bytes)return T27_BAD;pairs=in[at++];if(pairs>(bytes-at)/2U)return T27_BAD;
                for(i=0U;i<pairs;++i){if(in[at]==0xc0U)attr=in[at+1U]&0x3fU;at+=2U;}
                if(attr==0xffffffffU)return T27_BAD;
            }
            active=address==field-1U;
            if(active){if(attr&0x20U||found)return T27_BAD;found=1U;}
            if(!(attr&0x20U))++e.fields;
            ++address;continue;
        }
        if(code==0x08U){if(at==bytes)return T27_BAD;code=in[at++];if(active)return T27_UNSUPPORTED;}
        if(code&&(!character(code)||code==0x0eU||code==0x0fU))return T27_UNSUPPORTED;
        if(active){if(n>=capacity&&code)return T27_BAD;if(out&&n<capacity)out[n]=(unsigned char)code;++n;if(code)last=n;}
        ++address;
    }
    if(address!=g->cells||!found)return T27_BAD;
    *length=last;*event=e;return T27_OK;
}
int T27BUFFER(const T27GEOMETRY *g,const unsigned char *in,unsigned int bytes,unsigned int field,
                unsigned char *out,unsigned int capacity,unsigned int *length,T27INPUTEVENT *event)
{
    T27INPUTEVENT checked;unsigned int n;int rc;
    if(!out||!length||!event)return T27_BAD;
    rc=buffer_pass(g,in,bytes,field,0,capacity,&n,&checked);if(rc)return rc;
    rc=buffer_pass(g,in,bytes,field,out,capacity,&n,&checked);if(rc)return rc;
    *length=n;*event=checked;return T27_OK;
}
