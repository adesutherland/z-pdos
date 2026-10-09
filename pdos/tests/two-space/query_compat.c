/* SPDX-License-Identifier: MIT; independently captured DX3270 1.7.5 reply. */
#include "twospace_3270.h"
#include <assert.h>
#include <string.h>
int main(void)
{
    static const unsigned char captured[68]={
        0x88,0,8,0x81,0x80,0x81,0x84,0x86,0x87,0,18,0x80,1,0,0,132,0,27,1,0,96,0,112,9,12,13,236,
        0,21,0x86,0,8,0,0xf4,0xf1,0xf1,0xf2,0xf2,0xf3,0xf3,0xf4,0xf4,0xf5,0xf5,0xf6,0xf6,0xf7,0xf7,
        0,14,0x87,5,0,0,0xf1,0xf1,0xf2,0xf2,0xf4,0xf4,0xf8,0xf8,0,6,0x84,0,1,2};
    unsigned char changed[68];T27CAPABILITIES cap,before;unsigned int i;
    memset(&cap,0xa5,sizeof cap);before=cap;
    assert(T27QRPLY(captured,sizeof captured,&cap)==T27_BAD&&!memcmp(&cap,&before,sizeof cap));
    assert(!T27DXQR(captured,sizeof captured,&cap)&&cap.usable_valid&&cap.rows==27U&&cap.columns==132U);
    assert(cap.colours_valid==0xfeU&&cap.highlighting==7U&&cap.colours[5U]==0xf5U);
    assert(T27HAS(&cap,0x80U)&&T27HAS(&cap,0x81U)&&!T27HAS(&cap,0x84U));
    before=cap;
    for(i=0U;i<sizeof captured;++i){memcpy(changed,captured,sizeof changed);changed[i]^=0x80U;
        assert(T27DXQR(changed,sizeof changed,&cap)==T27_BAD&&!memcmp(&cap,&before,sizeof cap));}
    memcpy(changed,captured,sizeof changed);changed[12U]=0U;changed[17U]=50U;changed[25U]=25U;changed[26U]=200U;
    assert(!T27DXQR(changed,sizeof changed,&cap)&&cap.rows==50U&&cap.columns==132U);
    assert(T27DXQR(changed,sizeof changed-1U,&cap)==T27_BAD);
    return 0;
}
