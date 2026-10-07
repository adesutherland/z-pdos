/* SPDX-License-Identifier: MIT */
#include "twospace_transcript.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned int word(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
int main(void)
{
    unsigned char bytes[256],saved[64],payload[32];TTXSTATE state;
    memset(payload,0,32);payload[23]=37;
    assert(TTXINIT(&state,bytes,256)==0);
    assert(TTXAPPEND(&state,1,7,1047,0,(const unsigned char *)"COMMAND",7)==0);
    assert(TTXAPPEND(&state,2,7,1047,0,(const unsigned char *)"HELLO",5)==0);
    assert(TTXAPPEND(&state,3,7,0,0,payload,32)==0);
    assert(word(bytes)==1&&word(bytes+4)==1&&word(bytes+12)==1&&word(bytes+20)==7);
    assert(word(bytes+39+4)==2&&word(bytes+39+12)==2&&word(bytes+39+16)==5);
    assert(word(bytes+76+4)==3&&word(bytes+76+12)==3&&word(bytes+76+16)==32);
    memcpy(saved,bytes,64);
    assert(TTXAPPEND(&state,2,7,1047,0,bytes,128)==-2&&state.gaps==1&&state.full);
    assert(!memcmp(saved,bytes,64)&&word(bytes+140+4)==5&&word(bytes+140+12)==4);
    assert(TTXAPPEND(&state,2,7,1047,0,bytes,1)==-2);
    puts("append-only transcript: ordered boundaries/text/result and explicit full-buffer gap pass");
    return 0;
}
