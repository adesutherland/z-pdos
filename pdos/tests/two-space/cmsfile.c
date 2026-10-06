/* SPDX-License-Identifier: MIT
 * Corrupt staged CMS files must be refused before K exposes any record.
 */
#include "twospace_cmsfile.h"
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (0)

static void put(unsigned char *p, unsigned int value)
{
    p[0]=(unsigned char)(value>>24); p[1]=(unsigned char)(value>>16);
    p[2]=(unsigned char)(value>>8); p[3]=(unsigned char)value;
}

static void seal(unsigned char *block)
{
    unsigned int i, hash=0x811c9dc5U;
    for (i=64U; i<72U; ++i)
        hash=(hash^(unsigned int)block[i])*0x01000193U;
    put(block+56U,hash);
}

static int run(void)
{
    static unsigned char data[TSIF_BLOCK];
    TSIFINFO info, retained;
    memcpy(data,"PDCMSF01",8U);
    put(data+8U,31U); put(data+12U,8U);
    put(data+16U,4U); put(data+20U,2U);
    data[64U]=0U; data[65U]=3U;
    data[66U]='A'; data[67U]='B'; data[68U]='C';
    data[69U]=0U; data[70U]=1U; data[71U]='Z';
    seal(data);
    CHECK(TSIFHEADER(data,sizeof data,31U,&info)==TSIF_OK);
    CHECK(info.payload_bytes==8U && info.source_bytes==4U &&
          info.records==2U && info.blocks==1U);
    CHECK(TSIFVALIDATE(data,sizeof data,31U,&retained)==TSIF_OK);
    CHECK(retained.payload_bytes==8U && retained.records==2U);
    CHECK(TSIFVALIDATE(data,sizeof data,24U,&info)==TSIF_BAD);
    CHECK(TSIFVALIDATE(data,sizeof data-1U,31U,&info)==TSIF_BAD);
    data[72U]=1U;
    CHECK(TSIFVALIDATE(data,sizeof data,31U,&info)==TSIF_BAD);
    data[72U]=0U;
    data[66U]='X';
    CHECK(TSIFVALIDATE(data,sizeof data,31U,&info)==TSIF_BAD);
    data[66U]='A';
    data[65U]=0U; seal(data);
    CHECK(TSIFVALIDATE(data,sizeof data,31U,&info)==TSIF_BAD);
    data[65U]=3U; seal(data);
    put(data+16U,5U);
    CHECK(TSIFVALIDATE(data,sizeof data,31U,&info)==TSIF_BAD);
    put(data+16U,4U);
    put(data+20U,1U);
    CHECK(TSIFVALIDATE(data,sizeof data,31U,&info)==TSIF_BAD);
    put(data+20U,2U);
    data[60U]=1U;
    CHECK(TSIFHEADER(data,sizeof data,31U,&info)==TSIF_BAD);
    data[60U]=0U;
    put(data+12U,TSIF_LIMIT);
    CHECK(TSIFHEADER(data,sizeof data,31U,&info)==TSIF_BAD);
    put(data+12U,8U);
    data[0U]='X';
    CHECK(TSIFHEADER(data,sizeof data,31U,&info)==TSIF_BAD);
    return 0;
}

int main(void)
{
    int at=run();
    if (at) { fprintf(stderr,"CMS file validation failed at %d\n",at);
              return 1; }
    puts("CMS file envelope: valid records and corruption controls pass");
    return 0;
}
