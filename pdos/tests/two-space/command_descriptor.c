/* SPDX-License-Identifier: MIT; verify the actual relocated resident artifact. */
#include "twospace_command_router.h"
#include "twospace_tso.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
static unsigned int word(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
int main(int argc,char **argv)
{
    FILE *f;long length;unsigned char *stage,*image,*entry,*veneer;
    TSTINFO info;unsigned int i,address;const unsigned int base=0x0b000000U;
    assert(argc==2);f=fopen(argv[1],"rb");assert(f);
    assert(!fseek(f,0,SEEK_END));length=ftell(f);
    assert(length>64&&length<=(long)(TST_MAX_RAW+TST_BLOCK));assert(!fseek(f,0,SEEK_SET));
    stage=(unsigned char *)malloc((size_t)length);image=(unsigned char *)malloc(TST_MAX_IMAGE);assert(stage&&image);
    assert(fread(stage,1,(size_t)length,f)==(size_t)length);assert(!fclose(f));
    assert(TSTSTAGEVALIDATE(stage,(unsigned int)length,31U,&info)==TST_OK);
    assert(TSTIMAGE31(stage+64U,info.raw_bytes,base,image,TST_MAX_IMAGE,&info)==TST_OK);
    assert(info.entry_offset<=info.image_bytes&&info.image_bytes-info.entry_offset>=TCM_DESC_BYTES);
    entry=image+info.entry_offset;
    assert(entry[0]==0x0d&&entry[1]==0xc0&&entry[2]==0x47&&entry[3]==0xf0);
    assert(!memcmp(entry+TCM_DESC_SIGNATURE,"TCM1",4));
    for(i=0U;i<TCM_DESC_COUNT;++i){
        address=word(entry+TCM_DESC_ENTRIES+4U*i);
        assert(!(address&1U)&&address>=base&&address-base<info.image_bytes-8U);
        veneer=image+address-base;
        if(!i)assert(veneer[0]==0x0d&&veneer[1]==0xc0);
        else{
            /* LA 0,i; SVC 197; BR 14: native operation and mode-independent bytes. */
            assert(veneer[0]==0x41&&veneer[1]==0&&veneer[2]==0&&veneer[3]==i);
            assert(veneer[4]==0x0a&&veneer[5]==197&&veneer[6]==0x07&&veneer[7]==0xfe);
        }
    }
    free(image);free(stage);puts("COMMAND DESCRIPTOR PASS");return 0;
}
