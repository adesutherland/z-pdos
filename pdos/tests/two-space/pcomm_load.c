/* SPDX-License-Identifier: MIT
 * Compare successor materialisation of source-built PCOMM against the
 * independent Classic Linker flat output at two U placements.
 */
#include "twospace_tso.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned int read_all(const char *name,unsigned char *buffer)
{
    FILE *file=fopen(name,"rb"); size_t n;
    assert(file); n=fread(buffer,1,TST_MAX_IMAGE+1U,file);
    assert(n && n<=TST_MAX_IMAGE && !ferror(file) && feof(file));
    assert(fclose(file)==0); return (unsigned int)n;
}
int main(int argc,char **argv)
{
    unsigned char *stage,*actual,*expected;
    unsigned int bytes,n,base,i; TSTINFO info;
    assert(argc==4);
    stage=(unsigned char *)malloc(TST_MAX_IMAGE+1U);
    actual=(unsigned char *)malloc(TST_MAX_IMAGE+1U);
    expected=(unsigned char *)malloc(TST_MAX_IMAGE+1U);
    assert(stage && actual && expected);
    bytes=read_all(argv[1],stage);
    assert(TSTSTAGEVALIDATE(stage,bytes,31U,&info)==TST_OK);
    for(i=0;i<2U;++i) {
        base=i ? 0x0c000000U : 0x0b000000U;
        n=read_all(argv[i+2U],expected);
        assert(TSTIMAGE31(stage+64U,info.raw_bytes,base,actual,
                          TST_MAX_IMAGE,&info)==TST_OK);
        assert(info.image_bytes>=n && info.image_bytes-n<8U);
        assert(memcmp(actual,expected,n)==0);
        while(n<info.image_bytes) assert(actual[n++]==0);
        assert(info.entry_offset<info.image_bytes && info.flags==0x12U);
    }
    free(expected);free(actual);free(stage);
    puts("source-built PCOMM matches independent Classic Linker at both U bases");
    return 0;
}
