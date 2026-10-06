/* SPDX-License-Identifier: MIT
 * Checked native MVS load-module input for the shared-U successor.
 * The format handling follows Paul Edwards's public-domain pdosutil loader;
 * K materializes into private staging before mapping executable U pages.
 */
#ifndef PDOS_TWO_SPACE_TSO_H
#define PDOS_TWO_SPACE_TSO_H

#define TST_OK 0
#define TST_BAD -1
#define TST_MAX_RAW (5U*1024U*1024U)
#define TST_MAX_IMAGE (5U*1024U*1024U)
#define TST_BLOCK 18452U
typedef struct {
    unsigned int raw_bytes, records, max_record;
    unsigned int mode, rmode_any, flags;
    unsigned int entry_offset, image_bytes;
    unsigned int input_fnv;
} TSTINFO;

int TSTHEADER(const unsigned char *raw, unsigned int bytes,
              unsigned int expected_mode, TSTINFO *info);
int TSTSTAGEHEADER(const unsigned char *block, unsigned int length,
                   unsigned int *raw_bytes, unsigned int *blocks);
int TSTSTAGEHEADER64(const unsigned char *block, unsigned int length,
                     unsigned int *raw_bytes, unsigned int *blocks);
int TSTSTAGEVALIDATE(const unsigned char *stage, unsigned int length,
                     unsigned int expected_mode, TSTINFO *info);
/* AMODE31/RMODE ANY only. The caller owns a K-only destination with at least
 * capacity bytes; no partly built image is ever mapped into U on failure. */
int TSTIMAGE31(const unsigned char *raw, unsigned int bytes,
               unsigned int base, unsigned char *image,
               unsigned int capacity, TSTINFO *info);
/* AMODE64/RMODE ANY classic low-resident member. This does not accept the
 * separate RMODE HIGH AL8 relocation format. */
int TSTIMAGE64ANY(const unsigned char *raw, unsigned int bytes,
                  unsigned int base, unsigned char *image,
                  unsigned int capacity, TSTINFO *info);

#endif
