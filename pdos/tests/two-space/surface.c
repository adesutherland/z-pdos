/* SPDX-License-Identifier: MIT; independent terminal field/cursor model. */
#include "twospace_surface.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static unsigned char device[16384],field[16384],foreground[16384];
static unsigned int cursor,inserts;
static void apply(const T27GEOMETRY *g,const unsigned char *stream,unsigned int bytes,unsigned int full)
{
    unsigned int i=1U,at=0U,code,n,j,type,value,colour=0U;
    if(full){memset(device,0,sizeof device);memset(field,0,sizeof field);memset(foreground,0,sizeof foreground);}
    while(i<bytes){code=stream[i++];
        if(code==0x11U){assert(bytes-i>=2U&&!T27DECD(g,stream+i,&at));i+=2U;}
        else if(code==0x1dU){assert(i<bytes);field[at]=stream[i++]&0x3fU;device[at]=0U;at=(at+1U)%g->cells;}
        else if(code==0x29U){assert(i<bytes);n=stream[i++];assert(n<=(bytes-i)/2U);field[at]=0U;
            for(j=0U;j<n;++j){type=stream[i++];value=stream[i++];if(type==0xc0U)field[at]=(unsigned char)(value&0x3fU);if(type==0x42U)foreground[at]=(unsigned char)value;}
            device[at]=0U;at=(at+1U)%g->cells;
        }else if(code==0x28U){assert(bytes-i>=2U);type=stream[i++];value=stream[i++];if(type==0x42U)colour=value;}
        else if(code==0x13U){cursor=at;++inserts;}
        else{assert(code>=0x40U&&code!=0xffU);device[at]=(unsigned char)code;field[at]=0U;foreground[at]=(unsigned char)colour;at=(at+1U)%g->cells;}
    }
}
int main(void)
{
    unsigned char *memory=(unsigned char *)malloc(6U*TWF_MAX_CELLS),stream[65535],old_stream[16];
    TWFSURFACE s,old;T27GEOMETRY g;TWFPALETTE palette;unsigned int n,i;
    static const unsigned char title[]={0xc8U,0xc5U,0xd3U,0xd3U,0xd6U};
    assert(memory&&!T27GEOM(&g,24U,80U,T27_CODED12));
    assert(!TWFINIT(&s,&g,memory,memory+16384U,memory+32768U));
    assert(!TWFINIT(&old,&g,memory+49152U,memory+65536U,memory+81920U));
    assert(!TWFINPUT(&s,17U*80U+4U,256U));assert(s.kind[18U*80U]==TWF_INPUT&&s.kind[19U*80U]==TWF_INPUT);
    assert(s.kind[20U*80U+20U]==TWF_ATTRIBUTE&&s.kind[20U*80U+19U]==TWF_INPUT);
    assert(!TWFTEXT(&s,0U,1U,78U,title,sizeof title,TWF_HEADER));s.role[0U]=TWF_HEADER;
    memset(&palette,0,sizeof palette);palette.valid=0xf4U;palette.highlighting=2U;
    for(i=0U;i<16U;++i)palette.colour[i]=(unsigned char)(0xf0U+i);
    assert(!TWFENCODE(&s,0,1U,&palette,stream,sizeof stream,&n)&&stream[0U]==0xc3U);
    apply(&g,stream,n,1U);assert(cursor==s.input&&inserts==1U&&field[s.input-1U]==0U&&field[s.input+256U]==0x30U);
    assert(!memcmp(device+1U,title,sizeof title)&&foreground[1U]==0xf5U&&device[s.input]==0U);
    assert(!TWFCOMMIT(&old,&s));device[s.input]=0xc1U;device[s.input+1U]=0xc2U;cursor=s.input+2U;
    assert(!TWFTEXT(&s,3U,1U,78U,title,sizeof title,TWF_NORMAL));
    assert(!TWFENCODE(&s,&old,0U,&palette,stream,sizeof stream,&n)&&n&&stream[0U]==0x40U);
    apply(&g,stream,n,0U);assert(device[s.input]==0xc1U&&device[s.input+1U]==0xc2U&&cursor==s.input+2U&&inserts==1U);
    assert(!TWFCOMMIT(&old,&s)&&!TWFENCODE(&s,&old,0U,&palette,stream,sizeof stream,&n)&&n==0U);
    s.text[s.input]=0xc1U;s.text[s.input+1U]=0xc2U;
    assert(!TWFENCODE(&s,0,1U,&palette,stream,sizeof stream,&n));apply(&g,stream,n,1U);
    assert(field[s.input-1U]==1U&&device[s.input]==0xc1U&&device[s.input+1U]==0xc2U);
    s.role[s.input-1U]=TWF_HEADER;memset(stream,0xa5U,sizeof stream);
    assert(TWFENCODE(&s,&old,0U,&palette,stream,sizeof stream,&n)==-1&&stream[0U]==0xa5U);s.role[s.input-1U]=TWF_ACTION;
    memcpy(old_stream,stream,sizeof old_stream);assert(TWFENCODE(&s,0,1U,&palette,stream,8U,&n)==-1&&!memcmp(stream,old_stream,sizeof old_stream));
    assert(TWFRECT(&s,17U,0U,2U,80U,TWF_NORMAL)==-1);
    memset(&palette,0,sizeof palette);assert(!TWFENCODE(&s,0,1U,&palette,stream,sizeof stream,&n));
    apply(&g,stream,n,1U);assert(field[0U]==0x38U);
    assert(!T27GEOM(&g,62U,160U,T27_BINARY14)&&!TWFINIT(&s,&g,memory,memory+16384U,memory+32768U));
    assert(!TWFINPUT(&s,58U*160U+4U,256U)&&!TWFENCODE(&s,0,1U,&palette,stream,sizeof stream,&n));
    apply(&g,stream,n,1U);assert(cursor==s.input);
    free(memory);return 0;
}
