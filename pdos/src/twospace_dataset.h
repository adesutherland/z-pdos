/* SPDX-License-Identifier: MIT
 * Bounded first-extent lookup on the checked 100-cylinder 3390 profile.
 * Records are K-owned and valid only until the next callback read.
 */
#ifndef PDOS_TWO_SPACE_DATASET_H
#define PDOS_TWO_SPACE_DATASET_H

#define TSK_OK 0
#define TSK_BAD -1
#define TSK_ABSENT -2
#define TSK_CORRUPT -3

typedef int (*TSKREAD)(void *context, unsigned int cylinder,
                       unsigned int head, unsigned int record,
                       unsigned int capacity, const unsigned char **data);

typedef struct {
    unsigned int start_cylinder;
    unsigned int start_head;
    unsigned int end_cylinder;
    unsigned int end_head;
    unsigned int record_format;
    unsigned int block_length;
    unsigned int logical_length;
} TSKEXTENT;

int TSKFIND(TSKREAD read_record, void *context,
            const unsigned char *name, unsigned int name_length,
            TSKEXTENT *extent);
int TSKWITHIN(const TSKEXTENT *extent, unsigned int cylinder,
              unsigned int head);

#endif
