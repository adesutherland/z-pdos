/* Host controls for product C89 full-width region-first DAT builder. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "twospace_dat.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"dat:%d: %s\n",__LINE__,#x); return 1; } } while (0)
static TSPADDR address(unsigned int hi, unsigned int lo)
{ TSPADDR a; a.hi = hi; a.lo = lo; return a; }
static unsigned int word(const unsigned char *p)
{ return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|
         ((unsigned int)p[2]<<8)|(unsigned int)p[3]; }
static void purge(void *context)
{ ++*(unsigned int *)context; }

typedef struct {
    TSDSTATE *state;
    TSPADDR va;
    unsigned int unmapping, calls, failures;
} PURGEORDER;

static void purge_order(void *context)
{
    PURGEORDER *o = (PURGEORDER *)context;
    TSPADDR found;
    ++o->calls;
    if (o->unmapping &&
        (TSDLOOKUP(o->state,o->va,&found) != TSD_MISSING ||
         o->state->used != 69632U ||
         word(o->state->pool+16384U) != 0U)) ++o->failures;
}

static int recycling(void)
{
    TSDSTATE s, attached;
    TSPADDR old, stable = address(0x00200000U,0x20000U), transient;
    unsigned int i, used;
    unsigned char *pool, *snapshot;
    PURGEORDER order;
    /* Exactly two independent full paths: root + 2*(3 regions + page). */
    pool = (unsigned char *)calloc(122880U,1U);
    snapshot = (unsigned char *)malloc(122880U);
    CHECK(pool && snapshot);
    CHECK(TSDINIT(&s,pool,0x200000U,122880U) == TSD_OK);
    CHECK(TSDMAP(&s,address(0U,0x20000U),address(0U,0x9000U)) == TSD_OK);
    CHECK(TSDMAP(&s,stable,address(0U,0xa000U)) == TSD_OK);
    CHECK(s.used == s.capacity);
    CHECK(TSDUNMAP(&s,address(0U,0x20000U),&old) == TSD_OK);
    CHECK(old.lo == 0x9000U && s.used == s.capacity);
    CHECK(TSDATTACH(&attached,pool,s.pool_real,s.capacity,s.used,s.asce_lo)
          == TSD_OK);
    s = attached; /* Free holes survive reopening through the K alias. */
    for (i = 0U; i < 100U; ++i) {
        transient = address((i + 2U) << 21,0x20000U);
        CHECK(TSDMAP(&s,transient,address(0U,0xb000U)) == TSD_OK);
        CHECK(TSDLOOKUP(&s,stable,&old) == TSD_OK && old.lo == 0xa000U);
        CHECK(TSDLOOKUP(&s,transient,&old) == TSD_OK && old.lo == 0xb000U);
        CHECK(s.used == s.capacity);
        CHECK(TSDUNMAP(&s,transient,&old) == TSD_OK);
        CHECK(TSDLOOKUP(&s,transient,&old) == TSD_MISSING);
    }
    CHECK(TSDMAP(&s,address(stable.hi,stable.lo+4096U),
                 address(0U,0xc000U)) == TSD_OK);
    CHECK(TSDUNMAP(&s,stable,&old) == TSD_OK);
    CHECK(TSDLOOKUP(&s,address(stable.hi,stable.lo+4096U),&old)
          == TSD_OK && old.lo == 0xc000U);
    CHECK(TSDUNMAP(&s,address(stable.hi,stable.lo+4096U),&old) == TSD_OK);
    CHECK(s.used == 16384U);

    /* Reuse an interior page-table hole without disturbing adjacent tables. */
    CHECK(TSDMAP(&s,address(0U,0U),address(0U,0x9000U)) == TSD_OK);
    CHECK(TSDMAP(&s,address(0U,0x100000U),address(0U,0xa000U)) == TSD_OK);
    used = s.used;
    CHECK(TSDUNMAP(&s,address(0U,0U),&old) == TSD_OK);
    CHECK(s.used == used);
    CHECK(TSDMAP(&s,address(0U,0x200000U),address(0U,0xb000U)) == TSD_OK);
    CHECK(s.used == used);
    CHECK(TSDLOOKUP(&s,address(0U,0x100000U),&old) == TSD_OK);
    CHECK(old.lo == 0xa000U);

    /* A plan can reserve storage and still run out: preserve the whole pool. */
    CHECK(TSDUNMAP(&s,address(0U,0x200000U),&old) == TSD_OK);
    s.capacity = s.used + 32768U;
    memcpy(snapshot,pool,s.capacity);
    used = s.used;
    CHECK(TSDMAP(&s,address(0x00200000U,0U),address(0U,0xd000U))
          == TSD_FULL);
    CHECK(s.used == used && memcmp(snapshot,pool,s.capacity) == 0);
    CHECK(TSDLOOKUP(&s,address(0U,0x100000U),&old) == TSD_OK);
    CHECK(old.lo == 0xa000U);
    CHECK(TSDUNMAP(&s,address(0U,0x100000U),&old) == TSD_OK);
    CHECK(s.used == 16384U);
    order.state = &s; order.calls = order.failures = order.unmapping = 0U;
    CHECK(TSDLIVE(&s,purge_order,&order) == TSD_OK);
    for (i = 0U; i < 100U; ++i) {
        transient = address(i << 21,0x20000U);
        order.va = transient; order.unmapping = 0U;
        CHECK(TSDMAP(&s,transient,address(0U,0x9000U)) == TSD_OK);
        order.unmapping = 1U;
        CHECK(TSDUNMAP(&s,transient,&old) == TSD_OK);
        CHECK(s.used == 16384U);
    }
    CHECK(order.calls == 200U && order.failures == 0U);
    free(snapshot); free(pool);
    return 0;
}

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
    CHECK(TSDLOOKUP(&u,address(0,0x00200000U),&previous) == TSD_MISSING);
    CHECK(TSDUNMAP(&u,address(0,0x00200000U),&previous) == TSD_MISSING);
    CHECK(TSDLOOKUP(&u,address(0,0x02000000U),&previous) == TSD_OK);
    CHECK(previous.hi == 0U && previous.lo == 0xd000U);
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
    CHECK(recycling() == 0);
    puts("full-width sparse DAT: separate spaces, live purge, table recycling and atomic mapping pass");
    return 0;
}
