/* SPDX-License-Identifier: MIT; supplied-storage map of the supported VTOC. */
#ifndef PDOS_TWO_SPACE_DASD_H
#define PDOS_TWO_SPACE_DASD_H
#define TDA_TRACKS 1500U
#define TDA_HEADS 15U
#define TDA_MAX_CYLINDERS 65535U
#define TDA_MAX_TRACKS (TDA_MAX_CYLINDERS * TDA_HEADS)
#define TDA_GEOMETRY_QUERY 2U
#define TDA_RESERVED 1U
#define TDA_ALLOCATED 2U
typedef struct {
    unsigned char *map;
    unsigned int tracks,reserved,allocated,free,largest,datasets;
    unsigned int cylinders,heads;
} TDAMAP;
/* Geometry is declared by format 4; this does not establish device capacity.
 * The current VTOC position remains cylinders 1-2 on a 3390 (15 heads).
 * No output is modified on failure. Capacity is in one-byte map cells. */
int TDAGEOM(const unsigned char *,unsigned int,unsigned int *,unsigned int *);
int TDARANGE(const TDAMAP *,unsigned int,unsigned int,unsigned int,
             unsigned int,unsigned int *,unsigned int *);
int TDAINIT(TDAMAP *,unsigned char *,unsigned int,const unsigned char *,unsigned int);
int TDAADD(TDAMAP *,const unsigned char *,unsigned int);
int TDASTAT(TDAMAP *);
#endif
