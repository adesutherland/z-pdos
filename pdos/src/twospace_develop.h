/* SPDX-License-Identifier: MIT; shared installed-program workflow. */
#ifndef PDOS_DEVELOP_H
#define PDOS_DEVELOP_H
#include "twospace_submit.h"
#include "twospace_editfile.h"
#define TDV_PROGRAMS 2U
#define TDV_REGISTRY 1024U
#define TDV_HEADER 178U
#define TDV_ENTRY 210U
#define TDV_PAIR 96U
typedef struct {
 TJFOPS batch;
 unsigned int (*file)(void *,unsigned int,const char *,unsigned char *,unsigned int,unsigned char *);
 int (*edit)(void *,const char *,unsigned int);
 unsigned int (*present)(void *,const unsigned char *,unsigned int);
} TDVOPS;
typedef struct {
 TJFSTATE *jobs;TDVOPS ops;
 char source[45],output[45],work[45],registry[45],member[9],arguments[61],notice[160];
 unsigned char profile[32],catalog[TDV_REGISTRY],catalog_info[TEF_BYTES];
 unsigned int catalog_bytes,output_bytes,count,selected,reviewed,kind,ready;
} TDVSTATE;
int TDVCONFIG(TDVSTATE *,TJFSTATE *,const TDVOPS *);
int TDVKNOW(const char *);
int TDVCMD(TDVSTATE *,const char *);
int TDVFORM(TDVSTATE *);
#endif
