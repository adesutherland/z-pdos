/* SPDX-License-Identifier: MIT; independently expected SF and printer bytes. */
#include "twospace_3270.h"
#include <assert.h>
#include <string.h>
int main(void)
{
    unsigned char out[32],old[32],attrs[]={0x41U,0x42U},text[]={0xc1U,0xc2U,0x15U,0xc3U};unsigned int n=99U;
    static const unsigned char reply[]={0U,7U,9U,0U,2U,0x41U,0x42U};
    static const unsigned char printer[]={0xc8U,0xc1U,0xc2U,0x15U,0xc3U,0x19U};
    assert(!T27REPLY(2U,attrs,2U,out,sizeof out,&n)&&n==sizeof reply&&!memcmp(out,reply,n));
    memset(out,0x55,sizeof out);memcpy(old,out,sizeof out);n=99U;
    assert(T27REPLY(0U,attrs,2U,out,sizeof out,&n)&&!memcmp(old,out,sizeof out)&&n==99U);
    assert(!T27PRINT(text,sizeof text,out,sizeof out,&n)&&n==sizeof printer&&!memcmp(out,printer,n)&&!T27PRWRT(out,n));
    memset(out,0x55,sizeof out);memcpy(old,out,sizeof out);n=99U;text[1]=0x11U;
    assert(T27PRINT(text,sizeof text,out,sizeof out,&n)&&!memcmp(old,out,sizeof out)&&n==99U);
    assert(T27PRWRT(printer,0U));return 0;
}
