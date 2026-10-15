/* SPDX-License-Identifier: MIT; supplied-storage batch form presentation. */
#ifndef PDOS_BATCHUI_H
#define PDOS_BATCHUI_H
#include "twospace_submit.h"
#define TBF_ROWS 32U
#define TBF_WIDTH 132U
#define TBF_SUBMIT 1U
#define TBF_JOBS 2U
#define TBF_OUTPUT 3U
#define TBF_PREVIEW 4U
typedef struct {
 TJFSTATE *front;
 TJFOPS backend,bridge;
 unsigned int initialized,screen,page,column,height,width,total,shown;
 unsigned int selected,ready,ticket,attempted,last_id,hold,runhold,editing;
 unsigned int overrides,rc,exit;
 unsigned int revision;
 char deck[56],proc[9],source[56],out[56],parameters[101];
 char override[8][81],step[19],dd[9],notice[133];
 unsigned char rows[TBF_ROWS][TBF_WIDTH];
} TBFSTATE;
int TBFINIT(TBFSTATE *,TJFSTATE *,const TJFOPS *);
int TBFEVENT(TBFSTATE *,const char *,unsigned int);
int TBFVIEW(TBFSTATE *,unsigned int,unsigned int);
int TBFENTRY(TJFSTATE *,const TJFOPS *,const char *);
#endif
