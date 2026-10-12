/* SPDX-License-Identifier: MIT
 * Binary PDS output/readback through the actual newlib/MVSSUPA boundary.
 * Similar member names expose journal keys made from DD-name fragments. */
#include <stdio.h>
#include <string.h>

static int write_member(const char *name,unsigned int tag)
{
    unsigned char data[700];unsigned int i;FILE *file;
    for(i=0U;i<sizeof data;++i)data[i]=(unsigned char)(i*37U+tag);
    file=fopen(name,"wb");if(!file)return 1;
    if(fwrite(data,1U,sizeof data,file)!=sizeof data){fclose(file);return 2;}
    return fclose(file)?3:0;
}
static int read_member(const char *name,unsigned int tag)
{
    unsigned char data[700];unsigned int i;FILE *file;int extra;
    file=fopen(name,"rb");if(!file)return 4;
    if(fread(data,1U,sizeof data,file)!=sizeof data){fclose(file);return 5;}
    for(i=0U;i<sizeof data;++i)if(data[i]!=(unsigned char)(i*37U+tag)){fclose(file);return 6;}
    extra=fgetc(file);
    if(extra!=EOF||ferror(file)){fclose(file);return 7;}
    return fclose(file)?8:0;
}
int main(void)
{
    int rc;
    rc=write_member("MFA12.RXBIN(IOFLA001)",11U);if(rc)return rc;
    rc=write_member("MFA12.RXBIN(IOFLA002)",93U);if(rc)return rc;
    rc=read_member("MFA12.RXBIN(IOFLA001)",11U);if(rc)return rc;
    rc=read_member("MFA12.RXBIN(IOFLA002)",93U);if(rc)return rc;
    rc=write_member("MFA12.RXBIN(IOFLA001)",177U);if(rc)return rc;
    rc=read_member("MFA12.RXBIN(IOFLA002)",93U);if(rc)return rc;
    rc=read_member("MFA12.RXBIN(IOFLA001)",177U);if(rc)return rc;
    puts("NATIVE PDS BINARY ROUNDTRIP PASS");
    return 0;
}
