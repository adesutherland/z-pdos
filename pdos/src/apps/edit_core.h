/* SPDX-License-Identifier: MIT; original portable C90 line editor model. */
#ifndef PDOS_EDIT_CORE_H
#define PDOS_EDIT_CORE_H
#define EDC_LINES 4096U
#define EDC_WIDTH 256U
typedef struct {unsigned int length;unsigned char text[EDC_WIDTH];} EDCLINE;
typedef struct {unsigned int count,current;EDCLINE line[EDC_LINES];} EDCSTATE;
unsigned int EDCLOAD(EDCSTATE *,const unsigned char *,unsigned int);
unsigned int EDCPACK(const EDCSTATE *,unsigned char *,unsigned int,unsigned int,unsigned int,unsigned int *);
unsigned int EDCINSERT(EDCSTATE *,unsigned int,const unsigned char *,unsigned int);
unsigned int EDCDELETE(EDCSTATE *,unsigned int,unsigned int);
unsigned int EDCCHANGE(EDCSTATE *,unsigned int,const unsigned char *,unsigned int,const unsigned char *,unsigned int);
unsigned int EDCFIND(const EDCLINE *,const unsigned char *,unsigned int,unsigned int *);
int EDCEQUAL(const EDCSTATE *,const EDCSTATE *);
#endif
