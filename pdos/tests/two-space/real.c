/* Host controls for bootstrap real-frame lifetime and exhaustion. */
#include <stdio.h>
#include "twospace_real.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"real:%d\n",__LINE__); return 1; } } while (0)
int main(void)
{
    TSRPLAN p;
    unsigned int stage, launch, alternate;
    CHECK(TSRINIT(&p, 0x1000000U) == TSR_OK);
    CHECK(TSRRESERVE(&p,1U,0U,0x400000U,TSR_LOAD) == TSR_OK);
    CHECK(TSRRESERVE(&p,12U,0xf00000U,0x100000U,TSR_LOAD) == TSR_OK);
    CHECK(TSRRESERVE(&p,2U,0U,0x200000U,TSR_COPY|TSR_RUN) == TSR_OK);
    CHECK(TSRALLOC(&p,3U,0x200000U,0x400000U,p.limit,
                   TSR_LOAD|TSR_COPY,&stage) == TSR_OK);
    CHECK(stage == 0x400000U);
    CHECK(TSRALLOC(&p,4U,TSR_PAGE,0x400000U,p.limit,
                   TSR_LOAD|TSR_COPY,&launch) == TSR_OK);
    CHECK(launch == 0x600000U);
    CHECK(TSRRESERVE(&p,5U,stage,TSR_PAGE,TSR_COPY) == TSR_COLLISION);
    CHECK(TSRRELEASE(&p,3U,stage) == TSR_OK);
    CHECK(TSRALLOC(&p,6U,0x200000U,0x400000U,p.limit,
                   TSR_LOAD|TSR_COPY,&alternate) == TSR_OK);
    CHECK(alternate == stage);
    CHECK(TSRRELEASE(&p,3U,stage) == TSR_MISSING);
    CHECK(TSRRESERVE(&p,7U,0xfffff000U,0x2000U,TSR_RUN) == TSR_BAD);
    CHECK(TSRALLOC(&p,8U,0x1000000U,0x400000U,p.limit,
                   TSR_LOAD,&stage) == TSR_FULL);
    CHECK(TSRINIT(&p,0x1000000U) == TSR_OK);
    CHECK(TSRRESERVE(&p,9U,0x401000U,TSR_PAGE,TSR_LOAD|TSR_COPY) == TSR_OK);
    CHECK(TSRALLOC(&p,10U,0x200000U,0x400000U,p.limit,
                   TSR_LOAD|TSR_COPY,&stage) == TSR_OK);
    CHECK(TSRALLOC(&p,11U,TSR_PAGE,0x400000U,p.limit,
                   TSR_LOAD|TSR_COPY,&launch) == TSR_OK);
    CHECK(stage == 0x402000U && launch == 0x400000U);
    puts("real-frame lifetimes, collision, release and exhaustion pass");
    return 0;
}
