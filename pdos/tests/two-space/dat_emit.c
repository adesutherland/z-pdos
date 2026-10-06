/* Binary fixture adapter: put product-built K/U DAT tables into a flat core. */
#include <stdio.h>
#include <stdlib.h>
#include "twospace_fixture.h"
#include "twospace_placement.h"

#define CORE_SIZE TSF_CORE_BYTES
static unsigned char *core;

static TSPADDR address(unsigned int hi, unsigned int lo)
{ TSPADDR a; a.hi = hi; a.lo = lo; return a; }

int main(int argc, char **argv)
{
    FILE *f;
    TSFRESULT dat;
    TSPMAP placements;
    if (argc != 2) return 2;
    core = (unsigned char *)malloc(CORE_SIZE);
    if (!core) return 2;
    f = fopen(argv[1], "rb");
    if (!f || fread(core,1,CORE_SIZE,f) != CORE_SIZE ||
        fgetc(f) != EOF || fclose(f) != 0) return 2;
    TSPINIT(&placements);
    if (TSPRESV(&placements,1U,24U,address(0U,0x20000U),8192U) != TSP_OK ||
        TSPRESV(&placements,2U,31U,address(0U,0x02000000U),4096U) != TSP_OK ||
        TSPRESV(&placements,3U,64U,address(1U,0x10000000U),8192U) != TSP_OK ||
        TSPRESV(&placements,4U,24U,address(0U,0x20000U),4096U) != TSP_COLLISION)
        return 2;
    if (TSFBUILD(core,0x100000U,0x180000U,&dat,0,0)) return 2;
    f = fopen(argv[1], "wb");
    if (!f || fwrite(core,1,CORE_SIZE,f) != CORE_SIZE || fclose(f) != 0)
        return 2;
    printf("%u %u %u %u\n",dat.kasce,dat.kbytes,
           dat.uasce,dat.ubytes);
    free(core);
    return 0;
}
