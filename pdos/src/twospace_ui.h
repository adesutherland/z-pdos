/* SPDX-License-Identifier: MIT; reusable U31 presentation boundary. */
#ifndef PDOS_TWO_SPACE_UI_H
#define PDOS_TWO_SPACE_UI_H
#include "twospace_abi.h"
unsigned int TUIIO(unsigned int bytes,void *request,unsigned int operation);
unsigned int TUICAPS(unsigned char cap[TSA_CAP_BYTES]);
unsigned int TUIOPEN(const unsigned char *title,unsigned int length);
unsigned int TUILINE(const unsigned char *text,unsigned int length);
unsigned int TUIKEY(unsigned char *,unsigned int,unsigned int *,unsigned int *);
unsigned int TUISET(const unsigned char *,unsigned int,unsigned int);
unsigned int TUIJOB(const unsigned char *,unsigned int);
unsigned int TUIEXT(void);
unsigned int TUISHELL(void);
unsigned int TUICTRL(unsigned int);
unsigned int TUIJOIN(unsigned int,unsigned int);
unsigned int TUIEND(unsigned int,unsigned int,unsigned int);
unsigned int TUIVOL(const unsigned char *,unsigned int);
unsigned int TUIPANEL(unsigned int,unsigned char [64]);
unsigned int TUIREAD(unsigned char *text,unsigned int capacity,unsigned int *length);
unsigned int TUIMON(unsigned char status[TSA_MONITOR_BYTES]);
unsigned int TUIHAND(unsigned int source);
unsigned int TUICLOSE(void);
unsigned int TUIRESULT(unsigned char result[TSA_RESULT_BYTES]);
#endif
