/* SPDX-License-Identifier: MIT
 * K-owned, low-real channel program for a bounded 3390 record read.
 * The aperture is a supervisor mapping; no channel address is a U pointer.
 */
#ifndef PDOS_TWO_SPACE_CHANNEL_H
#define PDOS_TWO_SPACE_CHANNEL_H

#define TSC_OK 0
#define TSC_BAD -1
#define TSC_IO -2
#define TSC_REGION_BYTES 0x10000U
#define TSC_MAX_RECORD 32767U
#define TSC_MAX_CONSOLE 16384U
#define TSC_MAX_INPUT 16384U
#define TSC_ORB_OFFSET 0x000U
#define TSC_IRB_OFFSET 0x100U
#define TSC_CCW_OFFSET 0x200U
#define TSC_SEEK_OFFSET 0x300U
#define TSC_SEARCH_OFFSET 0x310U
#define TSC_SCHIB_OFFSET 0x400U
#define TSC_DATA_OFFSET 0x1000U

typedef struct {
    unsigned char *aperture;
    unsigned int real_bytes;
    unsigned int region_real;
} TSCSTATE;

int TSCINIT(TSCSTATE *state, unsigned char *aperture,
            unsigned int real_bytes, unsigned int region_real);
int TSCBUILDREAD(TSCSTATE *state, unsigned int cylinder, unsigned int head,
                 unsigned int record, unsigned int command,
                 unsigned int capacity);
/* Replace a preformatted fixed-length record without changing track layout. */
int TSCBUILDUPDATE(TSCSTATE *state, unsigned int cylinder, unsigned int head,
                   unsigned int record, unsigned int length);
int TSCBUILDBLOCK(TSCSTATE *,unsigned int,unsigned int,unsigned int,
                   unsigned int,unsigned int);
int TSCBUILDTAPE(TSCSTATE *,unsigned int,unsigned int);
int TSCCHECKTAPE(const TSCSTATE *,unsigned int,unsigned int,unsigned int *);
int TSCCHECKREAD(const TSCSTATE *state, unsigned int capacity,
                 unsigned int *transferred);
/* CKD zero-length record: CE/DE/UE, no channel error, full residual. */
int TSCCHECKEND(const TSCSTATE *state, unsigned int capacity);
int TSCBUILDCONSWRITE(TSCSTATE *state, unsigned int length);
int TSCBUILDCONSCMD(TSCSTATE *state,unsigned int command,unsigned int length,
                     unsigned int input);
int TSCCHECKWRITE(const TSCSTATE *state);
int TSCBUILDCONSREAD(TSCSTATE *state, unsigned int capacity);
int TSCCHECKCONSREAD(const TSCSTATE *state, unsigned int capacity,
                     unsigned int *transferred);
unsigned char *TSCDATA(const TSCSTATE *state);
unsigned char *TSCORB(const TSCSTATE *state);
unsigned char *TSCIRB(const TSCSTATE *state);
unsigned char *TSCSCHIB(const TSCSTATE *state);

#endif
