/* SPDX-License-Identifier: MIT; shared character-cell workspace presentation. */
#ifndef PDOS_TWO_SPACE_WORKSPACE_H
#define PDOS_TWO_SPACE_WORKSPACE_H
#include "twospace_3270.h"
#define TWP_PROGRAMS "Programs"
#define TWP_SOURCE "Source"
#define TWP_BUILD "Build"
#define TWP_RUN "Run"
#define TWP_OUTPUT "Output"
#define TWP_DETAILS "Details"
#define TWP_HELP "Help"
#define TWP_CONTEXT_MAX 68U
/* Complete native IBM1047 row. Coordinates are zero based; body is 2..R-4.
 * Context is native; all labels are C execution text. No terminal ownership. */
int TWPFRAME(const T27GEOMETRY *,unsigned int,const char *,
 const unsigned char *,unsigned int,const char *,const char *,const char *,
 unsigned char *);
/* A protected, keyboard-locked snapshot. job is a native TJB_SLOT_BYTES slot
 * or NULL before acknowledgement. Native DONE still means U is finalising. */
int TWPBUSY(const T27GEOMETRY *,const unsigned char *,unsigned int,
 const char *,const unsigned char *,unsigned char *,unsigned int,unsigned int *);
/* Native job summary body shared by retained Details and the busy frame. */
int TWPJOBROW(const T27GEOMETRY *,unsigned int,const unsigned char *,unsigned char *);
#endif
