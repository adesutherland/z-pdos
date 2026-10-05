/* Binary fixture adapter: put product-built K/U DAT tables into a flat core. */
#include <stdio.h>
#include <stdlib.h>
#include "twospace_dat.h"

#define CORE_SIZE 0x200000U
#define POOL_SIZE 0x40000U
static unsigned char *core;

static TSPADDR address(unsigned int hi, unsigned int lo)
{ TSPADDR a; a.hi = hi; a.lo = lo; return a; }

static int map(TSDSTATE *s, unsigned int vhi, unsigned int vlo,
               unsigned int rlo)
{ return TSDMAP(s, address(vhi,vlo), address(0U,rlo)) == TSD_OK; }

static void store32(unsigned char *p, unsigned int x)
{
    p[0]=(unsigned char)(x>>24); p[1]=(unsigned char)(x>>16);
    p[2]=(unsigned char)(x>>8); p[3]=(unsigned char)x;
}

int main(int argc, char **argv)
{
    FILE *f;
    TSDSTATE k, u;
    TSPMAP placements;
    if (argc != 2) return 2;
    core = (unsigned char *)malloc(CORE_SIZE);
    if (!core) return 2;
    f = fopen(argv[1], "rb");
    if (!f || fread(core,1,CORE_SIZE,f) != CORE_SIZE ||
        fgetc(f) != EOF || fclose(f) != 0) return 2;
    if (TSDINIT(&k, core+0x100000U,0x100000U,POOL_SIZE) != TSD_OK ||
        TSDINIT(&u, core+0x140000U,0x140000U,POOL_SIZE) != TSD_OK)
        return 2;
    TSPINIT(&placements);
    if (TSPRESV(&placements,1U,24U,address(0U,0x20000U),8192U) != TSP_OK ||
        TSPRESV(&placements,2U,31U,address(0U,0x02000000U),4096U) != TSP_OK ||
        TSPRESV(&placements,3U,64U,address(1U,0x10000000U),8192U) != TSP_OK ||
        TSPRESV(&placements,4U,24U,address(0U,0x20000U),4096U) != TSP_COLLISION)
        return 2;
    if (!map(&k,0,0,0) || !map(&k,0,0x1000U,0x1000U) ||
        !map(&k,0,0x2000U,0x2000U) || !map(&k,0,0x3000U,0x3000U) ||
        !map(&k,0,0x4000U,0x4000U) ||
        !map(&k,0,0x02000000U,0xa000U) ||
        !map(&k,0,0x02001000U,0xb000U) ||
        !map(&k,0,0x03000000U,0xc000U) ||
        !map(&k,0,0x04000000U,0x7000U) ||
        !map(&k,0,0x04001000U,0xe000U) ||
        !map(&k,0x01000000U,0,0x9000U) ||
        !map(&u,0,0x20000U,0x5000U) ||
        !map(&u,0,0x21000U,0x7000U) ||
        !map(&u,0,0x02000000U,0xd000U) ||
        !map(&u,1,0x10000000U,0x6000U) ||
        !map(&u,1,0x10001000U,0xe000U)) return 2;
    store32(core+0x4000U,0U); store32(core+0x4004U,k.asce_lo);
    store32(core+0x4008U,0U); store32(core+0x400cU,u.asce_lo);
    f = fopen(argv[1], "wb");
    if (!f || fwrite(core,1,CORE_SIZE,f) != CORE_SIZE || fclose(f) != 0)
        return 2;
    printf("%u %u %u %u\n",k.asce_lo,k.used,u.asce_lo,u.used);
    free(core);
    return 0;
}
