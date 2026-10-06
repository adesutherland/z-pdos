/* SPDX-License-Identifier: MIT */
#include "twospace_cmscursor.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr,"cmscursor:%d: %s\n",__LINE__,#x); exit(1); } \
} while (0)

int main(void)
{
    TSISTATE state={0};
    unsigned char first[18], second[18];
    TSIINPUT *parent, *child, *other;
    unsigned int i;
    for (i=0U; i<18U; ++i) first[i]=second[i]=0x40U;
    first[0]=0xc1U; second[0]=0xc2U;
    parent=TSIEMPTY(&state);
    CHECK(parent && TSIOWNER(&state,parent,100U)==100U);
    for (i=0U; i<18U; ++i) parent->id[i]=first[i];
    parent->profile=31U; parent->real=0x1000000U; parent->cursor=123U;
    child=TSIEMPTY(&state);
    CHECK(child && child!=parent);
    for (i=0U; i<18U; ++i) child->id[i]=second[i];
    child->profile=31U; child->real=0x1200000U; child->cursor=77U;
    CHECK(TSIFIND(&state,first,31U)==parent &&
          TSIFIND(&state,second,31U)==child);
    CHECK(parent->cursor==123U && TSIOWNER(&state,child,100U)==101U);
    other=TSIEMPTY(&state);
    CHECK(other && other!=child);
    for (i=0U; i<18U; ++i) other->id[i]=first[i];
    other->profile=24U; other->real=0x1400000U; other->cursor=55U;
    CHECK(TSIFIND(&state,first,24U)==other &&
          TSIFIND(&state,first,31U)==parent);
    CHECK(TSIFINDOWNED(&state,first,31U,1U)==0);
    child->token=2U;
    for (i=0U; i<18U; ++i) child->id[i]=first[i];
    CHECK(TSIFINDOWNED(&state,first,31U,2U)==child);
    CHECK(TSIFIND(&state,first,31U)==parent && parent->cursor==123U);
    CHECK(TSIFIND(&state,second,31U)==0);
    TSICLEAR(child);
    CHECK(TSIFIND(&state,second,31U)==0 && TSIFIND(&state,first,31U)==parent);
    CHECK(TSIEMPTY(&state)==child && parent->cursor==123U);
    CHECK(TSIOWNER(&state,parent,0U)==0U);
    CHECK(TSIFIND(&state,first,64U)==0);
    for (i=1U; i<TSI_SLOTS; ++i) state.slot[i].real=i*4096U;
    CHECK(TSIEMPTY(&state)==0);
    puts("CMS input cursor inventory: independent files/profiles and full table");
    return 0;
}
