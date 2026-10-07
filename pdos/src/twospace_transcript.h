/* SPDX-License-Identifier: MIT; bounded, versioned append-only K transcript. */
#ifndef PDOS_TWO_SPACE_TRANSCRIPT_H
#define PDOS_TWO_SPACE_TRANSCRIPT_H
#include "twospace_abi.h"
typedef struct {
    unsigned char *data;
    unsigned int capacity,used,sequence_hi,sequence_lo,gaps,full;
} TTXSTATE;
int TTXINIT(TTXSTATE *state,unsigned char *data,unsigned int capacity);
int TTXAPPEND(TTXSTATE *state,unsigned int type,unsigned int token,
               unsigned int encoding,unsigned int flags,const unsigned char *payload,
               unsigned int length);
#endif
