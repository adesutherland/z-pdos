/* SPDX-License-Identifier: MIT; supplied-storage batch DD descriptors. */
#ifndef PDOS_TWO_SPACE_JOBDD_H
#define PDOS_TWO_SPACE_JOBDD_H
#define TJD_BYTES 3072U
#define TJD_COUNT 16U
#define TJD_ENTRY 96U
#define TJD_DATA (32U+TJD_COUNT*TJD_ENTRY)
#define TJD_DATASET 1U
#define TJD_INLINE 2U
#define TJD_DUMMY 3U
#define TJD_SYSOUT 4U
#define TJD_TEMP 5U
#define TJD_CONCAT 3U
#define TJD_PASS 1U
#define TJD_DELETE 2U
#define TJD_KEEP 3U
#define TJD_NEW 1U
#define TJD_OLD 2U
#define TJD_TEMP_LIMIT 2097152U
#define TJD_INPUT 1U
#define TJD_OUTPUT 2U
#define TJD_FB 0x90U
#define TJD_VB 0x50U
/* All integers are big endian. Header: version, size, entry count, step
 * count, encoding (1 = IBM1047 text / uninterpreted binary octets), reserved.
 * Entry: step (1 based), kind, direction, RECFM, LRECL, BLKSIZE, inline
 * offset/bytes, DD[8], DSN[44], member[8], flags. Version 1 flags are zero.
 * Version 2 flags: bits 0..1 concatenation ordinal (head=0), bits 8..9
 * temporary normal disposition, bits 16..17 temporary NEW/OLD status.
 * TEMP uses offset as its byte quota, DSN as name[8]+36 blanks, no member.
 * TEMP KEEP is PASS; all temporary data is deleted at job end. Inline data is a packed
 * sequence of halfword lengths and exact logical records, including VB empty
 * records. No implicit conversion, trimming or newline interpretation. */
unsigned int TJDWORD(const unsigned char *);
int TJDNAME(const unsigned char *);
int TJDCHK(const unsigned char *,unsigned int steps);
/* Form a native BSAM block. Advance the cursor only on complete success.
 * Return 1 for EOF; an empty VB record is a successful RDW of length four. */
int TJDBLOCK(const unsigned char *entry,const unsigned char *records,
             unsigned int bytes,unsigned int *cursor,unsigned char *block,
             unsigned int capacity,unsigned int *actual);
typedef int (*TJDVISIT)(void *,const unsigned char *,unsigned int);
/* Validate the entire native block before visiting any logical record.
 * cost is the Stage 2 spool byte cost (empty print record becomes one blank).
 * A null visitor performs only validation and cost computation. */
int TJDOUT(const unsigned char *entry,const unsigned char *block,
           unsigned int bytes,unsigned int *cost,TJDVISIT,void *);
#endif
