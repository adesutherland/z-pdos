/* SPDX-License-Identifier: MIT; diagnostic PCOMM's native console boundary. */
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
void *tsuopen(const char *,int *,int *,int *,int *,void **,const char *);
int tsuread(void *,void *,size_t *);
int tsuwrite(void *,unsigned char **,size_t *);
void tsuclose(void *);
int tsudyn(size_t,const char *,size_t,const char *);
static unsigned int written, input_bytes;
unsigned int TSUTPUT(unsigned int bytes,const unsigned char *data)
{ assert(data && bytes<=130); written+=bytes; return 0; }
unsigned int TSUTGET(unsigned int capacity,unsigned char *data)
{ assert(capacity==252 && data); memset(data,'X',input_bytes); return input_bytes; }
int main(void)
{
    int mode=1, recfm=0, lrecl=0, block=0;
    void *output,*input,*buffer;
    unsigned char *record;
    size_t length;
    output=tsuopen("SYSPRINT",&mode,&recfm,&lrecl,&block,&buffer,0);
    assert((ptrdiff_t)output>0 && recfm==1 && lrecl==256 && block==256);
    record=(unsigned char *)buffer; memset(record,0,256);
    record[0]=1; record[1]=0; length=256;
    assert(tsuwrite(output,&record,&length)==0 && written==252);
    record[0]=1; record[1]=1;
    assert(tsuwrite(output,&record,&length)==8 && written==252);
    mode=0; input=tsuopen("SYSIN   ",&mode,&recfm,&lrecl,&block,&buffer,0);
    input_bytes=252;
    assert(tsuread(input,&record,&length)==0 && length==256 &&
           record[0]==1 && record[1]==0 && record[255]=='X');
    input_bytes=0;
    assert(tsuread(input,&record,&length)==0 && length==4 && record[1]==4);
    tsuclose(input); assert(tsuread(input,&record,&length)==8);
    assert(tsudyn(8,"PDP001HD",8,"FILE.BAT")!=0);
    tsuclose(output);
    puts("PCOMM console bounds, records, empty input and explicit file rejection pass");
    return 0;
}
