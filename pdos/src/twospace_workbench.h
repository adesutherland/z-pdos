/* SPDX-License-Identifier: MIT; the selected declarative Workbench layout. */
#ifndef PDOS_TWO_SPACE_WORKBENCH_H
#define PDOS_TWO_SPACE_WORKBENCH_H
#include "twospace_surface.h"
#include "twospace_history.h"
#define TWB_DISPATCH 1U
#define TWB_ACTIVE 2U
#define TWB_DONE 3U
typedef struct {
    unsigned int wide,input,heading,status,keys,position,body,body_rows,body_width;
    unsigned int shell_column,shell_row,shell_rows,shell_width;
} TWBPLAN;
typedef struct {
    const unsigned char *title,*name,*volume,*draft,*input_name;
    unsigned int title_bytes,name_bytes,volume_bytes,draft_bytes,cursor,cursor_valid,input_name_bytes;
    unsigned int job,phase,os,rc_valid,rc,focus,source,input_program,waiting,help,capture_gap,joined;
} TWBCONTEXT;
int TWBLAYOUT(unsigned int,unsigned int,TWBPLAN *);
int TWBCOMPOSE(TWFSURFACE *,const TWBPLAN *,const TWBCONTEXT *,const TPHSTATE *,TPHVIEW *,TPHVIEW *,unsigned char *,unsigned int);
#endif
