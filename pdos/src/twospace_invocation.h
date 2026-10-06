/* SPDX-License-Identifier: MIT
 * K-owned, single-CPU invocation and resource lifetime ledger.
 * A token is never reused during one IPL, so a late I/O completion cannot
 * attach itself to a later invocation occupying the same stack slot. The K
 * dispatcher serializes all ledger mutations and delivers queued interrupt
 * completions only at a safe point; this C module is not reentrant.
 */
#ifndef PDOS_TWO_SPACE_INVOCATION_H
#define PDOS_TWO_SPACE_INVOCATION_H

#define TSV_MAX_DEPTH 8U
#define TSV_MAX_RESOURCES 24U

#define TSV_OK 0
#define TSV_BAD -1
#define TSV_FULL -2
#define TSV_STALE -3
#define TSV_BUSY -4

#define TSV_CMS 1U
#define TSV_TSO 2U
#define TSV_ACTIVE 1U
#define TSV_SUSPENDED 2U
#define TSV_REAPING 3U

#define TSV_IMAGE 1U
#define TSV_ALLOCATION 2U
#define TSV_FILE 3U
#define TSV_LOWCORE 4U
#define TSV_TERMINAL 5U
#define TSV_IO 6U
#define TSV_IO_PENDING 0U
#define TSV_IO_COMPLETE 1U

typedef struct {
    unsigned int hi, lo;
} TSVWORD64;

typedef struct {
    TSVWORD64 gpr[16];
    TSVWORD64 psw;
    TSVWORD64 asce;
    unsigned int key;
} TSVCONTEXT;

typedef struct {
    unsigned int kind;
    unsigned int handle;
    unsigned int state;
} TSVRESOURCE;

typedef struct {
    unsigned int token;
    unsigned int parent;
    unsigned int personality;
    unsigned int amode;
    unsigned int state;
    unsigned int image_owner;
    unsigned int runtime_owner;
    TSVCONTEXT caller;
    TSVRESOURCE resource[TSV_MAX_RESOURCES];
    unsigned int resources;
} TSVFRAME;

typedef struct {
    TSVFRAME frame[TSV_MAX_DEPTH];
    unsigned int depth;
    unsigned int next_token;
} TSVSTACK;

typedef int (*TSVCLEAN)(unsigned int token, const TSVRESOURCE *resource,
                        void *context);

void TSVINIT(TSVSTACK *stack);
int TSVBEGIN(TSVSTACK *stack, unsigned int personality, unsigned int amode,
             unsigned int image_owner, unsigned int runtime_owner,
             const TSVCONTEXT *caller, unsigned int *token);
const TSVFRAME *TSVTOP(const TSVSTACK *stack);
int TSVOWN(TSVSTACK *stack, unsigned int token, unsigned int kind,
           unsigned int handle);
int TSVFORGET(TSVSTACK *stack, unsigned int token, unsigned int kind,
              unsigned int handle);
int TSVCOMPLETE(TSVSTACK *stack, unsigned int token, unsigned int handle);
/* Cleanup runs in reverse acquisition order. An unsuccessful callback leaves
 * this frame in REAPING state; callers retry after a completion event. The
 * parent cannot resume until every owned resource has been released. A
 * pending I/O resource is never removed before its completion/cancel event. */
int TSVEND(TSVSTACK *stack, unsigned int token, TSVCLEAN clean,
           void *context);

#endif
