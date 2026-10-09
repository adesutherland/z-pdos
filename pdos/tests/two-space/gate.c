/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "twospace_fixture.h"
#include "twospace_gate.h"

static unsigned int rights(unsigned int frame, void *unused)
{
    if(unused)++*(unsigned int *)unused;
    if (frame == 0x7000U || frame == 0x12000U || frame == 0x13000U)
        return TSG_READ | TSG_WRITE;
    if (frame == 0x5000U || frame == 0x6000U || frame == 0x11000U ||
        frame == 0x1f000U)
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
    if (TSFBUILD(real,TSF_KPOOL_REAL,TSF_UPOOL_REAL,&built,0,0) ||
        TSDATTACH(&tables,real+TSF_UPOOL_REAL,TSF_UPOOL_REAL,TSF_UPOOL_BYTES,
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
    real[0x12024U]=0xa5U;
    if (TSGPROBE(&gate,&request) != TSG_OK ||
        real[0x12024U] != 0xa5U) return 17;
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
    if (TSGPROBE(&gate,&request) != TSG_DENIED) return 18;
    if (TSGCOPY(&gate,&request,buffer,sizeof buffer) != TSG_DENIED)
        return 6;
    request.address.hi=1U; request.address.lo=0x10002ffeU;
    memset(buffer,0x5aU,sizeof buffer);
    memcpy(unchanged,buffer,sizeof buffer);
    if (TSGPROBE(&gate,&request) != TSG_DENIED ||
        real[0x13ffeU] != 0U || real[0x13fffU] != 0U) return 19;
    if (TSGCOPY(&gate,&request,buffer,sizeof buffer) != TSG_DENIED ||
        memcmp(buffer,unchanged,sizeof buffer) ||
        real[0x13ffeU] != 0U || real[0x13fffU] != 0U) return 7;
    request.address.lo=0x10003ffeU;
    request.direction=TSG_READ;
    if (TSGPROBE(&gate,&request) != TSG_UNMAPPED ||
        TSGCOPY(&gate,&request,buffer,sizeof buffer) != TSG_UNMAPPED)
        return 20;
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
    memcpy(root_entry,real+TSF_UPOOL_REAL,sizeof root_entry);
    real[TSF_UPOOL_REAL+4U]=0x7fU; real[TSF_UPOOL_REAL+5U]=0xffU;
    real[TSF_UPOOL_REAL+6U]=0x00U; real[TSF_UPOOL_REAL+7U]=0x0fU;
    address.hi=1U; address.lo=0x10001000U;
    if (TSDLOOKUP(&tables,address,&address) != TSD_BAD) return 16;
    memcpy(real+TSF_UPOOL_REAL,root_entry,sizeof root_entry);
    {
        unsigned char *bulk=(unsigned char *)malloc(8193U);
        unsigned int checks=0U;
        if(!bulk)return 21;
        gate.rights_context=&checks;
        request.address.hi=1U;request.address.lo=0x10001000U;
        request.length=8192U;request.direction=TSG_READ;
        for(i=0U;i<8192U;++i)real[0x12000U+i]=(unsigned char)(i*17U+3U);
        if(TSGCOPY(&gate,&request,bulk,8193U)!=TSG_BAD||
           TSGSCOPY(&gate,&request,bulk,8193U)!=TSG_OK||checks!=4U){free(bulk);return 22;}
        for(i=0U;i<8192U;++i)if(bulk[i]!=(unsigned char)(i*17U+3U)){free(bulk);return 23;}
        memset(bulk,0xc7U,8193U);request.length=8193U;request.direction=TSG_WRITE;
        real[0x12000U]=0x5aU;real[0x13000U]=0x5bU;
        if(TSGSCOPY(&gate,&request,bulk,8193U)!=TSG_DENIED||
           real[0x12000U]!=0x5aU||real[0x13000U]!=0x5bU){free(bulk);return 24;}
        request.length=TSG_MAX_SPAN+1U;
        if(TSGSPROB(&gate,&request)!=TSG_BAD){free(bulk);return 25;}
        {
            TSPADDR va,ra;
            va.hi=0U;va.lo=0xfffff000U;ra.hi=0U;ra.lo=0x12000U;
            if(TSDMAP(&tables,va,ra)!=TSD_OK){free(bulk);return 26;}
            va.hi=1U;va.lo=0U;ra.lo=0x1f000U;
            if(TSDMAP(&tables,va,ra)!=TSD_OK){free(bulk);return 27;}
            for(i=0U;i<16U;++i){real[0x12ff0U+i]=(unsigned char)i;real[0x1f000U+i]=(unsigned char)(i+16U);}
            request.address.hi=0U;request.address.lo=0xfffffff0U;
            request.length=32U;request.direction=TSG_READ;checks=0U;
            if(TSGSCOPY(&gate,&request,bulk,8193U)!=TSG_OK||checks!=4U){free(bulk);return 28;}
            for(i=0U;i<32U;++i)if(bulk[i]!=(unsigned char)i){free(bulk);return 29;}
            request.direction=TSG_WRITE;real[0x12ff0U]=0x5aU;
            if(TSGSCOPY(&gate,&request,bulk,8193U)!=TSG_DENIED||real[0x12ff0U]!=0x5aU){free(bulk);return 30;}
        }
        gate.rights_context=0;free(bulk);
    }
    if (TSFBUILD(real,0xa000U,TSF_UPOOL_REAL,&built,0,0) == 0) return 13;
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
    puts("K/U gate: full-width, page crossing, non-mutating probes, permissions and atomic failures pass");
    return 0;
}
