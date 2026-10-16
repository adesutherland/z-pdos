/* SPDX-License-Identifier: MIT; bounded native record compare-and-save. */
#ifndef PDOS_EDITFILE_H
#define PDOS_EDITFILE_H
#define TEF_BYTES 144U
#define TEF_LIMIT 65536U
#define TEF_LOAD 0U
#define TEF_SAVE 1U
#define TEF_NEW 2U
#define TEF_EXISTS 1U
#define TEF_MEMBER 2U
#define TEF_JOURNAL 4U
#define TEF_ID_BYTES 68U
/* v1 words0..36: version,size,action,capacity,used,recfm,lrecl,blksize,
 * record count,flags. IBM1047 DSN[44] at40, member[8] at84, volume[6] at92;
 * bytes98..103 reserved. SHA256 expected/result token at104; AL8 payload136.
 * Input used/count/flags are zero; LOAD attributes are zero, SAVE/NEW carry
 * the opened attributes at20..28 as a separate conflict check. LOAD with
 * capacity0 returns metadata/token only. SAVE/NEW compare the supplied token.
 * Payload is repeated big-endian halfword length plus exact logical bytes.
 * Safe saves use the IPL volume's native TSO journal. */
typedef struct {unsigned int format,lrecl,blksize;} TEFATTR;
int TEFVALID(const TEFATTR *);
int TEFUNPK(const TEFATTR *,const unsigned char *,unsigned int,
            unsigned char *,unsigned int,unsigned int *,unsigned int *);
int TEFPACK(const TEFATTR *,const unsigned char *,unsigned int,
            unsigned char *,unsigned int,unsigned int *,unsigned int *);
int TEFTOK(const unsigned char [52],const TEFATTR *,unsigned int,
           const unsigned char *,unsigned int,unsigned char *,unsigned int,
           unsigned char [32]);
unsigned int TEFFILE(unsigned int,const char *,unsigned char *,unsigned int,
                      unsigned char [TEF_BYTES]);
#endif
