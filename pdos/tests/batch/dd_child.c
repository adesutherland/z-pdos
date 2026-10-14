/* SPDX-License-Identifier: MIT; inherited DDs with independent OPEN lifetime. */
#include <stdio.h>
extern int D4UNDD(void);
int main(void)
{
    FILE *input,*out;unsigned char data[256];unsigned int i;
    /* A child cannot deallocate a binding owned by the step context. */
    if(D4UNDD()!=12)return 61;
    input=fopen("dd:SYSIN","rb");if(!input)return 62;
    if(fread(data,1U,sizeof data,input)!=sizeof data)return 63;
    for(i=0U;i<sizeof data;++i)if(data[i]!=(unsigned char)i)return 64;
    if(fgetc(input)!=EOF||ferror(input)||fclose(input))return 65;
    out=fopen("dd:FBOUT","w");if(!out)return 66;
    if(fputs("CHILD DD RECORD\n",out)<0||fclose(out))return 67;
    return 0;
}
