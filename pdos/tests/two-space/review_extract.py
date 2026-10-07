#!/usr/bin/env python3
"""Extract the actual K formatting/observation functions for host bounds tests."""
from pathlib import Path
import sys
out=Path(sys.argv[1]);out.mkdir(exist_ok=False)
s=Path("pdos/src/twospace_media.inc").read_text()
a=s.index("static char media_print_buffer");b=s.index("static int media_getdevn",a)
preamble="""#include <stdarg.h>
#include <string.h>
static char observed[256];
static unsigned int observed_bytes;
static int media_digit(int c){return c>='0'&&c<='9';}
static unsigned int console_v1_line(const unsigned char *p,unsigned int n){memcpy(observed,p,n);observed_bytes=n;return 0U;}
"""
main=r"""
int main(void){char name[44];memset(name,'X',sizeof name);media_printf("%.44s\n",name);if(observed_bytes!=44U||memcmp(observed,name,44U))return 1;media_printf("%lu\n",123456789UL);if(observed_bytes!=9U||memcmp(observed,"123456789",9U))return 2;return 0;}
"""
(out/"format.c").write_text(preamble+s[a:b]+main)
s=Path("pdos/src/twospace_service.c").read_text();a=s.index("static unsigned int real_observed_peak");b=s.index("unsigned int pdosTwoSpaceService",a)
body=s[a:b].replace("(volatile unsigned int *)(TSF_KAPERTURE_VA+0x3c800U)","output")
preamble="""#include "twospace_memory.h"
#include "twospace_invocation.h"
static TSMSTATE storage;static TSVSTACK native_invocations;static unsigned int storage_ready=1U;static volatile unsigned int output[8];
static int normal_boot(void){return 1;}
unsigned int TSMLOWFREE(const TSMSTATE *s){return s->virtuals.entry[0].owner?0U:0xee0000U;}
"""
main="""
int main(void){storage.virtuals.entry[0].owner=1U;storage.virtuals.entry[0].first.lo=0x20000U;storage.virtuals.entry[0].last.lo=0xefffffU;service_observe();if(output[5]!=0U||output[6]!=0U)return 1;storage.virtuals.entry[0].owner=0U;service_observe();if(output[5]!=0xee0000U||output[6]!=0U)return 2;return 0;}
"""
(out/"observations.c").write_text(preamble+body+main)
