/* SPDX-License-Identifier: MIT; exact logical record copy example. */
#include <stdio.h>
#include <stdlib.h>
#include "twospace_recordio.h"
int main(int argc,char **argv)
{
    unsigned char *data,info[TRI_BYTES];unsigned int bytes,rc;
    if(argc!=3){puts("RECIO source separate-empty-target");return 8;}
    data=(unsigned char *)malloc(TRI_LIMIT);if(!data)return 4;
    rc=TRIFILE(TRI_LOAD,argv[1],data,TRI_LIMIT,info);
    bytes=((unsigned int)info[16]<<24)|((unsigned int)info[17]<<16)
          |((unsigned int)info[18]<<8)|info[19];
    if(!rc)rc=TRIFILE(TRI_SAVE_EMPTY,argv[2],data,bytes,info);
    printf("RECIO: status=%u, framed bytes=%u\n",rc,bytes);
    free(data);return (int)rc;
}
