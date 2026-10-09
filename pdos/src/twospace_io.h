/* SPDX-License-Identifier: MIT
 * Supplied-storage operation ownership for every K channel provider.
 * One CPU serializes C; only accepted submission and exact status complete
 * a request. An interruption is a notification, never success by itself.
 */
#ifndef PDOS_TWO_SPACE_IO_H
#define PDOS_TWO_SPACE_IO_H
#include "twospace_channel.h"
#define TIO_OK 0
#define TIO_PENDING 1
#define TIO_DEFER 2
#define TIO_BAD -1
#define TIO_BUSY -2
#define TIO_ERROR -3
#define TIO_STALE -4
#define TIO_IDLE 0U
#define TIO_PREPARED 1U
#define TIO_ACTIVE 2U
#define TIO_DONE 3U
#define TIO_QUARANTINED 4U
#define TIO_SLOTS 8U
typedef struct {
    TSCSTATE *channel;
    unsigned int ssid,owner,ticket,phase,end_ccw,count,status_valid;
    unsigned char status[64];
    unsigned int notices;
    unsigned char notice[64];
} TIOREQUEST;
typedef struct {
    unsigned int next_ticket;
    TIOREQUEST request[TIO_SLOTS];
} TIOPOOL;
typedef struct {
    int (*start)(unsigned int,unsigned char *,unsigned char *,void *);
    int (*poll)(unsigned int,unsigned char *,void *);
    int (*wait)(unsigned int,unsigned char *,unsigned int,void *);
    int (*clear)(unsigned int,unsigned char *,void *);
    /* A raw unsolicited notification is distinct from request completion.
     * TIO_DEFER lets a prompt owner preserve input already submitted. */
    int (*notice)(TSCSTATE *,unsigned int,const unsigned char *,void *);
    void *context;
} TIOOPS;
void TIOINIT(TIOPOOL *);
int TIOBIND(TIOPOOL *,TSCSTATE *);
int TIOREADY(const TIOPOOL *,const TSCSTATE *);
int TIOTAKE(TIOPOOL *,TSCSTATE *,unsigned int,unsigned int,unsigned int *);
int TIOSUB(TIOPOOL *,TSCSTATE *,unsigned int,unsigned int,const TIOOPS *);
int TIOPOLL(TIOPOOL *,TSCSTATE *,unsigned int,unsigned int,const TIOOPS *);
int TIOCANCEL(TIOPOOL *,TSCSTATE *,unsigned int,unsigned int,const TIOOPS *);
int TIOQUIESCE(TIOPOOL *,TSCSTATE *,unsigned int,unsigned int,const TIOOPS *);
int TIOFREE(TIOPOOL *,TSCSTATE *,unsigned int,unsigned int);
int TIOEXEC(TIOPOOL *,TSCSTATE *,unsigned int,unsigned int,const TIOOPS *);
const TIOREQUEST *TIOSTATE(const TIOPOOL *,const TSCSTATE *);
#endif
