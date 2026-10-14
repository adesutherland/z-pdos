/* SPDX-License-Identifier: MIT; supplied-storage job temporary data. */
#ifndef PDOS_TWO_SPACE_JOBTEMP_H
#define PDOS_TWO_SPACE_JOBTEMP_H
#include "twospace_jobdd.h"
#define TJT_SLOTS 16U
typedef struct {
    unsigned int live,ready,format,lrecl,block,limit,bytes,writer;
    unsigned char name[8];
    unsigned char *data,*pending;
} TJTSLOT;
typedef struct {
    int (*acquire)(void *,unsigned int,unsigned int,unsigned char **);
    int (*release)(void *,unsigned int,unsigned char *);
    void *context;
} TJTOPS;
typedef struct {TJTSLOT slot[TJT_SLOTS];TJTOPS ops;} TJTPOOL;
void TJTINIT(TJTPOOL *,const TJTOPS *);
TJTSLOT *TJTFIND(TJTPOOL *,const unsigned char *);
int TJTCLAIM(TJTPOOL *,const unsigned char *entry,TJTSLOT **);
/* A failed write never replaces earlier committed data. A failed release
 * retains both old and candidate buffers, owned until successful cleanup. */
int TJTSTORE(TJTPOOL *,TJTSLOT *,const unsigned char *,unsigned int);
int TJTDROP(TJTPOOL *,TJTSLOT *);
int TJTEND(TJTPOOL *);
#endif
