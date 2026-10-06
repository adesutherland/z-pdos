/* SPDX-License-Identifier: MIT
 * Bounded two-space machine fixture map. The host oracle and guest bootstrap
 * consume the same product source so their DAT contracts cannot drift.
 */
#ifndef PDOS_TWO_SPACE_FIXTURE_H
#define PDOS_TWO_SPACE_FIXTURE_H

#define TSF_CORE_BYTES 0x400000U
#define TSF_KPOOL_BYTES 0x180000U
#define TSF_UPOOL_BYTES 0x160000U
#define TSF_REAL_BYTES 0x10000000U
#define TSF_KPOOL_REAL 0x100000U
#define TSF_UPOOL_REAL 0x280000U
#define TSF_KPOOL_VA 0x05000000U
#define TSF_UPOOL_VA 0x05180000U
#define TSF_KAPERTURE_VA 0x08000000U
#define TSF_SERVICE_PAGES 32U
#define TSF_SERVICE_EXT_REAL 0x14000U
#define TSF_SERVICE_EXT_BYTES 0xb000U
#define TSF_SERVICE_MORE_REAL 0x80000U
#define TSF_SERVICE_MORE_BYTES 0x10000U
#define TSF_TRAMPOLINE_VA 0x02020000U
#define TSF_PC_REAL 0x90000U
#define TSF_PC_BYTES 0x2000U
#define TSF_CHANNEL_REAL 0x3e0000U
#define TSF_CHANNEL_BYTES 0x10000U
#define TSF_CONSOLE_REAL 0x3f0000U
#include "twospace_dat.h"

typedef struct {
    unsigned int kasce;
    unsigned int uasce;
    unsigned int kbytes;
    unsigned int ubytes;
} TSFRESULT;

int TSFBUILD(unsigned char *core, unsigned int kpool, unsigned int upool,
             TSFRESULT *result, TSDPURGE purge, void *context);

#endif
