/* SPDX-License-Identifier: MIT; reusable U31 presentation boundary. */
#ifndef PDOS_TWO_SPACE_UI_H
#define PDOS_TWO_SPACE_UI_H
#include "twospace_abi.h"
unsigned int TUIIO(unsigned int bytes,void *request,unsigned int operation);
unsigned int TUICAPS(unsigned char cap[TSA_CAP_BYTES]);
unsigned int TUIOPEN(const unsigned char *title,unsigned int length);
unsigned int TUILINE(const unsigned char *text,unsigned int length);
unsigned int TUIREAD(unsigned char *text,unsigned int capacity,unsigned int *length);
unsigned int TUIMON(unsigned char status[TSA_MONITOR_BYTES]);
unsigned int TUIHAND(unsigned int source);
unsigned int TUICLOSE(void);
#endif
