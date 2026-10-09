/* SPDX-License-Identifier: MIT; supplied-storage map of the bounded PDOS VTOC. */
#ifndef PDOS_TWO_SPACE_DASD_H
#define PDOS_TWO_SPACE_DASD_H
#define TDA_TRACKS 1500U
#define TDA_RESERVED 1U
#define TDA_ALLOCATED 2U
typedef struct {
    unsigned char *map;
    unsigned int tracks,reserved,allocated,free,largest,datasets;
} TDAMAP;
int TDAINIT(TDAMAP *,unsigned char *,unsigned int,const unsigned char *,unsigned int);
int TDAADD(TDAMAP *,const unsigned char *,unsigned int);
int TDASTAT(TDAMAP *);
#endif
