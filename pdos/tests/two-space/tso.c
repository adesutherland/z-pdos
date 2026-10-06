/* SPDX-License-Identifier: MIT
 * Compare selected low-resident AMODE31/64 materializers with pinned native
 * load-module outputs at two placements, then mutate format boundaries.
 */
#include "twospace_tso.h"
#include "pdosutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr,"tso:%d: %s\n",__LINE__,#x); exit(1); } \
} while (0)

static unsigned int read_file(const char *path, unsigned char *bytes)
{
    FILE *file=fopen(path,"rb");
    size_t count;
    CHECK(file!=0);
    count=fread(bytes,1,TST_MAX_RAW+1U,file);
    CHECK(count>0U && count<=TST_MAX_RAW && !ferror(file) && feof(file));
    CHECK(fclose(file)==0);
    return (unsigned int)count;
}

static unsigned int hash_image(const unsigned char *image,
                               unsigned int length)
{
    unsigned int i, hash=0x811c9dc5U;
    for (i=0U; i<length; ++i)
        hash=(hash^image[i])*0x01000193U;
    return hash;
}

int int_rdblock(int a, int b, int c, int d, void *e, int f, int g)
{
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g;
    abort(); return -1;
}

static void compare_released_loader(const unsigned char *raw,
                                    unsigned int raw_bytes,
                                    const unsigned char *image,
                                    unsigned int image_bytes,
                                    unsigned int base,
                                    int mode)
{
    char *legacy=(char *)calloc(TST_MAX_IMAGE,1U);
    int length=(int)raw_bytes, entry, amode, rmode;
    CHECK(legacy!=0);
    memcpy(legacy,raw,raw_bytes);
    CHECK(fixPEMode(legacy,&length,&entry,(int)base,TST_MAX_IMAGE,
                    &amode,&rmode)==0);
    CHECK(amode==mode && rmode==1 && length==(int)image_bytes);
    CHECK(memcmp(image,legacy,image_bytes)==0);
    free(legacy);
}

int main(int argc, char **argv)
{
    unsigned char *raw, *image, *mutated;
    unsigned int bytes, bytes64, stage_bytes;
    TSTINFO info;
    if (argc!=5) return 2;
    raw=(unsigned char *)malloc(TST_MAX_RAW+1U);
    mutated=(unsigned char *)malloc(TST_MAX_RAW+1U);
    image=(unsigned char *)malloc(TST_MAX_IMAGE);
    CHECK(raw && mutated && image);
    bytes=read_file(argv[1],raw);
    CHECK(bytes==1395270U);
    CHECK(TSTHEADER(raw,bytes,31U,&info)==TST_OK);
    CHECK(info.records==1429U && info.max_record==6160U &&
          info.flags==0x12U && info.rmode_any==1U &&
          info.entry_offset==0U && info.input_fnv==0x5db419aeU);
    CHECK(TSTIMAGE31(raw,bytes,0x05000000U,image,TST_MAX_IMAGE,&info)==TST_OK);
    CHECK(info.image_bytes==1094960U &&
          hash_image(image,info.image_bytes)==0xf4b21310U);
    compare_released_loader(raw,bytes,image,info.image_bytes,0x05000000U,2);
    CHECK(TSTIMAGE31(raw,bytes,0x07000000U,image,TST_MAX_IMAGE,&info)==TST_OK);
    CHECK(info.image_bytes==1094960U &&
          hash_image(image,info.image_bytes)==0x3ce51566U);
    compare_released_loader(raw,bytes,image,info.image_bytes,0x07000000U,2);
    CHECK(TSTIMAGE31(raw,bytes,0x20000U,image,TST_MAX_IMAGE,&info)==TST_BAD);
    CHECK(TSTIMAGE64ANY(raw,bytes,0x09000000U,image,
                        TST_MAX_IMAGE,&info)==TST_BAD);
    CHECK(TSTIMAGE31(raw,bytes,0x07000000U,image,1000000U,&info)==TST_BAD);
    CHECK(TSTHEADER(raw,bytes,64U,&info)==TST_BAD);
    CHECK(TSTHEADER(raw,bytes-1U,31U,&info)==TST_BAD);
    memcpy(mutated,raw,bytes);
    mutated[0U]=0U; mutated[1U]=1U;
    CHECK(TSTHEADER(mutated,bytes,31U,&info)==TST_BAD);
    memcpy(mutated,raw,bytes);
    mutated[5U]=0U;
    CHECK(TSTHEADER(mutated,bytes,31U,&info)==TST_BAD);
    memcpy(mutated,raw,bytes);
    mutated[336U+57U]=0x11U;
    CHECK(TSTHEADER(mutated,bytes,31U,&info)==TST_BAD);
    memcpy(mutated,raw,bytes);
    mutated[336U+53U]=0xffU;
    mutated[336U+54U]=0xffU;
    mutated[336U+55U]=0xffU;
    CHECK(TSTIMAGE31(mutated,bytes,0x05000000U,image,
                     TST_MAX_IMAGE,&info)==TST_BAD);
    memcpy(mutated,raw,bytes);
    CHECK(mutated[644U]==0x20U);
    mutated[644U]=0xffU;
    CHECK(TSTIMAGE31(mutated,bytes,0x05000000U,image,
                     TST_MAX_IMAGE,&info)==TST_BAD);
    memcpy(mutated,raw,bytes);
    CHECK(mutated[764U+8U]==4U);
    mutated[773U]=mutated[774U]=mutated[775U]=0xffU;
    CHECK(TSTIMAGE31(mutated,bytes,0x05000000U,image,
                     TST_MAX_IMAGE,&info)==TST_BAD);
    bytes64=read_file(argv[2],raw);
    CHECK(bytes64==821446U);
    CHECK(TSTHEADER(raw,bytes64,64U,&info)==TST_OK);
    CHECK(info.records==425U && info.max_record==6160U &&
          info.flags==0x11U && info.rmode_any==1U &&
          info.entry_offset==0U && info.input_fnv==0x007eda54U);
    CHECK(TSTIMAGE31(raw,bytes64,0x05000000U,image,
                     TST_MAX_IMAGE,&info)==TST_BAD);
    CHECK(TSTIMAGE64ANY(raw,bytes64,0x09000000U,image,
                        TST_MAX_IMAGE,&info)==TST_OK);
    CHECK(info.image_bytes==767728U &&
          hash_image(image,info.image_bytes)==0x94d5943aU);
    compare_released_loader(raw,bytes64,image,info.image_bytes,
                            0x09000000U,1);
    CHECK(TSTIMAGE64ANY(raw,bytes64,0x0b000000U,image,
                        TST_MAX_IMAGE,&info)==TST_OK);
    CHECK(info.image_bytes==767728U &&
          hash_image(image,info.image_bytes)==0x1c28a98aU);
    compare_released_loader(raw,bytes64,image,info.image_bytes,
                            0x0b000000U,1);
    CHECK(TSTIMAGE64ANY(raw,bytes64,0x20000U,image,
                        TST_MAX_IMAGE,&info)==TST_BAD);
    CHECK(TSTIMAGE64ANY(raw,bytes64,0x09000000U,image,
                        767727U,&info)==TST_BAD);
    CHECK(TSTIMAGE64ANY(raw,bytes64-1U,0x09000000U,image,
                        TST_MAX_IMAGE,&info)==TST_BAD);
    stage_bytes=read_file(argv[4],raw);
    CHECK(stage_bytes==45U*TST_BLOCK);
    CHECK(TSTSTAGEHEADER64(raw,stage_bytes,&bytes,&bytes64)==TST_OK &&
          bytes==821446U && bytes64==45U);
    CHECK(TSTSTAGEVALIDATE(raw,stage_bytes,64U,&info)==TST_OK &&
          info.raw_bytes==821446U && info.records==425U);
    CHECK(TSTSTAGEVALIDATE(raw,stage_bytes,31U,&info)==TST_BAD);
    memcpy(mutated,raw,stage_bytes);
    mutated[64U+100U]^=1U;
    CHECK(TSTSTAGEVALIDATE(mutated,stage_bytes,64U,&info)==TST_BAD);
    memcpy(mutated,raw,stage_bytes);
    mutated[stage_bytes-1U]=1U;
    CHECK(TSTSTAGEVALIDATE(mutated,stage_bytes,64U,&info)==TST_BAD);
    memcpy(mutated,raw,stage_bytes);
    mutated[6U]=0x31U;
    CHECK(TSTSTAGEVALIDATE(mutated,stage_bytes,64U,&info)==TST_BAD);
    stage_bytes=read_file(argv[3],raw);
    CHECK(stage_bytes==76U*TST_BLOCK);
    CHECK(TSTSTAGEVALIDATE(raw,stage_bytes,31U,&info)==TST_OK &&
          info.raw_bytes==1395270U && info.records==1429U);
    memcpy(mutated,raw,stage_bytes);
    mutated[64U+100U]^=1U;
    CHECK(TSTSTAGEVALIDATE(mutated,stage_bytes,31U,&info)==TST_BAD);
    memcpy(mutated,raw,stage_bytes);
    mutated[stage_bytes-1U]=1U;
    CHECK(TSTSTAGEVALIDATE(mutated,stage_bytes,31U,&info)==TST_BAD);
    memcpy(mutated,raw,stage_bytes);
    mutated[15U]=75U;
    CHECK(TSTSTAGEVALIDATE(mutated,stage_bytes,31U,&info)==TST_BAD);
    puts("TSO31 and TSO64 ANY native images: two placements each match released loader; malformed controls pass");
    free(raw); free(mutated); free(image);
    return 0;
}
