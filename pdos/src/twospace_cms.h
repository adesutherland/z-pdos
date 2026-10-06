/* SPDX-License-Identifier: MIT
 * Bounded first-block inspection of a checked staged CMS MODULE. Full
 * record/hash verification and loading are separate gates.
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

#endif
