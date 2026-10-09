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
static TSMSTATE storage;static TSVSTACK native_invocations;static unsigned int storage_ready=1U,observation_scans;static volatile unsigned int output[8];
static int normal_boot(void){return 1;}
unsigned int TSMLOWFREE(const TSMSTATE *s){return s->virtuals.entry[0].owner?0U:0xee0000U;}
"""
main="""
int main(void){unsigned int i;storage.virtuals.entry[0].owner=1U;storage.virtuals.entry[0].first.lo=0x20000U;storage.virtuals.entry[0].last.lo=0xefffffU;service_observe();if(output[5]!=0U||output[6]!=0U)return 1;for(i=0U;i<1000U;++i)service_observe();if(observation_scans!=1U)return 3;native_invocations.depth=2U;service_observe();if(output[7]!=2U||observation_scans!=1U)return 4;storage.virtuals.entry[0].owner=0U;++storage.virtuals.generation;service_observe();if(output[5]!=0xee0000U||output[6]!=0U||observation_scans!=2U)return 2;return 0;}
"""
(out/"observations.c").write_text(preamble+body+main)
s=Path("pdos/src/twospace_console.inc").read_text()
a=s.index("static void tc_gap(unsigned int,unsigned int);");b=s.index("static void tc_gap(unsigned int token",a)
c=s.index("static void tc_other_status(");d=s.index("static unsigned int console_v1_read(",c)
(out/"console-functions.inc").write_text(s[a:b]+s[c:d])
provider=Path("pdos/src/twospace_io.inc").read_text();at=provider.index("static int channel_vector(")
(out/"channel-provider.inc").write_text(provider[:at])
(out/"channel-cache.inc").write_text(provider[at:provider.index("static int channel_missing(")])
(out/"console_output.c").write_text(Path("pdos/tests/two-space/io_endpoint.c").read_text())

# Apply the actual renderer's stream to an independent field-cell model.
# Ordinary data removes a field attribute at that cell, just as the device
# does. Full-width rows must preserve the field topology and cursor.
a=s.index("static unsigned int tc_render(void)");b=s.index("static unsigned int console_v1_init(void)",a)
preamble=r'''
#include <string.h>
#include "twospace_console.h"
#include "twospace_invocation.h"
static TTCCAP tc_cap;static TTCVIEW view,*tc_view=&view;static TSVSTACK native_invocations;
static unsigned char stream[TTC_MAX_STREAM],confirmed[TTC_MAX_CELLS],header[TTC_MAX_CELLS],footer[TTC_MAX_CELLS];
static unsigned char *tc_stream=stream,*tc_raw=confirmed,*tc_header_cells=header,*tc_footer_cells=footer;
static unsigned char tc_headers[TSV_MAX_DEPTH+1U][132];static unsigned int tc_header_lengths[TSV_MAX_DEPTH+1U];
static const unsigned char tc_default_header[]={0xc8U},tc_default_footer[]={0xc5U};
static unsigned int tc_ssid=1U,tc_display_known,tc_input_field,tc_input_end,console_channel,fail_output,cursor_orders;
static unsigned char attribute[TTC_MAX_CELLS],device_data[TTC_MAX_CELLS];
static unsigned int tc_output(unsigned int *channel,unsigned int ssid,unsigned int command,const unsigned char *p,unsigned int n)
{
    unsigned int i=1U,at=0U,cells=tc_cap.rows*tc_cap.columns;
    (void)channel;(void)ssid;
    if(fail_output)return 12U;
    if(command==13U){memset(attribute,0,sizeof attribute);memset(device_data,0,sizeof device_data);}
    else if(command!=1U)return 99U;
    while(i<n){
        unsigned int code=p[i++];
        if(code==0x11U){if(n-i<2U||TTCDECODE(&tc_cap,p+i,&at))return 98U;i+=2U;}
        else if(code==0x1dU){if(i==n)return 97U;attribute[at]=1U;device_data[at]=p[i++];at=(at+1U)%cells;}
        else if(code==0x13U)++cursor_orders;
        else{attribute[at]=0U;device_data[at]=(unsigned char)code;at=(at+1U)%cells;}
    }
    return 0U;
}
'''
main=r'''
int main(void)
{
    unsigned int model,i,body,foot,input,cells,saved_cursor;
    native_invocations.depth=1U;
    for(model=2U;model<=5U;++model){
        if(TTCCONFIG(&tc_cap,TSA_DEVICE_3270,9U,model,0U)||TTCVINIT(tc_view,&tc_cap))return 1;
        cells=tc_cap.rows*tc_cap.columns;body=tc_view->body_rows*tc_cap.columns;
        foot=(tc_view->entry_row-1U)*tc_cap.columns-1U;input=tc_view->entry_row*tc_cap.columns;
        memset(confirmed,0,sizeof confirmed);memset(view.cells,0xd8,sizeof view.cells);
        tc_display_known=fail_output=cursor_orders=0U;
        if(tc_render()||!attribute[foot]||!attribute[input])return 2;
        saved_cursor=cursor_orders;
        memset(view.cells,0xe7,sizeof view.cells);
        if(tc_render()||!attribute[foot]||!attribute[input]||cursor_orders!=saved_cursor)return 3;
        if(!attribute[cells-1U]||!attribute[tc_view->body_row*tc_cap.columns-1U])return 4;
        for(i=0U;i<body;++i)
            if(device_data[tc_view->body_row*tc_cap.columns+i]!=0xe7U)return 5;
        fail_output=1U;memset(view.cells,0xe9,sizeof view.cells);
        if(tc_render()!=12U||tc_display_known||confirmed[tc_view->body_row*tc_cap.columns]!=0xe7U)return 6;
        fail_output=0U;
        if(tc_render()||!attribute[foot]||!attribute[input]||!tc_display_known)return 7;
    }
    return 0;
}
'''
(out/"console_render.c").write_text(preamble+s[a:b]+main)
