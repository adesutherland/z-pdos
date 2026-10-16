/* SPDX-License-Identifier: MIT; one controller for screen and line EDIT. */
#ifndef PDOS_EDIT_SESSION_H
#define PDOS_EDIT_SESSION_H
#include "edit_core.h"
#include "twospace_editfile.h"
#define TED_LEGACY 8U
typedef unsigned int (*TEDFILE)(unsigned int,const char *,unsigned char *,unsigned int,unsigned char *);
typedef struct {
 EDCSTATE *text,*undo,*candidate,*saved;
 unsigned char *packed,info[TEF_BYTES];
 char source[68],notice[160];
 unsigned int has_undo,input_mode,input_undo,top,left,rows,help,first,last;
 TEDFILE file;
} TEDSTATE;
unsigned int TEDWORD(const unsigned char *);
int TEDINIT(TEDSTATE *,TEDFILE);
void TEDFREE(TEDSTATE *);
int TEDOPEN(TEDSTATE *,const char *);
int TEDDIRTY(const TEDSTATE *);
void TEDTAKE(TEDSTATE *);
/* Command: 0 continue, 1 exit, 2 PRINT requested, 3 HELP; errors in notice. */
int TEDCMD(TEDSTATE *,char *);
int TEDSAVE(TEDSTATE *,const char *);
#endif
