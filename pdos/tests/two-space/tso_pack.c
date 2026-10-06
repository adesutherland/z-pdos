/* SPDX-License-Identifier: MIT
 * Wrap one checked native TSO RDW stream in F/18452 blocks for disposable
 * 3390 IPL staging. The guest validates the complete envelope and raw bytes.
 */
#include "twospace_tso.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void put_word(unsigned char *p, unsigned int value)
{
    p[0]=(unsigned char)(value>>24); p[1]=(unsigned char)(value>>16);
    p[2]=(unsigned char)(value>>8); p[3]=(unsigned char)value;
}

int main(int argc, char **argv)
{
    static const unsigned char magic[8]={
        0x50U,0x44U,0x54U,0x53U,0x4fU,0x33U,0x31U,0x01U};
    unsigned char *raw, *stage;
    FILE *input, *output;
    size_t read_bytes;
    unsigned int i, blocks, length, mode, fnv=0x811c9dc5U;
    TSTINFO info;
    if (argc!=3 && argc!=4) return 2;
    mode=argc==4 && strcmp(argv[3],"64")==0 ? 64U :
         argc==4 && strcmp(argv[3],"24")==0 ? 24U : 31U;
    if (argc==4 && mode==31U) return 2;
    raw=(unsigned char *)malloc(TST_MAX_RAW+1U);
    if (!raw) return 3;
    input=fopen(argv[1],"rb");
    if (!input) { free(raw); return 4; }
    read_bytes=fread(raw,1,TST_MAX_RAW+1U,input);
    if (!read_bytes || read_bytes>TST_MAX_RAW || ferror(input) ||
        !feof(input) || fclose(input)!=0 ||
        TSTHEADER(raw,(unsigned int)read_bytes,mode,&info)!=TST_OK) {
        free(raw); return 5;
    }
    blocks=((unsigned int)read_bytes+64U+TST_BLOCK-1U)/TST_BLOCK;
    length=blocks*TST_BLOCK;
    stage=(unsigned char *)calloc(length,1U);
    if (!stage) { free(raw); return 6; }
    for (i=0U; i<8U; ++i) stage[i]=magic[i];
    if (mode==64U) { stage[5U]=0x36U; stage[6U]=0x34U; }
    if (mode==24U) { stage[5U]=0x32U; stage[6U]=0x34U; }
    put_word(stage+8U,(unsigned int)read_bytes);
    put_word(stage+12U,blocks);
    for (i=0U; i<(unsigned int)read_bytes; ++i)
        fnv=(fnv^raw[i])*0x01000193U;
    put_word(stage+16U,fnv);
    memcpy(stage+64U,raw,read_bytes);
    if (TSTSTAGEVALIDATE(stage,length,mode,&info)!=TST_OK) {
        free(stage); free(raw); return 7;
    }
    output=fopen(argv[2],"wb");
    if (!output) { free(stage); free(raw); return 8; }
    if (fwrite(stage,1,length,output)!=length || fclose(output)!=0) {
        free(stage); free(raw); return 9;
    }
    printf("TSO%u stage: %u records, %u raw bytes, %u F/18452 blocks, FNV %08x\n",
           mode,info.records,info.raw_bytes,blocks,fnv);
    free(stage); free(raw);
    return 0;
}
