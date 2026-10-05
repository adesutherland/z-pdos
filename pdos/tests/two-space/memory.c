/* Transactional U storage, modes, low budget and page-rights controls. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "twospace_memory.h"
#include "twospace_gate.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"memory:%d: %s\n",__LINE__,#x); return 1; } } while (0)
static unsigned int purges;
static void purge(void *unused) { (void)unused; ++purges; }
static unsigned char keys[4096];
static unsigned int key_changes;
static void set_key(unsigned int real_page, unsigned int key, void *unused)
{
    (void)unused;
    if ((real_page & 4095U) == 0U && real_page < 0x1000000U &&
        (key == 0U || key == 0x80U)) {
        keys[real_page/4096U]=(unsigned char)key;
        ++key_changes;
    }
}
static TSPADDR addr(unsigned int hi, unsigned int lo)
{ TSPADDR a; a.hi=hi; a.lo=lo; return a; }

int main(void)
{
    unsigned char *aperture, *pool, buffer[8], child_image[4];
    TSDSTATE tables;
    TSMSTATE memory;
    TSPADDR at, real, old;
    TSGCONTEXT gate;
    TSGREQUEST request;
    unsigned int initial_low, i, child_real, nested_real, returned_rc;
    aperture=(unsigned char *)malloc(0x1000000U);
    pool=(unsigned char *)malloc(0x40000U);
    CHECK(aperture && pool);
    memset(aperture,0x5a,0x1000000U);
    CHECK(TSDINIT(&tables,pool,0x100000U,0x40000U) == TSD_OK);
    CHECK(TSDLIVE(&tables,purge,0) == TSD_OK);
    CHECK(TSMINIT(&memory,&tables,aperture,0x1000000U,0x200000U,
                  set_key,0) == TSM_OK);
    CHECK(TSMRESERVE(&memory,1U,24U,addr(0U,0U),4096U) == TSP_OK);
    CHECK(TSMRESERVE(&memory,2U,24U,addr(0U,0x20000U),0x19b000U) == TSP_OK);
    initial_low=TSMLOWFREE(&memory);
    CHECK(initial_low == 0x1000000U-4096U-0x19b000U);
    CHECK(TSMALLOC(&memory,10U,24U,addr(0U,0x20000U),
                   addr(0U,0xffffffU),4096U,1,&at) == TSM_COLLISION);
    CHECK(TSMALLOC(&memory,10U,24U,addr(0U,0x20000U),
                   addr(0U,0xffffffU),8191U,0,&at) == TSM_OK);
    CHECK(at.hi == 0U && at.lo == 0x1bb000U);
    CHECK(TSMLOWFREE(&memory) == initial_low-8192U);
    CHECK(TSDLOOKUP(&tables,at,&real) == TSD_OK);
    CHECK(real.hi == 0U && real.lo == 0x200000U);
    CHECK(keys[0x200000U/4096U] == 0x80U);
    CHECK(TSMRIGHTS(0x200000U,&memory) == 3U);
    CHECK(TSMRIGHTS(0x100000U,&memory) == 0U);
    CHECK(TSMRIGHTS(0x3000U,&memory) == 0U);
    for (i=0U; i<8192U; ++i) CHECK(aperture[0x200000U+i] == 0U);
    gate.u_tables=&tables;
    gate.real_aperture=aperture; gate.real_bytes=0x1000000U;
    gate.rights=TSMRIGHTS; gate.rights_context=&memory;
    request.address=addr(0U,at.lo+0xffeU);
    request.length=4U; request.direction=TSG_WRITE; request.svc=120U;
    buffer[0]=1U; buffer[1]=2U; buffer[2]=3U; buffer[3]=4U;
    CHECK(TSGCOPY(&gate,&request,buffer,sizeof buffer) == TSG_OK);
    CHECK(aperture[0x200ffeU] == 1U && aperture[0x201001U] == 4U);
    child_image[0]=0x11U; child_image[1]=0x22U;
    child_image[2]=0x33U; child_image[3]=0x44U;
    CHECK(TSMOVERLAYPUSH(&memory,10U,at,child_image,4U) == TSM_OK);
    CHECK(TSMFREE(&memory,10U,at) == TSM_COLLISION);
    CHECK(TSDLOOKUP(&tables,at,&real) == TSD_OK);
    child_real=real.lo;
    CHECK(child_real != 0x200000U && aperture[child_real] == 0x11U);
    CHECK(keys[child_real/4096U] == 0x80U);
    CHECK(TSMRIGHTS(0x200000U,&memory) == 0U);
    CHECK(TSMRIGHTS(child_real,&memory) == 3U);
    CHECK(TSMOVERLAYPUSH(&memory,10U,at,child_image,4U) == TSM_OK);
    CHECK(TSDLOOKUP(&tables,at,&real) == TSD_OK);
    nested_real=real.lo;
    CHECK(nested_real != child_real && nested_real != 0x200000U);
    aperture[nested_real]=0xeeU;
    CHECK(TSMOVERLAYPOP(&memory,10U,at,12U,&returned_rc) == TSM_OK);
    CHECK(returned_rc == 12U);
    CHECK(TSDLOOKUP(&tables,at,&real) == TSD_OK && real.lo == child_real);
    CHECK(aperture[child_real] == 0x11U && aperture[nested_real] == 0U);
    CHECK(keys[nested_real/4096U] == 0U);
    CHECK(TSMOVERLAYPOP(&memory,10U,at,0x1357U,&returned_rc) == TSM_OK);
    CHECK(returned_rc == 0x1357U);
    CHECK(TSDLOOKUP(&tables,at,&real) == TSD_OK && real.lo == 0x200000U);
    CHECK(aperture[0x200ffeU] == 1U && aperture[0x201001U] == 4U);
    CHECK(aperture[child_real] == 0U);
    CHECK(keys[child_real/4096U] == 0U);
    CHECK(TSMFREE(&memory,11U,at) == TSM_BAD);
    CHECK(TSMFREE(&memory,10U,at) == TSM_OK);
    CHECK(TSMFREE(&memory,10U,at) == TSM_ABSENT);
    CHECK(TSDLOOKUP(&tables,at,&real) == TSD_MISSING);
    CHECK(TSMRIGHTS(0x200000U,&memory) == 0U);
    CHECK(keys[0x200000U/4096U] == 0U);
    CHECK(TSMLOWFREE(&memory) == initial_low);
    CHECK(TSMALLOC(&memory,20U,31U,addr(0U,0x02000000U),
                   addr(0U,0x7fffffffU),4096U,0,&at) == TSM_OK);
    CHECK(at.lo == 0x02000000U && at.hi == 0U);
    CHECK(TSMLOWFREE(&memory) == initial_low);
    CHECK(TSMFREE(&memory,20U,at) == TSM_OK);
    CHECK(TSMALLOC(&memory,30U,64U,addr(1U,0x20000000U),
                   addr(1U,0x2fffffffU),4096U,0,&at) == TSM_OK);
    CHECK(at.hi == 1U && at.lo == 0x20000000U);
    CHECK(TSMFREE(&memory,30U,at) == TSM_OK);
    CHECK(TSMALLOC(&memory,31U,31U,addr(0U,0x02000000U),
                   addr(0U,0x7fffffffU),0xe00000U,0,&at) == TSM_OK);
    CHECK(TSMALLOC(&memory,32U,24U,addr(0U,0x200000U),
                   addr(0U,0xffffffU),4096U,0,&at) == TSM_NOMEM);
    CHECK(TSMLOWFREE(&memory) == initial_low);
    CHECK(TSMFREE(&memory,31U,addr(0U,0x02000000U)) == TSM_OK);
    CHECK(TSDMAP(&tables,addr(0U,0x201000U),addr(0U,0x3000U)) == TSD_OK);
    CHECK(TSMALLOC(&memory,40U,24U,addr(0U,0x200000U),
                   addr(0U,0xffffffU),8192U,0,&at) == TSM_NOMEM);
    CHECK(TSDLOOKUP(&tables,addr(0U,0x200000U),&real) == TSD_MISSING);
    CHECK(TSDLOOKUP(&tables,addr(0U,0x201000U),&real) == TSD_OK &&
          real.lo == 0x3000U);
    CHECK(TSMLOWFREE(&memory) == initial_low);
    CHECK(TSDUNMAP(&tables,addr(0U,0x201000U),&old) == TSD_OK);
    CHECK(old.lo == 0x3000U);
    CHECK(purges > 10U);
    CHECK(key_changes > 10U);
    free(pool); free(aperture);
    puts("shared-U storage: transactional map/free, nested fixed overlay restore, real rights, AMODE bounds, no low fallback pass");
    return 0;
}
