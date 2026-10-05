/* Host controls for product C89 full-width region-first DAT builder. */
#include <stdio.h>
#include <stdlib.h>
#include "twospace_dat.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"dat:%d: %s\n",__LINE__,#x); return 1; } } while (0)
static TSPADDR address(unsigned int hi, unsigned int lo)
{ TSPADDR a; a.hi = hi; a.lo = lo; return a; }
static unsigned int word(const unsigned char *p)
{ return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|
         ((unsigned int)p[2]<<8)|(unsigned int)p[3]; }
static void purge(void *context)
{ ++*(unsigned int *)context; }

int main(void)
{
    TSDSTATE k, u, small, attached;
    TSPADDR previous;
    unsigned int purges = 0U;
    unsigned char *kp, *up, *tiny;
    kp = (unsigned char *)malloc(0x40000U);
    up = (unsigned char *)malloc(0x40000U);
    tiny = (unsigned char *)malloc(16384U);
    CHECK(kp && up && tiny);
    CHECK(TSDINIT(&k, kp, 0x100000U, 0x40000U) == TSD_OK);
    CHECK(TSDINIT(&u, up, 0x140000U, 0x40000U) == TSD_OK);
    CHECK(k.asce_lo == 0x10000fU && u.asce_lo == 0x14000fU);
    CHECK(TSDMAP(&k, address(0x01000000U,0), address(0,0x9000)) == TSD_OK);
    CHECK(k.used == 69632U); /* R1/R2/R3/segment plus one page table */
    CHECK(word(kp + 8U * 8U + 4U) == 0x10400fU); /* 2^56: R1 index 8 */
    CHECK(TSDMAP(&k, address(0,0x02000000U), address(0,0xa000)) == TSD_OK);
    CHECK(TSDMAP(&k, address(0,0x02000000U), address(0,0xb000)) == TSD_EXISTS);
    CHECK(TSDMAP(&u, address(0,0x20000U), address(0,0x5000)) == TSD_OK);
    CHECK(TSDMAP(&u, address(0,0x02000000U), address(0,0xd000)) == TSD_OK);
    CHECK(TSDMAP(&u, address(0x00000001U,0x10000000U),
                 address(0,0x6000)) == TSD_OK);
    CHECK(word(up + 8U * 8U + 4U) == 0x20U); /* K high R1 absent in U */
    CHECK(TSDINIT(&small, tiny, 0x180000U, 16384U) == TSD_OK);
    CHECK(TSDMAP(&small, address(0,0), address(0,0)) == TSD_FULL);
    CHECK(small.used == 16384U); /* failure leaves no partial tables */
    CHECK(TSDMAP(&k, address(0,1), address(0,0)) == TSD_BAD);
    CHECK(TSDMAP(&k, address(0,0x500000U), address(0,0x100000U)) == TSD_BAD);
    CHECK(TSDMAPTABLE(&k,address(0,0x05000000U),
                      address(0,0x100000U)) == TSD_OK);
    CHECK(TSDLIVE(&u,0,0) == TSD_BAD);
    CHECK(TSDLIVE(&u,purge,&purges) == TSD_OK);
    CHECK(TSDUNMAP(&u,address(0,0x20000U),&previous) == TSD_OK);
    CHECK(previous.hi == 0U && previous.lo == 0x5000U && purges == 1U);
    CHECK(TSDUNMAP(&u,address(0,0x20000U),&previous) == TSD_MISSING);
    CHECK(purges == 1U);
    CHECK(TSDMAP(&u,address(0,0x20000U),address(0,0x5000U)) == TSD_OK);
    CHECK(purges == 2U);
    CHECK(TSDMAP(&u,address(0,0x20000U),address(0,0x5000U)) == TSD_EXISTS);
    CHECK(purges == 2U);
    CHECK(TSDATTACH(&attached,up,0x140000U,0x40000U,u.used,u.asce_lo)
          == TSD_OK);
    CHECK(TSDLIVE(&attached,purge,&purges) == TSD_OK);
    CHECK(TSDUNMAP(&attached,address(0,0x20000U),&previous) == TSD_OK);
    CHECK(previous.lo == 0x5000U && purges == 3U);
    CHECK(TSDMAP(&attached,address(0,0x20000U),
                 address(0,0x5000U)) == TSD_OK && purges == 4U);
    free(kp); free(up); free(tiny);
    puts("full-width sparse DAT: high K, separate U, live purge, unmap and remap pass");
    return 0;
}
