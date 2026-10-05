/* SPDX-License-Identifier: MIT
 * Checked C31 successor handover. PLOAD and this stage run with DAT off.
 * The package contains a bare core; all K/U DAT is built here in the guest.
 */
#include <stdio.h>
#include <string.h>
#include "pdosutil.h"
#include "twospace_fixture.h"
#include "twospace_real.h"

#define BLOCK 18452U
#define CORE_SIZE TSF_CORE_BYTES
#define PAGE 4096U
#define DATA_RECORDS 40U
#define DIAG (*(volatile unsigned int *)0x30000U)
#define DETAIL ((volatile unsigned int *)0x30000U)
#define FAIL(n) do { DIAG = 0x54530000U | (unsigned int)(n); return (n); } while (0)
#define STAGE_OWNER 3U
#define LAUNCH_OWNER 4U
static unsigned char record_buf[BLOCK];
static unsigned char seen[CORE_SIZE / PAGE / 8U];
static TSRPLAN handover;
static TSRPLAN final_core;
static unsigned int purges;
void TSFPURGE(void *unused);
static void purge_callback(void *unused)
{ (void)unused; ++purges; TSFPURGE(0); }

int initsys(void);
int rdblock(int dev, int cyl, int head, int rec, void *buf, int len, int cmd);
int int_rdblock(int dev, int cyl, int head, int rec, void *buf, int len, int cmd)
{ return rdblock(dev,cyl,head,rec,buf,len,cmd); }

static unsigned int be16(const unsigned char *p)
{ return ((unsigned int)p[0]<<8) | (unsigned int)p[1]; }
static unsigned int be32(const unsigned char *p)
{ return ((unsigned int)p[0]<<24) | ((unsigned int)p[1]<<16) |
         ((unsigned int)p[2]<<8) | (unsigned int)p[3]; }
static void put32(unsigned char *p, unsigned int n)
{
    p[0]=(unsigned char)(n>>24); p[1]=(unsigned char)(n>>16);
    p[2]=(unsigned char)(n>>8); p[3]=(unsigned char)n;
}
static void put64(unsigned char *p, unsigned int n)
{ put32(p,0U); put32(p+4,n); }
static int zeroes(const unsigned char *p, unsigned int n)
{
    unsigned int i;
    for (i=0U; i<n; ++i) if (p[i]) return 0;
    return 1;
}
static unsigned int crc32_bytes(const unsigned char *p, unsigned int n)
{
    unsigned int crc = 0xffffffffU, i, j;
    for (i=0U; i<n; ++i) {
        crc ^= p[i];
        for (j=0U; j<8U; ++j)
            crc = (crc >> 1) ^ ((crc & 1U) ? 0xedb88320U : 0U);
    }
    return crc ^ 0xffffffffU;
}

/* The DSCB first extent bounds every probe, including track rollover. */
static int next_record(int dev, int *c, int *h, int *r,
                       int endc, int endh)
{
    int got;
    while (*c < endc || (*c == endc && *h <= endh)) {
        got = rdblock(dev,*c,*h,*r,record_buf,(int)BLOCK,0x0e);
        if (got >= 0) { ++*r; return got; }
        *r=1; ++*h;
        if (*h == 15) { *h=0; ++*c; }
    }
    return -1;
}

static int patch_launch(unsigned char *stub, unsigned int length,
                        unsigned int launch, unsigned int source,
                        unsigned int entry)
{
    unsigned int i, where=0U, found=0U;
    if (length < 48U) return -1;
    for (i=0U; i<=length-48U; ++i)
        if (be32(stub+i) == 0x54534c32U) { where=i; ++found; }
    if (found != 1U || !zeroes(stub+where+4U,28U) ||
        be32(stub+where+32U) != 1U ||
        be32(stub+where+36U) != 0x80000000U ||
        !zeroes(stub+where+40U,8U) ||
        (launch < source + CORE_SIZE && source < launch + PAGE))
        return -1;
    put64(stub+where+8U,0U);
    put64(stub+where+16U,source);
    put64(stub+where+24U,CORE_SIZE);
    put64(stub+where+40U,entry);
    return 0;
}

int main(int argc, char **argv)
{
    int device, cyl, head, rec, endc, endh, got;
    unsigned int records, core_crc, launch_crc, launch_len, entry;
    unsigned int stage, launch, i, j, used, count, at, size, page;
    unsigned int kpool, upool, launch_seen=0U;
    TSFRESULT dat;
    unsigned char *core, *stub;
    (void)argc; (void)argv;
    DIAG = 0x54530001U;
    device = initsys();
    if (findFileExtent(device,"KCORE.BIN",&cyl,&head,&rec,&endc,&endh) != 0)
        FAIL(12);
    if (endc < cyl || (endc == cyl && endh < head) ||
        endc >= 100 || head < 0 || head >= 15 || endh < 0 || endh >= 15)
        FAIL(13);
    got = next_record(device,&cyl,&head,&rec,endc,endh);
    if (got != (int)BLOCK || be32(record_buf) != 0x54535032U ||
        be16(record_buf+4U) != 2U || be16(record_buf+6U) != 40U ||
        be32(record_buf+8U) != CORE_SIZE ||
        be32(record_buf+12U) != TSF_REAL_BYTES ||
        !zeroes(record_buf+40U,BLOCK-40U) ||
        crc32_bytes(record_buf,36U) != be32(record_buf+36U)) FAIL(16);
    records = be32(record_buf+16U);
    core_crc = be32(record_buf+20U);
    launch_crc = be32(record_buf+24U);
    launch_len = be32(record_buf+28U);
    entry = be32(record_buf+32U);
    if (!records || records > DATA_RECORDS || !launch_len ||
        launch_len > PAGE || entry != 0x1000U) FAIL(17);
    if (TSRINIT(&handover,TSF_REAL_BYTES) != TSR_OK ||
        TSRRESERVE(&handover,1U,0U,0x400000U,TSR_LOAD) != TSR_OK ||
        TSRRESERVE(&handover,5U,0x00f00000U,0x100000U,
                   TSR_LOAD) != TSR_OK ||
        TSRRESERVE(&handover,2U,0U,CORE_SIZE,TSR_COPY|TSR_RUN) != TSR_OK ||
        TSRALLOC(&handover,STAGE_OWNER,CORE_SIZE,0x400000U,
                 TSF_REAL_BYTES,TSR_LOAD|TSR_COPY,&stage) != TSR_OK ||
        TSRALLOC(&handover,LAUNCH_OWNER,PAGE,0x400000U,
                 TSF_REAL_BYTES,TSR_LOAD|TSR_COPY,&launch) != TSR_OK)
        FAIL(18);
    core = (unsigned char *)stage; stub = (unsigned char *)launch;
    memset(core,0,CORE_SIZE); memset(stub,0,PAGE); memset(seen,0,sizeof seen);
    for (i=0U; i<records; ++i) {
        got = next_record(device,&cyl,&head,&rec,endc,endh);
        if (got != (int)BLOCK || be32(record_buf) != 0x54534432U ||
            be16(record_buf+4U) != i) FAIL(20);
        count = be16(record_buf+6U);
        if (!count || count > 4U) FAIL(21);
        used = 8U;
        for (j=0U; j<count; ++j) {
            if (used > BLOCK-6U) FAIL(22);
            at=be32(record_buf+used); size=be16(record_buf+used+4U);
            used += 6U;
            if (!size || size > PAGE || size > BLOCK-used) FAIL(23);
            if (at == CORE_SIZE) {
                if (launch_seen || size != launch_len) FAIL(24);
                memcpy(stub,record_buf+used,size);
                launch_seen=1U;
            } else {
                if ((at & (PAGE-1U)) || at >= CORE_SIZE || size != PAGE)
                    FAIL(25);
                page=at/PAGE;
                if (seen[page>>3] & (1U<<(page&7U))) FAIL(26);
                seen[page>>3] |= (unsigned char)(1U<<(page&7U));
                memcpy(core+at,record_buf+used,PAGE);
            }
            used += size;
        }
        if (!zeroes(record_buf+used,BLOCK-used)) FAIL(27);
    }
    got = next_record(device,&cyl,&head,&rec,endc,endh);
    if (got != (int)BLOCK || be32(record_buf) != 0x54534532U ||
        be16(record_buf+4U) != records || be16(record_buf+6U) != 0U ||
        !zeroes(record_buf+8U,BLOCK-8U) || !launch_seen ||
        crc32_bytes(core,CORE_SIZE) != core_crc ||
        crc32_bytes(stub,launch_len) != launch_crc) FAIL(28);
    if (be32(core+0x2000U) != 0x5044324eU ||
        !zeroes(core+0x4000U,16U) ||
        !zeroes(core+0x100000U,0x80000U)) FAIL(29);
    /* Reserve every supplied final image page before choosing DAT pools. */
    if (TSRINIT(&final_core,CORE_SIZE) != TSR_OK) FAIL(30);
    for (i=0U; i<CORE_SIZE/PAGE; ++i)
        if (seen[i>>3] & (1U<<(i&7U)))
            if (TSRRESERVE(&final_core,100U+i,i*PAGE,PAGE,TSR_RUN) != TSR_OK)
                FAIL(31);
    if (TSRALLOC(&final_core,3U,TSF_POOL_BYTES,0x100000U,
                 CORE_SIZE,TSR_RUN,&kpool) != TSR_OK ||
        TSRALLOC(&final_core,4U,TSF_POOL_BYTES,0x100000U,
                 CORE_SIZE,TSR_RUN,&upool) != TSR_OK ||
        TSFBUILD(core,kpool,upool,&dat,purge_callback,0) ||
        purges != 2U) FAIL(32);
    if (patch_launch(stub,launch_len,launch,stage,entry)) FAIL(33);
    /* Checked handover receipt is retained in the copied core. */
    put32(core+0x4080U,0x54535232U);
    put32(core+0x4084U,stage); put32(core+0x4088U,launch);
    put32(core+0x408cU,kpool); put32(core+0x4090U,upool);
    put32(core+0x4094U,dat.kbytes); put32(core+0x4098U,dat.ubytes);
    put32(core+0x409cU,TSF_REAL_BYTES);
    put32(core+0x40a0U,purges);
    DETAIL[1]=stage; DETAIL[2]=launch;
    DIAG = 0x5453ffffU;
    ((void (*)(void))stub)();
    return 44;
}
