/* SPDX-License-Identifier: MIT */
#include "twospace_fixture.h"
#include "twospace_dat.h"

typedef struct { unsigned int vhi, vlo, real; } TSFMAPPING;
static const TSFMAPPING kmaps[] = {
    {0U,0U,0U}, {0U,0x1000U,0x1000U}, {0U,0x2000U,0x2000U},
    {0U,0x3000U,0x3000U}, {0U,0x4000U,0x4000U},
    {0U,TSF_TRAMPOLINE_VA,0xf000U}, {0U,0x03000000U,0x10000U},
    {0x01000000U,0U,0x9000U}
};
static const TSFMAPPING umaps[] = {
    {0U,0x20000U,0x5000U}, {0U,0x21000U,0x7000U},
    {0U,0x02000000U,0x11000U}, {1U,0x10000000U,0x6000U},
    {1U,0x10001000U,0x12000U}, {1U,0x10002000U,0x13000U}
};

static int map_all(TSDSTATE *state, const TSFMAPPING *maps,
                   unsigned int count)
{
    unsigned int i;
    TSPADDR va, pa;
    pa.hi = 0U;
    for (i = 0U; i < count; ++i) {
        va.hi = maps[i].vhi; va.lo = maps[i].vlo;
        pa.lo = maps[i].real;
        if (TSDMAP(state,va,pa) != TSD_OK) return -1;
    }
    return 0;
}

static void put32(unsigned char *p, unsigned int n)
{
    p[0]=(unsigned char)(n>>24); p[1]=(unsigned char)(n>>16);
    p[2]=(unsigned char)(n>>8); p[3]=(unsigned char)n;
}

int TSFBUILD(unsigned char *core, unsigned int kpool, unsigned int upool,
             TSFRESULT *result, TSDPURGE purge, void *context)
{
    unsigned int i;
    TSDSTATE k, u;
    TSPADDR va, pa, old;
    if (!core || !result || (kpool & 4095U) || (upool & 4095U) ||
        kpool < 0x14000U || upool < 0x14000U ||
        kpool > TSF_CORE_BYTES - TSF_KPOOL_BYTES ||
        upool > TSF_CORE_BYTES - TSF_UPOOL_BYTES ||
        (kpool < upool + TSF_UPOOL_BYTES &&
         upool < kpool + TSF_KPOOL_BYTES) ||
        (kpool < TSF_SERVICE_EXT_REAL+TSF_SERVICE_EXT_BYTES &&
         TSF_SERVICE_EXT_REAL < kpool+TSF_KPOOL_BYTES) ||
        (upool < TSF_SERVICE_EXT_REAL+TSF_SERVICE_EXT_BYTES &&
         TSF_SERVICE_EXT_REAL < upool+TSF_UPOOL_BYTES) ||
        (kpool < TSF_SERVICE_MORE_REAL+TSF_SERVICE_MORE_BYTES &&
         TSF_SERVICE_MORE_REAL < kpool+TSF_KPOOL_BYTES) ||
        (upool < TSF_SERVICE_MORE_REAL+TSF_SERVICE_MORE_BYTES &&
         TSF_SERVICE_MORE_REAL < upool+TSF_UPOOL_BYTES) ||
        (kpool < TSF_CHANNEL_REAL+TSF_CHANNEL_BYTES &&
         TSF_CHANNEL_REAL < kpool+TSF_KPOOL_BYTES) ||
        (upool < TSF_CHANNEL_REAL+TSF_CHANNEL_BYTES &&
         TSF_CHANNEL_REAL < upool+TSF_UPOOL_BYTES) ||
        (kpool < TSF_CONSOLE_REAL+TSF_CHANNEL_BYTES &&
         TSF_CONSOLE_REAL < kpool+TSF_KPOOL_BYTES) ||
        (upool < TSF_CONSOLE_REAL+TSF_CHANNEL_BYTES &&
         TSF_CONSOLE_REAL < upool+TSF_UPOOL_BYTES)) return -1;
    for (i = 0U; i < sizeof kmaps / sizeof kmaps[0]; ++i)
        if ((kmaps[i].real >= kpool && kmaps[i].real < kpool + TSF_KPOOL_BYTES) ||
            (kmaps[i].real >= upool && kmaps[i].real < upool + TSF_UPOOL_BYTES))
            return -1;
    for (i = 0U; i < sizeof umaps / sizeof umaps[0]; ++i)
        if ((umaps[i].real >= kpool && umaps[i].real < kpool + TSF_KPOOL_BYTES) ||
            (umaps[i].real >= upool && umaps[i].real < upool + TSF_UPOOL_BYTES))
            return -1;
    if (TSDINIT(&k,core+kpool,kpool,TSF_KPOOL_BYTES) != TSD_OK ||
        TSDINIT(&u,core+upool,upool,TSF_UPOOL_BYTES) != TSD_OK ||
        map_all(&k,kmaps,sizeof kmaps / sizeof kmaps[0]) ||
        map_all(&u,umaps,sizeof umaps / sizeof umaps[0])) return -1;
    /* All final real frames have one supervisor-only C31 aperture. U never
       maps this window; the service gate first proves each U translation. */
    for (i = 0U; i < TSF_REAL_BYTES / 4096U; ++i) {
        va.hi=0U; va.lo=TSF_KAPERTURE_VA+i*4096U;
        pa.hi=0U; pa.lo=i*4096U;
        if ((pa.lo >= kpool && pa.lo < kpool+TSF_KPOOL_BYTES) ||
            (pa.lo >= upool && pa.lo < upool+TSF_UPOOL_BYTES)) {
            if (TSDMAPTABLE(&k,va,pa) != TSD_OK) return -1;
        } else if (TSDMAP(&k,va,pa) != TSD_OK) return -1;
    }
    for (i = 0U; i < TSF_SERVICE_PAGES; ++i) {
        va.hi=0U; va.lo=0x02000000U+i*4096U;
        pa.hi=0U; pa.lo=i<5U ? 0xa000U+i*4096U :
                              i<16U ? TSF_SERVICE_EXT_REAL+(i-5U)*4096U :
                              TSF_SERVICE_MORE_REAL+(i-16U)*4096U;
        if (TSDMAP(&k,va,pa) != TSD_OK) return -1;
    }
    /* K31 can revisit both table pools after the real bootstrap is gone.
       U has no such alias. The ledger has already reserved these frames. */
    for (i = 0U; i < TSF_KPOOL_BYTES / 4096U; ++i) {
        va.hi=0U; pa.hi=0U;
        va.lo=TSF_KPOOL_VA+i*4096U; pa.lo=kpool+i*4096U;
        if (TSDMAPTABLE(&k,va,pa) != TSD_OK) return -1;
    }
    for (i = 0U; i < TSF_UPOOL_BYTES / 4096U; ++i) {
        va.hi=0U; pa.hi=0U;
        va.lo=TSF_UPOOL_VA+i*4096U; pa.lo=upool+i*4096U;
        if (TSDMAPTABLE(&k,va,pa) != TSD_OK) return -1;
    }
    if (purge) {
        va.hi=0U; va.lo=0x20000U; pa.hi=0U; pa.lo=0x5000U;
        if (TSDLIVE(&u,purge,context) != TSD_OK ||
            TSDUNMAP(&u,va,&old) != TSD_OK ||
            old.hi != pa.hi || old.lo != pa.lo ||
            TSDMAP(&u,va,pa) != TSD_OK) return -1;
    }
    put32(core+0x4000U,0U); put32(core+0x4004U,k.asce_lo);
    put32(core+0x4008U,0U); put32(core+0x400cU,u.asce_lo);
    put32(core+0x4094U,k.used); put32(core+0x4098U,u.used);
    result->kasce=k.asce_lo; result->uasce=u.asce_lo;
    result->kbytes=k.used; result->ubytes=u.used;
    return 0;
}
