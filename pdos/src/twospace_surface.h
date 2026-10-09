/* SPDX-License-Identifier: MIT; supplied-storage cell compositor and encoder. */
#ifndef PDOS_TWO_SPACE_SURFACE_H
#define PDOS_TWO_SPACE_SURFACE_H
#include "twospace_3270.h"
#define TWF_MAX_CELLS 16384U
#define TWF_DATA 1U
#define TWF_ATTRIBUTE 2U
#define TWF_INPUT 3U
#define TWF_NORMAL 0U
#define TWF_HEADER 1U
#define TWF_SHELL 2U
#define TWF_ACTION 3U
#define TWF_ERROR 4U
#define TWF_FOCUS 5U
typedef struct {
    T27GEOMETRY geometry;
    unsigned char *text,*kind,*role;
    unsigned int input,capacity,cursor,cursor_valid;
} TWFSURFACE;
typedef struct {
    unsigned int valid,highlighting;
    unsigned char colour[16];
} TWFPALETTE;
int TWFINIT(TWFSURFACE *,const T27GEOMETRY *,unsigned char *,unsigned char *,unsigned char *);
int TWFINPUT(TWFSURFACE *,unsigned int,unsigned int);
int TWFTEXT(TWFSURFACE *,unsigned int,unsigned int,unsigned int,const unsigned char *,unsigned int,unsigned int);
int TWFRECT(TWFSURFACE *,unsigned int,unsigned int,unsigned int,unsigned int,unsigned int);
/* Full preflight precedes output publication. Delta never touches input or
 * inserts a cursor, clears a screen, resets MDT or restores the keyboard. */
int TWFENCODE(const TWFSURFACE *,const TWFSURFACE *,unsigned int,const TWFPALETTE *,unsigned char *,unsigned int,unsigned int *);
int TWFCOMMIT(TWFSURFACE *,const TWFSURFACE *);
#endif
