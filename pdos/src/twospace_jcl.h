/* SPDX-License-Identifier: MIT; supplied-storage JCL admission in U. */
#ifndef PDOS_TWO_SPACE_JCL_H
#define PDOS_TWO_SPACE_JCL_H
#include "twospace_job.h"
#include "twospace_jobdd.h"
#define TJC_CARDS 512U
#define TJC_TEXT 1024U
#define TJC_SYMBOLS 32U
#define TJC_LIBS 4U
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
    char name[9],program[9];
    unsigned int card,library_count,explicit_library;
    TJCLIB libraries[TJC_LIBS];
} TJCSTEP;
typedef struct { unsigned int direction,recfm,lrecl,blksize; } TJCATTR;
typedef struct {
    unsigned int step,card,column,kind,ordinal,status,disposition;
    unsigned int offset,bytes,quota;
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
} TJCSITE;
typedef struct {
    unsigned int valid,steps,dds;
    TJCSTEP step[TJB_STEPS];
    unsigned char job[TJB_DESC_BYTES],dd[TJD_BYTES];
} TJCRESULT;
typedef struct { char name[9],value[101]; } TJCSYM;
/* Caller-owned, typed storage; no allocator, global mutable state or POSIX.
 * The deck, workspace, result, site and diagnostic objects must be distinct.
 * The complete workspace is below the frozen 64 KiB converter envelope. */
typedef struct {
    TJCRESULT result;
    TJCDECL declaration[TJD_COUNT];
    TJCSYM symbol[TJC_SYMBOLS];
    TJCLIB joblib[TJC_LIBS];
    unsigned int symbols,joblibs,inline_at,card,column,ended,started,last_library;
    char name[18],operation[9],text[TJC_TEXT],expanded[TJC_TEXT];
    unsigned short source_card[TJC_TEXT],source_column[TJC_TEXT];
    unsigned short expanded_card[TJC_TEXT],expanded_column[TJC_TEXT];
} TJCWORK;
/* Exactly complete 80-byte IBM1047 records, including an explicit // end card.
 * Failure clears the entire result; no runnable prefix is returned. Callbacks
 * run only after the whole deck has passed syntax and framing validation.
 * PROC/SPACE and console integration are later S4-02/S4-03 work. */
int TJCCONV(TJCWORK *,const unsigned char *,unsigned int bytes,
            const TJCSITE *,TJCRESULT *,TJCERROR *);
/* Explicit printable character conversion, shared with the independent host
 * controls and U intake. Neither function assumes ASCII host execution text. */
int TJCENC(const char *,unsigned int,unsigned char *);
int TJCDEC(const unsigned char *,unsigned int,char *);
const char *TJCMSG(unsigned int reason);
#endif
