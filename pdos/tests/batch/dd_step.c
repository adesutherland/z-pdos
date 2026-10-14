/* SPDX-License-Identifier: MIT; real native PDPCLIB/BSAM DD consumer.
 * There is no SVC 200 spool emission in this program or its child. */
#include <stdio.h>
#include <string.h>
#include <errno.h>
extern int D4LINK(void),D4BAD(void),D4TPUT(void);
#define REQUIRE(x,n) do{if(!(x))return n;}while(0)
int main(void)
{
    FILE *input,*out,*empty;unsigned char data[128];unsigned int i;
    REQUIRE(D4BAD()==12,71);REQUIRE(D4TPUT()==20,72);
    REQUIRE(fopen("dd:ABSENT","rb")==NULL,73);
    REQUIRE(fopen("dd:SYSIN","wb")==NULL,74);
    empty=fopen("dd:EMPTY","rb");REQUIRE(empty,75);
    REQUIRE(fgetc(empty)==EOF&&!ferror(empty)&&!fclose(empty),76);
    out=fopen("dd:SINK","w");REQUIRE(out,77);
    REQUIRE(fputs("DISCARDED\n",out)>=0&&!fclose(out),78);
    input=fopen("dd:SYSIN","rb");if(!input)return 1000+errno;
    REQUIRE(fread(data,1U,128U,input)==128U,80);
    for(i=0U;i<128U;++i)REQUIRE(data[i]==(unsigned char)i,81);
    REQUIRE(!D4LINK(),82);
    REQUIRE(fread(data,1U,128U,input)==128U,83);
    for(i=0U;i<128U;++i)REQUIRE(data[i]==(unsigned char)(i+128U),84);
    REQUIRE(fgetc(input)==EOF&&!ferror(input)&&!fclose(input),85);
    /* Reopening gets a new invocation-owned cursor on the same binding. */
    input=fopen("dd:SYSIN","rb");REQUIRE(input&&fgetc(input)==0&&!fclose(input),86);
    out=fopen("dd:FBOUT","w");REQUIRE(out,87);
    REQUIRE(fputs("PARENT DD RECORD\n",out)>=0&&!fclose(out),88);
    out=fopen("dd:VBOUT","w");REQUIRE(out,89);
    REQUIRE(fputs("\nVB TEXT\n",out)>=0&&!fclose(out),90);
    return 0;
}
