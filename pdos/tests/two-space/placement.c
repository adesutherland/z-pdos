/* Focused host controls for the Classic-C31-compatible shared-U ledger. */
#include <stdio.h>
#include "twospace_placement.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"placement:%d: %s\n",__LINE__,#x); return 1; } } while (0)
static TSPADDR addr(unsigned int hi, unsigned int lo)
{ TSPADDR a; a.hi = hi; a.lo = lo; return a; }

int main(void)
{
    TSPMAP map;
    TSPADDR found;
    unsigned int i;
    TSPINIT(&map);
    CHECK(TSPRESV(&map, 1, 24, addr(0,0), 4096) == TSP_OK);
    CHECK(TSPRESV(&map, 2, 24, addr(0,0x20000), 0x1a0000) == TSP_OK);
    CHECK(TSPRESV(&map, 3, 31, addr(0,0x02000000), 4096) == TSP_OK);
    CHECK(TSPRESV(&map, 4, 64, addr(1,0x10000000), 8192) == TSP_OK);
    CHECK(map.used == 4);
    CHECK(TSPRESV(&map, 7, 64, addr(0,0xfffff000U), 8192) == TSP_OK);
    CHECK(map.entry[4].last.hi == 1 && map.entry[4].last.lo == 0xfffU);
    CHECK(TSPRESV(&map, 5, 24, addr(0,0x20000), 4096) == TSP_COLLISION);
    CHECK(TSPRESV(&map, 5, 64, addr(0,0x02000000), 4096) == TSP_COLLISION);
    CHECK(TSPRESV(&map, 5, 24, addr(0,0x00fff000), 8192) == TSP_BAD);
    CHECK(TSPRESV(&map, 5, 31, addr(0,0x7ffff000), 8192) == TSP_BAD);
    CHECK(TSPRESV(&map, 5, 64, addr(0xffffffffU,0xfffff000U), 8192) == TSP_BAD);
    CHECK(TSPRESV(&map, 5, 64, addr(0,0x21001), 4096) == TSP_BAD);
    CHECK(TSPFIND(&map, 24, addr(0,0x20000), addr(0,0xffffff),
                  8192, &found) == TSP_OK);
    CHECK(found.hi == 0 && found.lo == 0x1c0000);
    CHECK(TSPRESV(&map, 6, 24, found, 8192) == TSP_OK);
    CHECK(TSPRELS(&map, 2) == TSP_OK);
    CHECK(TSPRELS(&map, 2) == TSP_ABSENT);
    CHECK(TSPRESV(&map, 5, 24, addr(0,0x20000), 4096) == TSP_OK);
    CHECK(TSPFIND(&map, 64, addr(0xffffffffU,0xfffff000U),
                  addr(0xffffffffU,0xffffffffU), 8192, &found) == TSP_FULL);
    TSPINIT(&map);
    for (i = 0; i < TSP_SLOTS; ++i)
        CHECK(TSPRESV(&map, i+1, 64, addr(0,i*4096), 4096) == TSP_OK);
    CHECK(TSPRESV(&map, 99, 64, addr(1,0), 4096) == TSP_FULL);
    puts("shared-U placement: fixed collision, relocation, AMODE bounds, high address and capacity pass");
    return 0;
}
