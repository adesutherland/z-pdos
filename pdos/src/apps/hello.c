/* SPDX-License-Identifier: MIT; C90 arguments example, U31. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv)
{
    int i;unsigned char *memory;unsigned long at,bytes=335544320UL;
    if(argc==2&&!strcmp(argv[1],"--memory-check")){
        memory=(unsigned char *)malloc(bytes);
        if(!memory){puts("HELLO: 320 MiB allocation failed");return 8;}
        for(at=0UL;at<bytes;at+=4096UL)memory[at]=(unsigned char)(at/4096UL);
        memory[bytes-1UL]=0x5aU;
        for(at=0UL;at<bytes;at+=4096UL)if(memory[at]!=(unsigned char)(at/4096UL)){
            free(memory);puts("HELLO: memory verification failed");return 12;
        }
        if(memory[bytes-1UL]!=0x5aU){free(memory);return 12;}
        free(memory);puts("HELLO: 320 MiB allocated, every page verified, released");return 0;
    }
    puts("Hello from a C application in U.");
    for(i=1;i<argc;++i)printf("argument %d: %s\n",i,argv[i]);
    return 0;
}
