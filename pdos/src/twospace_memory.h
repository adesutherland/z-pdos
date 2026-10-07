/* SPDX-License-Identifier: MIT
 * K-owned shared-U page allocation. All returned addresses are U virtual;
 * only the K real aperture and K table alias are dereferenced by C31.
 * The caller serializes operations on the single supported CPU.
 */
#ifndef PDOS_TWO_SPACE_MEMORY_H
#define PDOS_TWO_SPACE_MEMORY_H

#include "twospace_dat.h"
#include "twospace_real.h"

#define TSM_OK 0
#define TSM_BAD -1
#define TSM_NOMEM -2
#define TSM_COLLISION -3
#define TSM_ABSENT -4
#define TSM_CORRUPT -5
#define TSM_ALLOCS 128U
#define TSM_OVERLAYS 4U
typedef void (*TSMKEY)(unsigned int real_page, unsigned int key,
                       void *context);

typedef struct {
    unsigned int handle;
    unsigned int task;
    unsigned int mode;
    unsigned int bytes;
    unsigned int real;
    TSPADDR address;
} TSMALLOCATION;

typedef struct {
    unsigned int handle;
    unsigned int task;
    unsigned int parent_real;
    unsigned int child_real;
    unsigned int bytes;
    TSPADDR address;
} TSMOVERLAY;

typedef struct {
    TSDSTATE *u_tables;
    unsigned char *real_aperture;
    unsigned int real_bytes;
    unsigned int first_free_real;
    unsigned int next_handle;
    TSMKEY set_key;
    void *key_context;
    TSRPLAN real;
    TSPMAP virtuals;
    TSMALLOCATION allocations[TSM_ALLOCS];
    unsigned int depth;
    TSMOVERLAY overlay[TSM_OVERLAYS];
} TSMSTATE;

int TSMINIT(TSMSTATE *state, TSDSTATE *u_tables,
            unsigned char *real_aperture, unsigned int real_bytes,
            unsigned int first_free_real, TSMKEY set_key,
            void *key_context);
/* Reserve already mapped U pages before accepting allocations. The real
 * backing must lie in the boot-reserved interval, or be reserved separately.
 */
int TSMRESERVE(TSMSTATE *state, unsigned int owner, unsigned int mode,
               TSPADDR address, unsigned int bytes);
int TSMALLOC(TSMSTATE *state, unsigned int task, unsigned int mode,
             TSPADDR minimum, TSPADDR maximum, unsigned int bytes,
             int fixed, TSPADDR *address);
int TSMFREE(TSMSTATE *state, unsigned int task, TSPADDR address);
/* A fixed-origin child may temporarily occupy precisely the caller's mapped
 * interval. The caller's backing survives outside U; the caller must resume
 * only after POP has restored every PTE. This does not dispatch an app.
 */
int TSMOVERLAYPUSH(TSMSTATE *state, unsigned int task, TSPADDR address,
                   const unsigned char *image, unsigned int image_bytes);
int TSMOVERLAYPOP(TSMSTATE *state, unsigned int task, TSPADDR address,
                  unsigned int child_rc, unsigned int *returned_rc);
unsigned int TSMRIGHTS(unsigned int real_page, void *context);
unsigned int TSMLOWFREE(const TSMSTATE *state);

#endif
