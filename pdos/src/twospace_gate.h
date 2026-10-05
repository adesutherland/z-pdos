/* SPDX-License-Identifier: MIT
 * Bounded, full-width U transfer into K-owned C31 storage. No U virtual
 * address is ever converted to a C pointer. One CPU holds the U table map
 * stable for the duration of each preflight and transfer.
 */
#ifndef PDOS_TWO_SPACE_GATE_H
#define PDOS_TWO_SPACE_GATE_H

#include "twospace_dat.h"

#define TSG_MAX_COPY 256U
#define TSG_READ 1U
#define TSG_WRITE 2U
#define TSG_OK 0
#define TSG_BAD -1
#define TSG_UNMAPPED -2
#define TSG_DENIED -3

typedef unsigned int (*TSGRIGHTS)(unsigned int real_page, void *context);

typedef struct {
    TSPADDR address;
    unsigned int length;
    unsigned int direction;
    unsigned int svc;
} TSGREQUEST;

typedef struct {
    const TSDSTATE *u_tables; /* K-only table alias */
    unsigned char *real_aperture; /* K-only aperture, never a U pointer */
    unsigned int real_bytes;
    TSGRIGHTS rights;
    void *rights_context;
} TSGCONTEXT;

int TSGCOPY(const TSGCONTEXT *gate, const TSGREQUEST *request,
            unsigned char *kernel_buffer, unsigned int capacity);

#endif
