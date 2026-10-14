/* SPDX-License-Identifier: MIT; independent wire and ownership controls. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "twospace_batch.h"
static void put(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
int main(void)
{
    unsigned char *real=malloc(0x100000U),*irb,*wire,saved[512];
    TSCSTATE reader,printer;TBCRESULT result;TBCPOOL pool;unsigned int a,b;
    assert(real);memset(real,0xa5,0x100000U);memset(&pool,0,sizeof pool);
    assert(!TSCINIT(&reader,real,0x100000U,TBC_READER_REAL));
    assert(!TSCINIT(&printer,real,0x100000U,TBC_PRINTER_REAL));
    assert(!TBCBUILD(&reader,TBC_READER,2U,80U));
    wire=real+TBC_READER_REAL+TSC_CCW_OFFSET;
    assert(!memcmp(wire,"\x02\x20\x00\x50\x00\x0a\x10\x00",8U));
    assert(real[TBC_READER_REAL-1U]==0xa5U&&real[TBC_READER_REAL+TSC_REGION_BYTES]==0xa5U);
    memcpy(saved,real+TBC_READER_REAL,sizeof saved);
    assert(TBCBUILD(&reader,TBC_READER,2U,79U)==TBC_BAD);
    assert(TBCBUILD(&reader,TBC_READER,9U,80U)==TBC_BAD);
    assert(!memcmp(saved,real+TBC_READER_REAL,sizeof saved));
    reader.occupied=1U;assert(TBCBUILD(&reader,TBC_READER,2U,80U)==TBC_BAD);
    assert(!memcmp(saved,real+TBC_READER_REAL,sizeof saved));reader.occupied=0U;
    irb=TSCIRB(&reader);put(irb+4U,0x000a0208U);irb[8U]=12U;
    assert(TBCCHECK(&reader,irb,TBC_READER,2U,80U,&result)==TBC_OK&&result.actual==80U&&!result.eof);
    irb[11U]=1U;assert(TBCCHECK(&reader,irb,TBC_READER,2U,80U,&result)==TBC_IO&&!result.actual);
    irb[8U]=13U;irb[11U]=80U;
    assert(TBCCHECK(&reader,irb,TBC_READER,2U,80U,&result)==TBC_EOF&&result.eof&&!result.actual);
    irb[11U]=79U;assert(TBCCHECK(&reader,irb,TBC_READER,2U,80U,&result)==TBC_IO&&!result.eof);
    irb[11U]=80U;irb[8U]=14U;
    assert(TBCCHECK(&reader,irb,TBC_READER,2U,80U,&result)==TBC_IO&&result.device_status==14U&&!result.eof);
    irb[8U]=13U;irb[9U]=1U;assert(TBCCHECK(&reader,irb,TBC_READER,2U,80U,&result)==TBC_IO);
    irb[9U]=0U;put(irb+4U,0x000b0208U);assert(TBCCHECK(&reader,irb,TBC_READER,2U,80U,&result)==TBC_IO);
    memset(TSCDATA(&printer),0xd7,132U);assert(!TBCBUILD(&printer,TBC_PRINTER,9U,132U));
    assert(!memcmp(real+TBC_PRINTER_REAL+TSC_CCW_OFFSET,"\x09\x20\x00\x84\x00\x0b\x10\x00",8U));
    assert(TSCDATA(&printer)[0U]==0xd7U&&TSCDATA(&printer)[131U]==0xd7U);
    assert(!TBCBUILD(&printer,TBC_PRINTER,0x89U,1U));
    assert(real[TBC_PRINTER_REAL+TSC_CCW_OFFSET]==0x89U);
    assert(TBCBUILD(&printer,TBC_PRINTER,9U,133U)==TBC_BAD&&TBCBUILD(&printer,TBC_PRINTER,9U,0U)==TBC_BAD);
    irb=TSCIRB(&printer);put(irb+4U,0x000b0208U);irb[8U]=12U;
    assert(TBCCHECK(&printer,irb,TBC_PRINTER,0x89U,1U,&result)==TBC_OK&&result.actual==1U);
    irb[8U]=8U;assert(TBCCHECK(&printer,irb,TBC_PRINTER,0x89U,1U,&result)==TBC_IO);
    irb[8U]=13U;assert(TBCCHECK(&printer,irb,TBC_PRINTER,0x89U,1U,&result)==TBC_IO&&!result.eof);
    assert(!TBCBUILD(&printer,TBC_PRINTER,4U,8U));put(irb+4U,0x000b0208U);irb[8U]=12U;irb[11U]=7U;
    assert(TBCCHECK(&printer,irb,TBC_PRINTER,4U,8U,&result)==TBC_OK&&result.actual==1U);
    printer.quarantined=1U;assert(TBCBUILD(&printer,TBC_PRINTER,9U,1U)==TBC_BAD);printer.quarantined=0U;
    assert(!TBCOPEN(&pool,11U,TBC_READER,12U,&a));
    assert(TBCOPEN(&pool,12U,TBC_READER,12U,&b)==TBC_BUSY);
    assert(TBCOPEN(&pool,12U,TBC_PRINTER,12U,&b)==TBC_BUSY);
    assert(!TBCFIND(&pool,12U,a)&&TBCCLOSE(&pool,12U,a)==TBC_BAD);
    assert(!TBCCLOSE(&pool,11U,a)&&!TBCFIND(&pool,11U,a));
    assert(!TBCOPEN(&pool,12U,TBC_READER,12U,&b)&&b!=a);
    assert(!TBCCLOSE(&pool,12U,b));pool.next_handle=0xffffffffU;
    assert(TBCOPEN(&pool,12U,TBC_READER,12U,&b)==TBC_BAD&&!pool.endpoint[0].handle);
    free(real);puts("PD033 wire/status/EOF/partial/owner/handle controls PASS");return 0;
}
