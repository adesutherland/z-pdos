/* SPDX-License-Identifier: MIT
 * Keep enough U DAT capacity for simultaneous wide application heaps.
 */
#include <stdio.h>
#include <stdlib.h>
#include "twospace_fixture.h"

int main(void)
{
    unsigned char *core;
    TSDSTATE u;
    TSFRESULT built;
    TSPADDR va, pa, found;
    unsigned int i;
    core=(unsigned char *)calloc(TSF_CORE_BYTES,1U);
    if (!core || TSFBUILD(core,0x100000U,0x180000U,&built,0,0) ||
        TSDATTACH(&u,core+0x180000U,0x180000U,TSF_UPOOL_BYTES,
                  built.ubytes,built.uasce) != TSD_OK) return 1;
    va.hi=pa.hi=0U;
    for (i=0U; i<4096U; ++i) {
        va.lo=0x04000000U+i*4096U;
        pa.lo=0x200000U+i*4096U;
        if (TSDMAP(&u,va,pa) != TSD_OK) return 2;
    }
    va.hi=1U;
    for (i=0U; i<8192U; ++i) {
        va.lo=0x20000000U+i*4096U;
        pa.lo=0x1200000U+i*4096U;
        if (TSDMAP(&u,va,pa) != TSD_OK) return 3;
    }
    va.lo=0x21fff000U;
    if (TSDLOOKUP(&u,va,&found) != TSD_OK ||
        found.hi || found.lo != 0x31ff000U ||
        u.used > TSF_UPOOL_BYTES) return 4;
    printf("simultaneous 16 MiB U31 and 32 MiB U64 DAT: %u of %u bytes\n",
           u.used,TSF_UPOOL_BYTES);
    free(core);
    return 0;
}
