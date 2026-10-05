/* SPDX-License-Identifier: MIT
 * Successor IPL proof: PLOAD enters this small native C31 staging module.
 * It reads a checked sparse core record stream into real 4-6 MiB, puts the
 * PIC launch stub at 8 MiB, and transfers to the stub. The stub copies the
 * core to its real addresses after this module stops running.
 */
#include <stdio.h>
#include <string.h>
#include "pdosutil.h"

#define BLOCK 18452
#define CORE_SIZE 0x200000U
#define STAGING 0x400000U
#define LAUNCH 0x800000U
#define DIAG (*(volatile unsigned int *)0x30000U)
#define DETAIL ((volatile unsigned int *)0x30000U)
#define FAIL(n) do { DIAG = 0x54530000U | (unsigned int)(n); return (n); } while (0)
static unsigned char record_buf[BLOCK];
static unsigned char seen[64];

int initsys(void);
int rdblock(int dev, int cyl, int head, int rec, void *buf, int len, int cmd);
int int_rdblock(int dev, int cyl, int head, int rec, void *buf, int len, int cmd)
{ return rdblock(dev,cyl,head,rec,buf,len,cmd); }

static unsigned int be16(const unsigned char *p)
{ return ((unsigned int)p[0]<<8) | (unsigned int)p[1]; }
static unsigned int be32(const unsigned char *p)
{ return ((unsigned int)p[0]<<24) | ((unsigned int)p[1]<<16) |
         ((unsigned int)p[2]<<8) | (unsigned int)p[3]; }

int main(int argc, char **argv)
{
    int device, cyl, head, rec, count, seq = 0, complete = 0, launched = 0;
    unsigned int used, i, real, size, page;
    (void)argc; (void)argv;
    DIAG = 0x54530001U;
    device = initsys();
    DIAG = 0x54530002U;
    if (findFile(device,"COMMAND.EXE",&cyl,&head,&rec) != 0) {
        DIAG = 0x5453000cU;
        return 12;
    }
    DIAG = 0x54530003U;
    memset((void *)STAGING,0,CORE_SIZE);
    memset(seen,0,sizeof seen);
    while (seq < 128) {
        count = rdblock(device,cyl,head,rec,record_buf,BLOCK,0x0e);
        if (count < 0) {
            rec = 1; ++head;
            count = rdblock(device,cyl,head,rec,record_buf,BLOCK,0x0e);
            if (count < 0) {
                head = 0; ++cyl;
                count = rdblock(device,cyl,head,rec,record_buf,BLOCK,0x0e);
            }
        }
        DETAIL[1] = (unsigned int)count;
        DETAIL[2] = (unsigned int)cyl;
        DETAIL[3] = (unsigned int)head;
        DETAIL[4] = (unsigned int)rec;
        DETAIL[5] = be32(record_buf);
        DETAIL[6] = be32(record_buf+4);
        if (count != BLOCK || be32(record_buf) != 0x54535031U ||
            be16(record_buf+4) != (unsigned int)seq) FAIL(16);
        used = 8U;
        count = (int)be16(record_buf+6);
        if (count == 0) { complete = 1; break; }
        for (i = 0; i < (unsigned int)count; ++i) {
            if (used + 6U > BLOCK) FAIL(20);
            real = be32(record_buf+used);
            size = be16(record_buf+used+4U);
            used += 6U;
            DETAIL[7] = real; DETAIL[8] = size; DETAIL[9] = i;
            if (!size || size > 4096U || used + size > BLOCK) FAIL(24);
            if (real == LAUNCH) {
                if (launched || size > 4096U) FAIL(28);
                memcpy((void *)LAUNCH,record_buf+used,size);
                launched = 1;
            } else {
                if ((real & 4095U) || real >= CORE_SIZE ||
                    size != 4096U) FAIL(32);
                page = real >> 12;
                if (seen[page >> 3] & (1U << (page & 7U))) FAIL(36);
                seen[page >> 3] |= (unsigned char)(1U << (page & 7U));
                memcpy((void *)(STAGING+real),record_buf+used,size);
            }
            used += size;
        }
        ++rec; ++seq;
        DIAG = 0x54530100U | (unsigned int)seq;
    }
    if (!complete || !launched || !seq) { DIAG = 0x54530028U; return 40; }
    DIAG = 0x5453ffffU;
    ((void (*)(void))LAUNCH)();
    return 44;
}
