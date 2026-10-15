/* SPDX-License-Identifier: MIT; common reader/console batch controller in U. */
#ifndef PDOS_SUBMIT_H
#define PDOS_SUBMIT_H
#include "twospace_jcl.h"
typedef struct {
 void *context;
 int (*load)(void *,const char *,unsigned char *,unsigned int,unsigned int *);
 unsigned int (*service)(void *,unsigned int,unsigned int,unsigned char *,void *);
 int (*input)(void *,unsigned char *,unsigned int,unsigned int *);
 void (*line)(void *,const char *);
 void (*record)(void *,const unsigned char *,unsigned int);
} TJFOPS;
typedef struct {
 TJCWORK work;
 TJCRESULT result;
 unsigned char deck[TJC_CARDS*80U],procedure[TJC_CARDS*80U],output[TJB_PRINT_BYTES];
 unsigned char header[128],meta[TJB_META_BYTES];
 char loadlib[45],proclib[45],prefix[45],volume[7],mapping[6][9];
 TJFOPS ops;
} TJFSTATE;
int TJFKNOW(const char *);
int TJFCMD(TJFSTATE *,const TJFOPS *,const char *);
int TJFENTRY(const char *);
#endif
