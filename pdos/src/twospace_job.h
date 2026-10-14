/* SPDX-License-Identifier: MIT; supplied-storage serial job and spool core. */
#ifndef PDOS_TWO_SPACE_JOB_H
#define PDOS_TWO_SPACE_JOB_H
#define TJB_BYTES 262144U
#define TJB_SLOTS 3U
#define TJB_SLOT_BYTES 2048U
#define TJB_PRINT_BYTES 81920U
#define TJB_BASE 32U
#define TJB_OUTPUT_BASE (TJB_BASE+TJB_SLOTS*TJB_SLOT_BYTES)
#define TJB_DD_BASE (TJB_OUTPUT_BASE+TJB_SLOTS*TJB_PRINT_BYTES)
#define TJB_STEPS 8U
#define TJB_STEP_BYTES 160U
#define TJB_DESC_BYTES (32U+TJB_STEPS*TJB_STEP_BYTES)
#define TJB_REQUEST_BYTES 128U
#define TJB_STEP_BASE 128U
#define TJB_FREE 0U
#define TJB_QUEUED 1U
#define TJB_HELD 2U
#define TJB_RUNNING 3U
#define TJB_DONE 4U
#define TJB_FAILED 5U
#define TJB_CANCELLED 6U
#define TJB_INTERRUPTED 7U
#define TJB_PENDING 0U
#define TJB_ACTIVE 1U
#define TJB_NORMAL 2U
#define TJB_SKIP 3U
#define TJB_ERROR 4U
#define TJB_CANCEL 5U
#define TJB_INTERRUPT 6U
#define TJB_PRINT_PENDING 0U
#define TJB_PRINT_ACTIVE 1U
#define TJB_PRINT_DONE 2U
#define TJB_PRINT_FAILED 3U
#define TJB_COND_NE 1U
#define TJB_OK 0
#define TJB_BAD 8
#define TJB_IO 12
#define TJB_BUSY 16
#define TJB_FULL 4
#define TJB_UNSUPPORTED 20
/* Slot fields: id, owner, state, current zero-based step, step count,
 * output quota/used, print state, OS status, RC-valid/RC, cleanup, cancel,
 * output sequence, print attempt count; job name at 64. Steps at 128.
 * Step: name[8], program[8], PARM length/COND, PARM[100], OS status,
 * RC-valid/RC, abnormal kind, cleanup, skip reason, state; reserved[8].
 * Output: sequence, step (1 based), length, command, DD[8], payload.
 * Every persisted integer is big endian, including on an ASCII host. */
typedef struct { unsigned char *bytes; } TJBPOOL;
unsigned int TJBWORD(const unsigned char *p);
void TJBPUT(unsigned char *p,unsigned int value);
int TJBINIT(TJBPOOL *pool,unsigned char *bytes,unsigned int size);
/* Validate an entire persisted envelope before recovery mutates it. */
int TJBLOAD(TJBPOOL *pool,unsigned char *bytes,unsigned int size);
int TJBCHECK(const unsigned char *descriptor,unsigned int size);
unsigned char *TJBFIND(TJBPOOL *pool,unsigned int owner,unsigned int id);
unsigned char *TJBSTEP(unsigned char *job,unsigned int step);
int TJBSUB(TJBPOOL *,unsigned int owner,const unsigned char *,unsigned int,unsigned int *id);
int TJBACT(TJBPOOL *,unsigned int owner,unsigned int id,unsigned int action);
/* action: 1 hold, 2 release, 3 cancel, 4 purge. */
int TJBSTART(TJBPOOL *,unsigned int owner,unsigned int id);
int TJBNEXT(TJBPOOL *,unsigned int owner,unsigned int id);
int TJBEND(TJBPOOL *,unsigned int owner,unsigned int id,unsigned int os,
           unsigned int rc_valid,unsigned int rc,unsigned int abnormal,unsigned int cleanup);
int TJBAPP(TJBPOOL *,unsigned int owner,unsigned int id,const unsigned char dd[8],
           unsigned int command,const unsigned char *data,unsigned int bytes);
unsigned char *TJBOUT(TJBPOOL *,unsigned int id);
/* Optional Stage 3 allocation descriptor in the formerly reserved tail.
 * Version-1 Stage 2 envelopes with a zero tail remain readable. Binding
 * upgrades the envelope to version 2; older kernels must reject it. */
unsigned char *TJBDDS(TJBPOOL *,unsigned int id);
int TJBBIND(TJBPOOL *,unsigned int owner,unsigned int id,const unsigned char *,unsigned int);
#endif
