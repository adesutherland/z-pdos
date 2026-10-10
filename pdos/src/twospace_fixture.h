/* SPDX-License-Identifier: MIT
 * Bounded two-space machine fixture map. The host oracle and guest bootstrap
 * consume the same product source so their DAT contracts cannot drift.
 */
#ifndef PDOS_TWO_SPACE_FIXTURE_H
#define PDOS_TWO_SPACE_FIXTURE_H

#define TSF_CORE_BYTES 0x1000000U
#define TSF_KPOOL_BYTES 0x300000U
#define TSF_UPOOL_BYTES 0x300000U
#define TSF_REAL_BYTES 0x20000000U
#define TSF_KPOOL_REAL 0x800000U
#define TSF_UPOOL_REAL 0xb00000U
#define TSF_KPOOL_VA 0x05000000U
#define TSF_UPOOL_VA 0x05300000U
#define TSF_KAPERTURE_VA 0x08000000U
#define TSF_SERVICE_PAGES 512U
#define TSF_SERVICE_REAL 0x400000U
#define TSF_SERVICE_BYTES 0x200000U
#define TSF_BOOT_LOW_BYTES 0x20000U
#define TSF_KSTACK_PAGES 128U
#define TSF_KSTACK_EXT_REAL 0x600000U
#define TSF_KSTACK_EXT_BYTES 0x7c000U
#define TSF_TRAMPOLINE_VA 0x02800000U
#define TSF_PACKAGE_DATA_RECORDS 256U
#define TSF_PC_REAL 0x90000U
#define TSF_PC_BYTES 0x2000U
#define TSF_CHANNEL_REAL 0x3e0000U
#define TSF_CHANNEL_BYTES 0x10000U
#define TSF_CONSOLE_REAL 0x3f0000U
#define TSF_NORMAL_REAL 0x95020U
#define TSF_NORMAL_MAGIC 0x54534e31U
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
