/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "twospace_fixture.h"
#include "twospace_gate.h"

static unsigned int rights(unsigned int frame, void *unused)
{
    (void)unused;
    if (frame == 0x7000U || frame == 0x12000U || frame == 0x13000U)
        return TSG_READ | TSG_WRITE;
    if (frame == 0x5000U || frame == 0x6000U || frame == 0x11000U)
        return TSG_READ;
    return 0U;
}

static int test(unsigned char *real)
{
    TSFRESULT built;
    TSDSTATE tables;
    TSGCONTEXT gate;
    TSGREQUEST request;
    unsigned char buffer[TSG_MAX_COPY];
    unsigned char unchanged[TSG_MAX_COPY];
    unsigned char root_entry[8];
    TSPADDR address;
    unsigned int i;
    if (TSFBUILD(real,0x100000U,0x180000U,&built,0,0) ||
        TSDATTACH(&tables,real+0x180000U,0x180000U,TSF_UPOOL_BYTES,
                  built.ubytes,built.uasce) != TSD_OK) return 1;
    gate.u_tables=&tables; gate.real_aperture=real;
    gate.real_bytes=TSF_REAL_BYTES; gate.rights=rights;
    gate.rights_context=0;
    request.address.hi=1U; request.address.lo=0x10001ffeU;
    request.length=4U; request.direction=TSG_READ; request.svc=1U;
    real[0x12ffeU]=0xa1U; real[0x12fffU]=0xb2U;
    real[0x13000U]=0xc3U; real[0x13001U]=0xd4U;
    memset(buffer,0,sizeof buffer);
    if (TSGCOPY(&gate,&request,buffer,sizeof buffer) != TSG_OK ||
        buffer[0] != 0xa1U || buffer[1] != 0xb2U ||
        buffer[2] != 0xc3U || buffer[3] != 0xd4U) return 2;
    request.address.lo=0x10001f80U; request.length=TSG_MAX_COPY;
    for (i=0U; i<TSG_MAX_COPY; ++i) real[0x12f80U+i]=(unsigned char)i;
    if (TSGCOPY(&gate,&request,buffer,sizeof buffer) != TSG_OK)
        return 14;
    for (i=0U; i<TSG_MAX_COPY; ++i)
        if (buffer[i] != (unsigned char)i) return 15;
    request.address.lo=0x10001024U; request.direction=TSG_WRITE;
    request.length=4U;
    buffer[0]=0x55U; buffer[1]=0x66U; buffer[2]=0x77U; buffer[3]=0x88U;
    if (TSGCOPY(&gate,&request,buffer,sizeof buffer) != TSG_OK ||
        memcmp(real+0x12024U,buffer,4U)) return 3;
    request.address.hi=0x10U; request.address.lo=0x21000U;
    request.direction=TSG_READ;
    if (TSGCOPY(&gate,&request,buffer,sizeof buffer) != TSG_UNMAPPED)
        return 4;
    request.address.hi=0xffffffffU;
    request.address.lo=0xffffffffU;
    if (TSGCOPY(&gate,&request,buffer,sizeof buffer) != TSG_BAD) return 5;
    request.address.hi=0U; request.address.lo=0x20000U;
    request.direction=TSG_WRITE;
    if (TSGCOPY(&gate,&request,buffer,sizeof buffer) != TSG_DENIED)
        return 6;
    request.address.hi=1U; request.address.lo=0x10002ffeU;
    memset(buffer,0x5aU,sizeof buffer);
    memcpy(unchanged,buffer,sizeof buffer);
    if (TSGCOPY(&gate,&request,buffer,sizeof buffer) != TSG_UNMAPPED ||
        memcmp(buffer,unchanged,sizeof buffer) ||
        real[0x13ffeU] != 0U || real[0x13fffU] != 0U) return 7;
    request.length=0U;
    if (TSGCOPY(&gate,&request,buffer,sizeof buffer) != TSG_BAD) return 8;
    request.length=TSG_MAX_COPY+1U;
    if (TSGCOPY(&gate,&request,buffer,sizeof buffer) != TSG_BAD) return 9;
    request.length=4U; request.direction=3U;
    if (TSGCOPY(&gate,&request,buffer,sizeof buffer) != TSG_BAD) return 10;
    request.direction=TSG_READ;
    if (TSGCOPY(&gate,&request,buffer,3U) != TSG_BAD) return 11;
    address.hi=0U; address.lo=0x20000U;
    if (TSDLOOKUP(&tables,address,&address) != TSD_OK ||
        address.hi != 0U || address.lo != 0x5000U) return 12;
    memcpy(root_entry,real+0x180000U,sizeof root_entry);
    real[0x180004U]=0x7fU; real[0x180005U]=0xffU;
    real[0x180006U]=0x00U; real[0x180007U]=0x0fU;
    address.hi=1U; address.lo=0x10001000U;
    if (TSDLOOKUP(&tables,address,&address) != TSD_BAD) return 16;
    memcpy(real+0x180000U,root_entry,sizeof root_entry);
    if (TSFBUILD(real,0xa000U,0x180000U,&built,0,0) == 0) return 13;
    return 0;
}

int main(void)
{
    unsigned char *real=(unsigned char *)calloc(TSF_REAL_BYTES,1U);
    int result;
    if (!real) return 99;
    result=test(real);
    free(real);
    if (result) { fprintf(stderr,"K/U gate control %d failed\n",result); return 1; }
    puts("K/U gate: full-width, page crossing, writes, permissions and atomic failures pass");
    return 0;
}
