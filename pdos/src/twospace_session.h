/* SPDX-License-Identifier: MIT; supplied-storage, invocation-owned records. */
#ifndef PDOS_TWO_SPACE_SESSION_H
#define PDOS_TWO_SPACE_SESSION_H
#include "twospace_3270.h"
#define TDS_OK 0
#define TDS_BAD -1
#define TDS_BUSY -2
#define TDS_STALE -3
#define TDS_FULL -4
#define TDS_EMPTY 1
#define TDS_SLOTS 8U
#define TDS_EVENTS 4U
#define TDS_RECORD_BYTES 65535U
#define TDS_TRANSFER_BYTES 131072U
#define TDS_SLOT_BYTES (TDS_EVENTS*TDS_RECORD_BYTES+TDS_TRANSFER_BYTES)
#define TDS_POOL_BYTES (TDS_SLOTS*TDS_SLOT_BYTES)
#define TDS_EVENT_KEY 1U
#define TDS_EVENT_STRUCTURED 2U
#define TDS_EVENT_LINE 3U
#define TDS_EVENT_GEOMETRY 4U
#define TDS_EVENT_INVALID 5U
#define TDS_EVENT_BUFFER 6U
/* A full physical read at the configured limit cannot prove record end. */
#define TDS_EVENT_TRUNCATED 7U
#define TDS_READY 1U
#define TDS_ASSEMBLING 2U
#define TDS_SUBMITTED 3U
#define TDS_QUARANTINED 4U
#define TDS_SEALED 5U
#define TDS_CLOSING 6U
typedef struct {
    unsigned int type,generation,sequence_hi,sequence_lo,bytes;
    unsigned int aid,cursor_valid,cursor,fields;
    const unsigned char *data;
} TDSEVENT;
typedef struct {
    unsigned int handle,owner,address,device_class,generation,phase;
    T27GEOMETRY geometry;
    unsigned int first,count,sequence_hi,sequence_lo;
    TDSEVENT event[TDS_EVENTS];
    unsigned char *records,*transfer;
    unsigned int transfer_bytes,transfer_command,transfer_sequence;
} TDSSESSION;
typedef struct {
    unsigned int next_handle;
    unsigned char *data;
    unsigned int bytes;
    TDSSESSION session[TDS_SLOTS];
} TDSPOOL;
int TDSINIT(TDSPOOL *,unsigned char *,unsigned int);
int TDSOPEN(TDSPOOL *,unsigned int,unsigned int,unsigned int,const T27GEOMETRY *,unsigned int,unsigned int *);
TDSSESSION *TDSFIND(TDSPOOL *,unsigned int,unsigned int);
int TDSCLOSE(TDSPOOL *,unsigned int,unsigned int);
int TDSPOST(TDSSESSION *,unsigned int,const unsigned char *,unsigned int);
int TDSPEEK(const TDSSESSION *,const TDSEVENT **);
int TDSACK(TDSSESSION *,unsigned int,unsigned int);
int TDSGEOM(TDSPOOL *,unsigned int,const T27GEOMETRY *,unsigned int);
/* Contiguous fragment offsets bind one complete binary transfer. */
int TDSFRAGMENT(TDSSESSION *,unsigned int,unsigned int,unsigned int,const unsigned char *,unsigned int,unsigned int);
int TDSSUBMIT(TDSSESSION *);
int TDSFINISH(TDSSESSION *,unsigned int);
#endif
