/* SPDX-License-Identifier: MIT; PD-033 bounded 3505/1403 device contract. */
#ifndef PDOS_TWO_SPACE_BATCH_H
#define PDOS_TWO_SPACE_BATCH_H
#include "twospace_channel.h"
#define TBC_OK 0
#define TBC_BAD -1
#define TBC_BUSY -2
#define TBC_IO -3
#define TBC_EOF 1
#define TBC_READER 1U
#define TBC_PRINTER 2U
#define TBC_CARD_BYTES 80U
#define TBC_PRINT_BYTES 132U
#define TBC_SENSE_BYTES 8U
#define TBC_READER_REAL 0x000a0000U
#define TBC_PRINTER_REAL 0x000b0000U
typedef struct {
    unsigned int actual,device_status,channel_status,sense_bytes,eof;
    unsigned char sense[TBC_SENSE_BYTES];
} TBCRESULT;
typedef struct {
    unsigned int owner,handle,kind,address,ssid;
    TBCRESULT result;
} TBCENDPOINT;
typedef struct {
    unsigned int next_handle;
    TBCENDPOINT endpoint[2];
} TBCPOOL;
int TBCBUILD(TSCSTATE *,unsigned int,unsigned int,unsigned int);
int TBCCHECK(const TSCSTATE *,const unsigned char *,unsigned int,
             unsigned int,unsigned int,TBCRESULT *);
int TBCOPEN(TBCPOOL *,unsigned int,unsigned int,unsigned int,unsigned int *);
TBCENDPOINT *TBCFIND(TBCPOOL *,unsigned int,unsigned int);
int TBCCLOSE(TBCPOOL *,unsigned int,unsigned int);
#endif
