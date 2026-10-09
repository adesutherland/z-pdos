/* SPDX-License-Identifier: MIT; bounded semantic history, independent of cells. */
#ifndef PDOS_TWO_SPACE_HISTORY_H
#define PDOS_TWO_SPACE_HISTORY_H
#define TPH_OK 0
#define TPH_BAD -1
#define TPH_EMPTY 1
#define TPH_TEXT 2U
#define TPH_LINE_BYTES 256U
#define TPH_VIEW_ROWS 256U
typedef struct {
    unsigned int sequence_hi,sequence_lo,group,token,type,offset,bytes;
} TPHRECORD;
typedef struct {
    unsigned char *data;unsigned int capacity,used,tail;
    TPHRECORD *record;unsigned int slots,first,count,evicted;
    unsigned int sequence_hi,sequence_lo;
} TPHSTATE;
typedef struct {
    unsigned int group,follow,sequence_hi,sequence_lo,row,evicted;
} TPHVIEW;
typedef struct {unsigned int slot,row;} TPHROW;
int TPHINIT(TPHSTATE *,unsigned char *,unsigned int,TPHRECORD *,unsigned int);
int TPHAPPEND(TPHSTATE *,unsigned int,unsigned int,unsigned int,const unsigned char *,unsigned int);
void TPHFOLLOW(TPHVIEW *,unsigned int);
/* Renders native bytes padded with IBM1047 spaces; does not alter history. */
int TPHRENDER(const TPHSTATE *,TPHVIEW *,unsigned int,unsigned int,unsigned char *,unsigned int,unsigned int *);
/* Latest semantic records, one visibly shortened record per compact row. */
int TPHCOMPACT(const TPHSTATE *,const TPHVIEW *,unsigned int,unsigned int,unsigned char *,unsigned int,unsigned int *);
int TPHPAGE(const TPHSTATE *,TPHVIEW *,unsigned int,unsigned int,int);
#endif
