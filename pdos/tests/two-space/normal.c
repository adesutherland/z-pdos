/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <stdlib.h>
#include "twospace_fixture.h"
#include "twospace_dat.h"
static unsigned int purges;
static void purge(void *p) { (void)p; ++purges; }
int main(void) {
 unsigned char *core=(unsigned char *)calloc(1,TSF_CORE_BYTES);
 TSFRESULT r;TSDSTATE k,u;TSPADDR va,pa;
 if(!core)return 2;
 core[TSF_NORMAL_REAL]=0x54;core[TSF_NORMAL_REAL+1]=0x53;
 core[TSF_NORMAL_REAL+2]=0x4e;core[TSF_NORMAL_REAL+3]=0x31;
 if(TSFBUILD(core,TSF_KPOOL_REAL,TSF_UPOOL_REAL,&r,purge,0)||purges!=2)return 1;
 if(TSDATTACH(&k,core+TSF_KPOOL_REAL,TSF_KPOOL_REAL,TSF_KPOOL_BYTES,r.kbytes,r.kasce)||
 TSDATTACH(&u,core+TSF_UPOOL_REAL,TSF_UPOOL_REAL,TSF_UPOOL_BYTES,r.ubytes,r.uasce))return 1;
 if(TSF_CORE_BYTES!=0x01000000U||TSF_REAL_BYTES!=0x20000000U||
    TSF_SERVICE_BYTES!=0x00200000U||TSF_KSTACK_PAGES!=128U||
    r.kbytes>0x00300000U||r.ubytes>0x00300000U)return 1;
 va.hi=0;va.lo=0x2000;
 if(TSDLOOKUP(&k,va,&pa)||pa.hi||pa.lo!=0x2000)return 1;
 for(va.lo=0;va.lo<0x1000000;va.lo+=4096)if(TSDLOOKUP(&u,va,&pa)!=TSD_MISSING)return 1;
 va.hi=0x01000000;va.lo=0;
 if(TSDLOOKUP(&k,va,&pa)||TSDLOOKUP(&u,va,&pa)!=TSD_MISSING)return 1;
 va.hi=0;va.lo=0x02000000U+(TSF_SERVICE_PAGES-1U)*4096U;
 if(TSDLOOKUP(&k,va,&pa)||pa.lo!=TSF_SERVICE_REAL+TSF_SERVICE_BYTES-4096U||
    TSDLOOKUP(&u,va,&pa)!=TSD_MISSING)return 1;
 va.lo=0x03000000U+(TSF_KSTACK_PAGES-1U)*4096U;
 if(TSDLOOKUP(&k,va,&pa)||pa.lo!=TSF_KSTACK_EXT_REAL+TSF_KSTACK_EXT_BYTES-4096U||
    TSDLOOKUP(&u,va,&pa)!=TSD_MISSING)return 1;
 for(va.lo=0x02000000U;va.lo<0x02200000U;va.lo+=4096U)
  if(TSDLOOKUP(&k,va,&pa)||pa.hi||pa.lo!=0x00400000U+va.lo-0x02000000U||
     TSDLOOKUP(&u,va,&pa)!=TSD_MISSING)return 1;
 for(va.lo=0x03004000U;va.lo<0x03080000U;va.lo+=4096U)
  if(TSDLOOKUP(&k,va,&pa)||pa.hi||pa.lo!=0x00600000U+va.lo-0x03004000U||
     TSDLOOKUP(&u,va,&pa)!=TSD_MISSING)return 1;
 va.lo=0x02800000U;
 if(TSDLOOKUP(&k,va,&pa)||pa.lo!=0xf000U||TSDLOOKUP(&u,va,&pa)!=TSD_MISSING)return 1;
 va.lo=0x05300000U;
 if(TSDLOOKUP(&k,va,&pa)||pa.lo!=0xb00000U||TSDLOOKUP(&u,va,&pa)!=TSD_MISSING)return 1;
 va.lo=0x055ff000U;
 if(TSDLOOKUP(&k,va,&pa)||pa.lo!=0xdff000U)return 1;
 va.lo=0x27fff000U;
 if(TSDLOOKUP(&k,va,&pa)||pa.lo!=0x1ffff000U||TSDLOOKUP(&u,va,&pa)!=TSD_MISSING)return 1;
 if(TSFBUILD(core,0x400000U,0xb00000U,&r,0,0)==0||
    TSFBUILD(core,0x600000U,0xb00000U,&r,0,0)==0||
    TSFBUILD(core,0x800000U,0x900000U,&r,0,0)==0)return 1;
 free(core);puts("Normal boot: empty low U, protected high K and actual bootstrap purges pass");return 0;
}
