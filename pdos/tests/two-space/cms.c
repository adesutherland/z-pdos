/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "twospace_cms.h"

static void put(unsigned char *p, unsigned int n)
{ p[0]=(unsigned char)(n>>24); p[1]=(unsigned char)(n>>16);
  p[2]=(unsigned char)(n>>8); p[3]=(unsigned char)n; }
static unsigned int get(const unsigned char *p)
{ return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|
         ((unsigned int)p[2]<<8)|(unsigned int)p[3]; }
static unsigned int source_byte(const unsigned char *stage, unsigned int where)
{
    unsigned int cursor=64U, size;
    size=((unsigned int)stage[cursor]<<8)|stage[cursor+1U];
    cursor+=2U+size;
    for (;;) {
        size=((unsigned int)stage[cursor]<<8)|stage[cursor+1U];
        if (where < size) return stage[cursor+2U+where];
        where-=size; cursor+=2U+size;
    }
}

int main(int argc, char **argv)
{
    unsigned char block[18452];
    unsigned char *whole, *image;
    unsigned int fnv=0x811c9dc5U, i;
    unsigned int entry, base, cursor, size, image_count, address, old;
    TSHINFO info;
    FILE *source;
    size_t count, bytes;
    long file_size;
    memset(block,0,sizeof block);
    memcpy(block,"PDCMSM01",8U);
    put(block+8U,24U); put(block+12U,4180U);
    put(block+16U,4096U); put(block+20U,2U);
    block[64U]=0U; block[65U]=80U;
    put(block+66U,0x20000U); put(block+70U,0x20000U);
    put(block+74U,0x21000U); put(block+78U,0x21000U);
    block[109U]=0x80U;
    block[146U]=0x10U; block[147U]=0U;
    for (i=64U; i<64U+4180U; ++i)
        fnv=(fnv ^ block[i])*0x01000193U;
    put(block+56U,fnv);
    if (TSHHEADER(block,sizeof block,24U,&info) != TSH_OK ||
        info.origin != 0x20000U || info.image_bytes != 4096U ||
        TSHVALIDATE(block,sizeof block,24U,&info) != TSH_OK ||
        TSHHEADER(block,sizeof block,31U,&info) != TSH_BAD)
        return 1;
    image=(unsigned char *)malloc(4096U);
    if (!image || TSHIMAGE(block,sizeof block,24U,0x20000U,
                           image,4096U,&entry) != TSH_OK ||
        entry != 0x20000U || image[0] != 0U ||
        TSHIMAGE(block,sizeof block,24U,0x22000U,
                 image,4096U,&entry) != TSH_BAD) return 12;
    free(image);
    block[109U]=0U;
    if (TSHHEADER(block,sizeof block,24U,&info) != TSH_BAD) return 2;
    block[109U]=0x80U; put(block+74U,0x22000U);
    if (TSHHEADER(block,sizeof block,24U,&info) != TSH_BAD) return 3;
    put(block+74U,0x21000U); block[148U]=1U;
    if (TSHVALIDATE(block,sizeof block,24U,&info) != TSH_BAD) return 4;
    if (argc == 1) return 0;
    if (argc != 3) return 5;
    source=fopen(argv[1],"rb");
    if (!source) return 6;
    if (fseek(source,0,SEEK_END) || (file_size=ftell(source)) < 146L ||
        file_size > 9L*1024L*1024L || fseek(source,0,SEEK_SET)) return 7;
    bytes=(size_t)file_size;
    whole=(unsigned char *)malloc(bytes);
    if (!whole) return 8;
    count=fread(whole,1,bytes,source);
    if (ferror(source) || fclose(source) || count != bytes) return 9;
    if (TSHVALIDATE(whole,(unsigned int)bytes,(unsigned int)atoi(argv[2]),
                    &info) != TSH_OK) return 10;
    image=(unsigned char *)malloc(info.image_bytes);
    if (!image) return 13;
    base=info.profile == 24U ? info.origin : 0x03000000U;
    if (TSHIMAGE(whole,(unsigned int)bytes,info.profile,base,image,
                 info.image_bytes,&entry) != TSH_OK ||
        entry != base+info.entry-info.origin ||
        TSHIMAGE(whole,(unsigned int)bytes,info.profile,base,image,
                 info.image_bytes-1U,&entry) != TSH_BAD)
        return 14;
    cursor=64U;
    size=((unsigned int)whole[cursor]<<8)|whole[cursor+1U];
    cursor+=2U+size;
    image_count=(info.image_bytes+65534U)/65535U;
    for (i=0U; i<image_count; ++i) {
        size=((unsigned int)whole[cursor]<<8)|whole[cursor+1U];
        if (i == 0U && image[0] != whole[cursor+2U]) return 15;
        cursor+=2U+size;
    }
    if (info.profile == 31U) {
        size=((unsigned int)whole[cursor]<<8)|whole[cursor+1U];
        cursor+=2U+size;
        size=((unsigned int)whole[cursor]<<8)|whole[cursor+1U];
        if (size < 5U || whole[cursor+2U] != 3U) return 16;
        address=get(whole+cursor+3U)-info.origin;
        old=0U;
        for (i=0U; i<4U; ++i)
            old=(old<<8)|source_byte(whole,address+i);
        if (get(image+address) != base+old-info.origin) return 17;
    }
    free(image);
    whole[64U+info.module_bytes-1U]^=1U;
    if (TSHVALIDATE(whole,(unsigned int)bytes,(unsigned int)atoi(argv[2]),
                    &info) != TSH_BAD) return 11;
    whole[64U+info.module_bytes-1U]^=1U;
    printf("CMS%d MODULE origin=%08x entry=%08x image=%u records=%u RLD=%u\n",
           info.profile,info.origin,info.entry,info.image_bytes,
           info.records,info.relocation_records);
    free(whole);
    return 0;
}
