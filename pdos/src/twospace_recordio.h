/* SPDX-License-Identifier: MIT; bounded native record-file exchange. */
#ifndef PDOS_RECORDIO_H
#define PDOS_RECORDIO_H
#define TRI_BYTES 96U
#define TRI_LIMIT 65536U
#define TRI_PROBE 0U
#define TRI_LOAD 1U
#define TRI_SAVE_EMPTY 2U
/* Wire words: version,size,action,capacity,used,recfm,lrecl,blksize,
 * records,explicit-volume flag; dataset[44], name length, AL8 payload pointer.
 * A VOLSER:dataset name supplies a six-byte volume at input offset20 and
 * flag1 at36; otherwise K uses the selected volume. Attributes replace it.
 * Payload: repeated big-endian halfword length then exact record bytes.
 * Empty variable records are length zero; fixed records include padding.
 * Only PS FB/VB, one contiguous cylinder, first-track data plus EOF.
 * At most 64 physical blocks within the 3390 track capacity; spans cannot
 * overlap the request header. Both load and save enforce the same bounds.
 * SAVE_EMPTY requires a registered non-IPL volume and an empty target.
 */
unsigned int TRIFILE(unsigned int action,const char *name,
                     unsigned char *data,unsigned int capacity,
                     unsigned char info[TRI_BYTES]);
#endif
