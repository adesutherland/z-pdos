/* SPDX-License-Identifier: MIT; independent model/address and rendering controls. */
#include "twospace_console.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned int word(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
int main(void)
{
    static const unsigned int rows[4]={24,32,43,27},cols[4]={80,80,80,132};
    static const unsigned char last[4][2]={{0x5d,0x7f},{0xe7,0x7f},{0xf5,0x6f},{0xf7,0x6b}};
    static const unsigned char text[5]={0xc8,0xc5,0xd3,0xd3,0xd6};
    unsigned char stream[16384],capbytes[64],address[2],input[262],answer[256];
    unsigned int model,n,i,decoded,len,aid,command;
    TTCCAP cap;TTCVIEW view;TTCFIELD field;
    for(model=2;model<=5;++model){
        assert(TTCCONFIG(&cap,1,9,model,10)==TTC_OK);
        assert(cap.rows==rows[model-2]&&cap.columns==cols[model-2]);
        assert(TTCCAPS(&cap,capbytes)==TTC_OK&&word(capbytes)==1&&word(capbytes+4)==64);
        assert(word(capbytes+16)==rows[model-2]&&word(capbytes+20)==cols[model-2]);
        assert(TTCADDR(&cap,cap.rows*cap.columns-1,address)==TTC_OK&&!memcmp(address,last[model-2],2));
        assert(TTCDECODE(&cap,address,&decoded)==TTC_OK&&decoded==cap.rows*cap.columns-1);
        assert(TTCVINIT(&view,&cap)==TTC_OK);
        for(i=0;i<100;++i)assert(TTCVLINE(&view,text,5)==TTC_OK);
        assert(TTCVSCREEN(&view,text,5,text,5,stream,sizeof stream,&len)==TTC_OK);
        assert(stream[0]==0xc3&&stream[1]==0x11&&!memcmp(stream+2,last[model-2],2)&&stream[4]==0x1d&&stream[5]==0xf8);
        assert(stream[len-1]==0x13&&len<sizeof stream);
        input[0]=0x7d;TTCADDR(&cap,view.entry_row*cap.columns+1,input+1);
        input[3]=0x11;memcpy(input+4,input+1,2);
        for(i=0;i<256;++i)input[6+i]=0xc1;
        assert(TTCINPUT(&cap,input,262,view.entry_row*cap.columns+1,answer,256,&n,&aid)==TTC_OK&&n==256&&aid==0x7d);
        assert(TTCINPUT(&cap,input,3,view.entry_row*cap.columns+1,answer,256,&n,&aid)==TTC_OK&&n==0);
        input[0]=0xf3U;
        assert(TTCINPUT(&cap,input,3,view.entry_row*cap.columns+1,answer,256,&n,&aid)==TTC_OK&&n==0&&aid==0xf3U);
        input[0]=0x6cU;
        assert(TTCINPUT(&cap,input,1,view.entry_row*cap.columns+1,answer,256,&n,&aid)==TTC_OK&&n==0&&aid==0x6cU);
        stream[0]=0x27;stream[1]=0x7e;stream[2]=0xc3;stream[3]=0x11;
        TTCADDR(&cap,112,stream+4);stream[6]=0x1d;stream[7]=0xf0;stream[8]=0xc1;
        assert(TTCRAW(&cap,stream,9,&command)==TTC_OK&&command==13);
        field.row=0;field.column=0;field.attributes=0x30;field.text=text;field.length=5;
        memset(stream,0xa5,sizeof stream);field.row=cap.rows;
        assert(TTCENCODE(&cap,&field,1,0,0,stream,sizeof stream,&len)==TTC_BAD&&stream[0]==0xa5);
    }
    assert(TTCCONFIG(&cap,2,9,0,0)==TTC_OK&&TTCCAPS(&cap,capbytes)==TTC_OK&&word(capbytes+16)==0&&!(word(capbytes+48)&TSA_FEATURE_SCREEN));
    assert(TTCVINIT(&view,&cap)==TTC_BAD);
    puts("four console models: geometry, address coding, scroll/layout, full input and raw native stream pass");
    return 0;
}
