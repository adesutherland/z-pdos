/* SPDX-License-Identifier: MIT; independent old-layout fixtures and migration. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "twospace_job.h"
#include "twospace_jobdd.h"
static unsigned char pool[TJB_BYTES],legacy[TJB_BYTES],saved[TJB_BYTES];
static unsigned char d[TJD_EXT_BYTES],job[TJB_DESC_BYTES];
static void name(unsigned char *p,unsigned int first)
{memset(p,0x40,8U);p[0]=(unsigned char)first;}
int main(void)
{
 TJBPOOL p,q;unsigned int id,i;unsigned char *e,*j;
 memset(job,0,sizeof job);TJBPUT(job,1U);TJBPUT(job+4U,sizeof job);name(job+8U,0xd1U);
 TJBPUT(job+16U,1U);TJBPUT(job+20U,4096U);name(job+32U,0xe2U);name(job+40U,0xd7U);
 memset(d,0,sizeof d);TJBPUT(d,1U);TJBPUT(d+4U,TJD_BYTES);TJBPUT(d+8U,1U);TJBPUT(d+12U,1U);TJBPUT(d+16U,1U);
 e=d+32U;TJBPUT(e,1U);TJBPUT(e+4U,TJD_DUMMY);TJBPUT(e+8U,TJD_INPUT);TJBPUT(e+12U,TJD_FB);TJBPUT(e+16U,80U);TJBPUT(e+20U,80U);name(e+32U,0xe2U);
 assert(!TJBINIT(&p,pool,sizeof pool)&&!TJBSUB(&p,7U,job,sizeof job,&id)&&!TJBBIND(&p,7U,id,d,TJD_BYTES));
 /* Manufacture the original 256 KiB wire layout independently: the occupied
    job/DD is deliberately in slot 2 to exercise the backwards relocation. */
 memset(legacy,0,sizeof legacy);memcpy(legacy,pool,TJB_DD_BASE);TJBPUT(legacy+4U,3U);
 memcpy(legacy+TJB_BASE+2U*TJB_SLOT_BYTES,legacy+TJB_BASE,TJB_SLOT_BYTES);memset(legacy+TJB_BASE,0,TJB_SLOT_BYTES);
 memcpy(legacy+TJB_DD_BASE+2U*TJD_BYTES,d,TJD_BYTES);memset(legacy+TJB_LEGACY_BYTES,0xa5,TJB_BYTES-TJB_LEGACY_BYTES);
 memcpy(saved,legacy,sizeof saved);q.bytes=0;
 assert(TJBUP(&q,legacy,TJB_LEGACY_BYTES,TJB_LEGACY_BYTES)==8&&!q.bytes&&!memcmp(legacy,saved,sizeof legacy));
 legacy[TJB_DD_BASE+2U*TJD_BYTES+31U]=1U;memcpy(saved,legacy,sizeof saved);
 assert(TJBUP(&q,legacy,sizeof legacy,TJB_LEGACY_BYTES)==8&&!q.bytes&&!memcmp(legacy,saved,sizeof legacy));
 legacy[TJB_DD_BASE+2U*TJD_BYTES+31U]=0U;
 assert(!TJBUP(&q,legacy,sizeof legacy,TJB_LEGACY_BYTES)&&TJBWORD(legacy+4U)==4U);
 j=TJBFIND(&q,7U,id);assert(j&&TJBWORD(j+8U)==TJB_HELD&&!memcmp(TJBDDS(&q,id),d,TJD_BYTES));
 for(i=TJD_BYTES;i<TJD_EXT_BYTES;++i)assert(TJBDDS(&q,id)[i]==0U);
 assert(!TJBLOAD(&q,legacy,sizeof legacy));
 assert(!TJBACT(&q,7U,id,3U)&&!TJBACT(&q,7U,id,4U));
 /* A real expanded allocation occupies entry 20, outside the old 16 bound. */
 memset(d,0,sizeof d);TJBPUT(d,3U);TJBPUT(d+4U,sizeof d);TJBPUT(d+8U,20U);TJBPUT(d+12U,4U);TJBPUT(d+16U,1U);
 for(i=0U;i<20U;++i){e=d+32U+i*TJD_ENTRY;TJBPUT(e,i/5U+1U);TJBPUT(e+4U,TJD_DUMMY);TJBPUT(e+8U,TJD_INPUT);
  TJBPUT(e+12U,TJD_FB);TJBPUT(e+16U,80U);TJBPUT(e+20U,80U);name(e+32U,0xc1U+i%5U);}
 assert(!TJDCHK(d,4U));TJBPUT(d+4U,TJD_BYTES);assert(TJDCHK(d,4U)==8);TJBPUT(d+4U,sizeof d);
 d[TJD_EXT_BYTES-1U]=1U;assert(TJDCHK(d,4U)==8);
 puts("PD033 expanded DD and legacy spool migration controls PASS");return 0;
}
