/* SPDX-License-Identifier: MIT
 * Bounded K-owned CMS input cursor inventory. Identity includes personality;
 * callers never receive a U pointer to the staged record bytes.
 */
#ifndef PDOS_TWO_SPACE_CMSCURSOR_H
#define PDOS_TWO_SPACE_CMSCURSOR_H

#define TSI_SLOTS 8U
typedef struct {
    unsigned char id[18];
    unsigned int profile, real, length, records, cursor;
} TSIINPUT;

typedef struct {
    TSIINPUT slot[TSI_SLOTS];
} TSISTATE;

TSIINPUT *TSIFIND(TSISTATE *state, const unsigned char id[18],
                  unsigned int profile);
TSIINPUT *TSIEMPTY(TSISTATE *state);
unsigned int TSIOWNER(const TSISTATE *state, const TSIINPUT *input,
                      unsigned int owner_base);
void TSICLEAR(TSIINPUT *input);

#endif
