/* SPDX-License-Identifier: MIT; independent inherited temporary cursor. */
#include <stdio.h>
int main(void)
{FILE *f=fopen("dd:WORK","rb");unsigned char data[256];unsigned int i;
 if(!f||fread(data,1U,sizeof data,f)!=sizeof data)return 81;
 for(i=0U;i<256U;++i)if(data[i]!=(unsigned char)i)return 82;
 return fgetc(f)!=EOF||ferror(f)||fclose(f)?83:0;}
