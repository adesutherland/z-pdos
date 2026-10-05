/* SPDX-License-Identifier: MIT
 * Bounded two-space machine fixture map. The host oracle and guest bootstrap
 * consume the same product source so their DAT contracts cannot drift.
 */
#ifndef PDOS_TWO_SPACE_FIXTURE_H
#define PDOS_TWO_SPACE_FIXTURE_H

#define TSF_CORE_BYTES 0x200000U
#define TSF_POOL_BYTES 0x40000U
#define TSF_REAL_BYTES 0x1000000U
#define TSF_KPOOL_VA 0x05000000U
#define TSF_UPOOL_VA 0x05040000U
#define TSF_KAPERTURE_VA 0x08000000U
#define TSF_SERVICE_PAGES 5U
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
