/* SPDX-License-Identifier: MIT
 * Compare the bounded C31 RMODE64 materializer with the released loader on
 * unchanged native members. The caller supplies their extracted RDW bytes.
 */
#include "twospace_tso.h"
#include "pdosutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr,"tso_high:%d: %s\n",__LINE__,#x); exit(1); } \
} while (0)
#define MAX_RELOCS TST_MAX_HIGH_RELOCS

int int_rdblock(int a, int b, int c, int d, void *e, int f, int g)
{
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g;
    abort(); return -1;
}

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

static unsigned int word(const unsigned char *p)
{
    return ((unsigned int)p[0]<<24) | ((unsigned int)p[1]<<16) |
           ((unsigned int)p[2]<<8) | p[3];
}

static void put_word(unsigned char *p, unsigned int value)
{
    p[0]=(unsigned char)(value>>24); p[1]=(unsigned char)(value>>16);
    p[2]=(unsigned char)(value>>8); p[3]=(unsigned char)value;
}

static void compare_member(const char *path, unsigned int expected_bytes,
                           unsigned int expected_image)
{
    unsigned char *raw=(unsigned char *)malloc(TST_MAX_RAW+1U);
    unsigned char *image=(unsigned char *)malloc(TST_MAX_IMAGE);
    unsigned char *other=(unsigned char *)malloc(TST_MAX_IMAGE);
    unsigned char *released=(unsigned char *)calloc(TST_MAX_IMAGE,1U);
    unsigned int *relocs=(unsigned int *)malloc(MAX_RELOCS*sizeof *relocs);
    unsigned int bytes, i, count;
    int legacy_bytes, entry;
    TSPADDR base;
    TSTINFO info;
    CHECK(raw && image && other && released && relocs);
    bytes=read_file(path,raw);
    CHECK(bytes==expected_bytes);
    CHECK(TSTHEADER(raw,bytes,64U,&info)==TST_BAD);
    CHECK(TSTHEADER64HIGH(raw,bytes,&info)==TST_OK);
    CHECK(info.mode==64U && info.flags==0x31U &&
          info.rmode_high==1U && info.rmode_any==0U &&
          info.entry_offset==0U);
    for (i=0U; i<MAX_RELOCS; ++i) relocs[i]=0xffffffffU;
    base.hi=1U; base.lo=0x10000000U;
    CHECK(TSTIMAGE64HIGH(raw,bytes,base,image,TST_MAX_IMAGE,
                         relocs,MAX_RELOCS,&info)==TST_OK);
    CHECK(info.image_bytes==expected_image);
    count=0U;
    while (count<MAX_RELOCS && relocs[count]!=0xffffffffU) ++count;
    CHECK(count>0U && count<MAX_RELOCS);
    memcpy(released,raw,bytes);
    legacy_bytes=(int)bytes;
    CHECK(fixPEHigh((char *)released,&legacy_bytes,&entry,
                    0x10000000U,TST_MAX_IMAGE)==0);
    CHECK(entry==0 && legacy_bytes==(int)expected_image &&
          memcmp(released,image,expected_image)==0);
    base.hi=2U; base.lo=0x20000000U;
    CHECK(TSTIMAGE64HIGH(raw,bytes,base,other,TST_MAX_IMAGE,
                         relocs,MAX_RELOCS,&info)==TST_OK);
    for (i=0U; i<count; ++i) {
        unsigned int at=relocs[i];
        CHECK(at<=expected_image-8U &&
              word(image+at)==1U && word(other+at)==2U &&
              word(other+at+4U)-word(image+at+4U)==0x10000000U);
        put_word(other+at,word(image+at));
        put_word(other+at+4U,word(image+at+4U));
    }
    CHECK(memcmp(other,image,expected_image)==0);
    base.hi=0U;
    CHECK(TSTIMAGE64HIGH(raw,bytes,base,other,TST_MAX_IMAGE,
                         relocs,MAX_RELOCS,&info)==TST_BAD);
    base.hi=1U; base.lo=0x10000001U;
    CHECK(TSTIMAGE64HIGH(raw,bytes,base,other,TST_MAX_IMAGE,
                         relocs,MAX_RELOCS,&info)==TST_BAD);
    base.lo=0x10000000U;
    CHECK(TSTIMAGE64HIGH(raw,bytes,base,other,expected_image-1U,
                         relocs,MAX_RELOCS,&info)==TST_BAD);
    CHECK(TSTIMAGE64HIGH(raw,bytes,base,other,TST_MAX_IMAGE,
                         relocs,1U,&info)==TST_BAD);
    printf("%s: raw %u, image %u, AL8 %u, two full-width bases pass\n",
           path,bytes,expected_image,count);
    free(relocs); free(released); free(other); free(image); free(raw);
}

int main(int argc, char **argv)
{
    if (argc!=4) return 2;
    compare_member(argv[1],4196266U,3916064U);
    compare_member(argv[2],1842322U,1775648U);
    compare_member(argv[3],1860542U,1799472U);
    return 0;
}
