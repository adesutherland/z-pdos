/* SPDX-License-Identifier: MIT */
#include "twospace_cmscursor.h"

TSIINPUT *TSIFIND(TSISTATE *state, const unsigned char id[18],
                  unsigned int profile)
{
    unsigned int slot, i;
    if (!state || !id || (profile!=24U && profile!=31U)) return 0;
    for (slot=0U; slot<TSI_SLOTS; ++slot) {
        TSIINPUT *input=&state->slot[slot];
        if (!input->real || input->profile!=profile) continue;
        for (i=0U; i<18U && input->id[i]==id[i]; ++i) {}
        if (i==18U) return input;
    }
    return 0;
}

TSIINPUT *TSIEMPTY(TSISTATE *state)
{
    unsigned int slot;
    if (!state) return 0;
    for (slot=0U; slot<TSI_SLOTS; ++slot)
        if (!state->slot[slot].real) return &state->slot[slot];
    return 0;
}

unsigned int TSIOWNER(const TSISTATE *state, const TSIINPUT *input,
                      unsigned int owner_base)
{
    unsigned int slot;
    if (!state || !input || !owner_base) return 0U;
    for (slot=0U; slot<TSI_SLOTS; ++slot)
        if (input==&state->slot[slot]) return owner_base+slot;
    return 0U;
}

void TSICLEAR(TSIINPUT *input)
{
    unsigned int i;
    if (!input) return;
    for (i=0U; i<18U; ++i) input->id[i]=0U;
    input->profile=input->real=input->length=0U;
    input->records=input->cursor=0U;
}
