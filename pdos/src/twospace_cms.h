/* SPDX-License-Identifier: MIT
 * Bounded staged CMS MODULE inspection and image materialization.
 */
#ifndef PDOS_TWO_SPACE_CMS_H
#define PDOS_TWO_SPACE_CMS_H

#define TSH_OK 0
#define TSH_BAD -1

typedef struct {
    unsigned int profile;
    unsigned int module_bytes;
    unsigned int image_bytes;
    unsigned int records;
    unsigned int entry;
    unsigned int origin;
    unsigned int end;
    unsigned int relocation_records;
} TSHINFO;

int TSHHEADER(const unsigned char *block, unsigned int length,
              unsigned int expected_profile, TSHINFO *info);
int TSHVALIDATE(const unsigned char *staged, unsigned int length,
                unsigned int expected_profile, TSHINFO *info);
int TSHIMAGE(const unsigned char *staged, unsigned int length,
             unsigned int expected_profile, unsigned int base,
             unsigned char *destination, unsigned int capacity,
             unsigned int *entry);

#endif
