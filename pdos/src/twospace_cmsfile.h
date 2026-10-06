/* SPDX-License-Identifier: MIT
 * Checked CMS variable-record file envelope in fixed 18452-byte blocks.
 * Pure C89: usable by the K C31 reader and host corruption checks.
 */
#ifndef PDOS_TWO_SPACE_CMSFILE_H
#define PDOS_TWO_SPACE_CMSFILE_H

#define TSIF_OK 0
#define TSIF_BAD -1
#define TSIF_BLOCK 18452U
#define TSIF_LIMIT (3U*1024U*1024U)

typedef struct {
    unsigned int profile;
    unsigned int payload_bytes;
    unsigned int source_bytes;
    unsigned int records;
    unsigned int blocks;
} TSIFINFO;

int TSIFHEADER(const unsigned char *first, unsigned int length,
               unsigned int expected_profile, TSIFINFO *info);
int TSIFVALIDATE(const unsigned char *stage, unsigned int length,
                 unsigned int expected_profile, TSIFINFO *info);

#endif
