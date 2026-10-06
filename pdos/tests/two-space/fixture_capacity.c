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
    if (!core || TSFBUILD(core,TSF_KPOOL_REAL,TSF_UPOOL_REAL,&built,0,0) ||
        TSDATTACH(&u,core+TSF_UPOOL_REAL,TSF_UPOOL_REAL,TSF_UPOOL_BYTES,
                  built.ubytes,built.uasce) != TSD_OK) return 1;
    va.hi=pa.hi=0U;
    for (i=0U; i<16384U; ++i) {
        va.lo=0x04000000U+i*4096U;
        pa.lo=0x400000U+i*4096U;
        if (TSDMAP(&u,va,pa) != TSD_OK) return 2;
    }
    va.hi=1U;
    for (i=0U; i<32768U; ++i) {
        va.lo=0x20000000U+i*4096U;
        pa.lo=0x5000000U+i*4096U;
        if (TSDMAP(&u,va,pa) != TSD_OK) return 3;
    }
    va.lo=0x27fff000U;
    if (TSDLOOKUP(&u,va,&found) != TSD_OK ||
        found.hi || found.lo != 0xcfff000U ||
        u.used > TSF_UPOOL_BYTES) return 4;
    printf("simultaneous 64 MiB U31 and 128 MiB U64 DAT: %u of %u bytes\n",
           u.used,TSF_UPOOL_BYTES);
    free(core);
    return 0;
}
