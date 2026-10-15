/* SPDX-License-Identifier: MIT; supplied-storage JCL admission in U. */
#ifndef PDOS_TWO_SPACE_JCL_H
#define PDOS_TWO_SPACE_JCL_H
#include "twospace_job.h"
#include "twospace_jobdd.h"
#define TJC_CARDS 512U
#define TJC_TEXT 1024U
#define TJC_SYMBOLS 32U
#define TJC_LIBS 4U
#define TJC_PROCS 4U
#define TJC_SYNTAX 1U
#define TJC_FEATURE 2U
#define TJC_LIMIT 3U
#define TJC_SYMBOL 4U
#define TJC_END 5U
#define TJC_RESOLVE 6U
#define TJC_DESCRIPTOR 7U
typedef struct { unsigned int card,column,reason; } TJCERROR;
typedef struct { char dsn[45]; } TJCLIB;
typedef struct {
    char name[18],local[9],program[9];
    unsigned int card,library_count,explicit_library,procedure;
    TJCLIB libraries[TJC_LIBS];
} TJCSTEP;
typedef struct { unsigned int direction,recfm,lrecl,blksize; } TJCATTR;
typedef struct {
    unsigned int step,card,column,kind,ordinal,status,disposition;
    unsigned int offset,bytes,quota;
    unsigned int space_unit,space_primary,space_secondary,unit;
    char volume[7];
    char name[9],dsn[45],member[9];
    TJCATTR attributes;
} TJCDECL;
/* Both callbacks are read-only admission queries. They must neither run code
 * nor allocate/change guest datasets. Strings use the C execution character
 * set; returned program names are eight IBM1047 bytes. Dataset/record lookup
 * supplies DD direction (DISP is not an OPEN mode) and omitted DCB attributes.
 * Explicit attributes/directions cannot be changed by a callback. */
typedef struct {
    void *context;
    int (*program)(void *,const TJCSTEP *,unsigned char program[8]);
    int (*dd)(void *,const TJCSTEP *,const TJCDECL *,TJCATTR *);
    int (*procedure)(void *,const char *,const TJCLIB *,unsigned int,
                     const unsigned char **,unsigned int *);
} TJCSITE;
typedef struct {
    unsigned int valid,steps,dds;
    TJCSTEP step[TJB_STEPS];
    unsigned char job[TJB_FULL_BYTES],dd[TJD_EXT_BYTES];
} TJCRESULT;
typedef struct { char name[9],value[101]; } TJCSYM;
typedef struct {
    unsigned int first,next,ordinal,used;
    char step[9],name[9];
} TJCOVR;
typedef struct {
    char key[18],value[256];
    unsigned int phase,card,column;
} TJCFIELD;
typedef struct {
    unsigned int active,next,cards,outer_count,arguments,overrides,last_override;
    unsigned int first_step,parameter_set,parameter_used,index;
    const unsigned char *deck;
    char call[9],procedure[9],parameter[101];
    char parm_step[TJB_STEPS][9],parm_value[TJB_STEPS][101];
    unsigned int parms,parm_used;
    TJCSYM outer[TJC_SYMBOLS],argument[TJC_SYMBOLS];
    TJCOVR override[TJD_MAX_COUNT];
} TJCPROC;
/* Caller-owned, typed storage; no allocator, global mutable state or POSIX.
 * The deck, workspace, result, site and diagnostic objects must be distinct.
 * The complete workspace is below the frozen 64 KiB converter envelope. */
typedef struct {
    TJCRESULT result;
    TJCDECL declaration[TJD_MAX_COUNT];
    TJCSYM symbol[TJC_SYMBOLS];
    TJCLIB joblib[TJC_LIBS];
    TJCLIB proclib[TJC_LIBS];
    TJCPROC proc;
    TJCFIELD field[16];
    unsigned int proclibs,fields,calls;
    char call_name[TJB_STEPS][9];
    unsigned int symbols,joblibs,inline_at,card,column,ended,started,last_library;
    char name[18],operation[9],text[TJC_TEXT],expanded[TJC_TEXT];
    unsigned short source_card[TJC_TEXT],source_column[TJC_TEXT];
    unsigned short expanded_card[TJC_TEXT],expanded_column[TJC_TEXT];
} TJCWORK;
/* Exactly complete 80-byte IBM1047 records, including an explicit // end card.
 * Failure clears the entire result; no runnable prefix is returned. Callbacks
 * run only after the whole deck has passed syntax and framing validation.
 * Procedures are immutable read-only inputs obtained through the site callback.
 * No program or application DD admission query occurs before full conversion. */
int TJCCONV(TJCWORK *,const unsigned char *,unsigned int bytes,
            const TJCSITE *,TJCRESULT *,TJCERROR *);
/* Explicit printable character conversion, shared with the independent host
 * controls and U intake. Neither function assumes ASCII host execution text. */
int TJCENC(const char *,unsigned int,unsigned char *);
int TJCDEC(const unsigned char *,unsigned int,char *);
const char *TJCMSG(unsigned int reason);
#endif
