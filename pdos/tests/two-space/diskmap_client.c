/* SPDX-License-Identifier: MIT; actual U client with deterministic service
 * responses, line/panel capture and allocation-failure injection. */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "twospace_ui.h"
#include "twospace_workbench.h"
#include "twospace_dasd.h"
static void *memory;static unsigned int allocation_bytes,allocations,releases,cylinders,screen_width;
static unsigned int service_calls,input_calls,fail_service,fail_panel,fail_input,fail_alloc,bad_summary,next_volume;
static unsigned int panel_closes,line_count;static char lines[24][132];
static void *test_malloc(size_t bytes)
{assert(!memory);if(fail_alloc)return 0;memory=malloc(bytes);assert(memory);allocation_bytes=(unsigned int)bytes;++allocations;return memory;}
static void test_free(void *p){assert(p&&p==memory);free(p);memory=0;++releases;}
#define malloc test_malloc
#define free test_free
#define main diskmap_main
#if defined(__APPLE__)
/* The target runtime supplies C89 sprintf; this host SDK deprecates its
 * declaration. Keep the actual target calls and all bounds/sanitizer checks. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
#include "../../src/apps/diskmap.c"
#if defined(__APPLE__)
#pragma clang diagnostic pop
#endif
#undef main
#undef malloc
#undef free
unsigned int TUIOPEN(const unsigned char *title,unsigned int n){assert(title&&n);return 0U;}
unsigned int TUICAPS(unsigned char *cap)
{memset(cap,0,64U);put(cap+8U,screen_width?TSA_DEVICE_3270:TSA_DEVICE_LINE);put(cap+16U,24U);put(cap+20U,screen_width);return 0U;}
int TWBLAYOUT(unsigned int height,unsigned int width,TWBPLAN *plan)
{assert(height==24U);memset(plan,0,sizeof *plan);plan->body_rows=width==78U?9U:12U;plan->body_width=width;plan->body=3U;return 0;}
unsigned int TUIIO(unsigned int bytes,void *request,unsigned int op)
{
    unsigned char *p=(unsigned char *)request;unsigned int total=cylinders*15U,capacity=word(p+12U);
    assert(bytes==64U&&op==TSA_IO_DASD_STATUS);++service_calls;if(fail_service==service_calls)return 20U;
    assert(word(p+8U)==(next_volume&&service_calls>2U?1U:0U));
    assert(word(p)==(capacity?1U:TDA_GEOMETRY_QUERY));
    if(capacity){
        assert(memory&&allocation_bytes==total&&capacity==total&&word(p+60U)==(unsigned int)(unsigned long)memory);
        memset(memory,0,total);memset(memory,TDA_RESERVED,45U);
        memset((unsigned char *)memory+total-15U,cylinders==3U?TDA_RESERVED|TDA_ALLOCATED:TDA_ALLOCATED,15U);
    }else assert(!memory);
    put(p+8U,2U);put(p+12U,0x1b9U);put(p+16U,cylinders);put(p+20U,15U);put(p+24U,bad_summary?TDA_MAX_TRACKS+1U:total);
    put(p+28U,45U);put(p+32U,cylinders==3U?0U:15U);put(p+36U,cylinders==3U?0U:total-60U);
    put(p+40U,cylinders==3U?0U:total-60U);put(p+44U,1U);memcpy(p+48U,"PDOS00",6U);return 0U;
}
unsigned int TUILINE(const unsigned char *p,unsigned int n)
{assert(!memory&&n<132U&&line_count<24U);memcpy(lines[line_count],p,n);lines[line_count][n]=0;++line_count;return 0U;}
unsigned int TUIPANEL(unsigned int op,unsigned char *p)
{
    assert(!memory);
    if(op==TSA_IO_PANEL_CLOSE){assert(word(p+8U)==17U);++panel_closes;return 0U;}
    if(fail_panel==op)return 12U;
    if(op==TSA_IO_PANEL_CREATE)put(p+8U,17U);
    if(op==TSA_IO_PANEL_TEXT)assert(word(p+52U)<=10U*132U);
    return 0U;
}
unsigned int TUIREAD(unsigned char *input,unsigned int capacity,unsigned int *n)
{assert(!memory&&capacity==256U);++input_calls;if(fail_input)return 12U;*n=1U;input[0]=next_volume&&input_calls==1U?'N':'Q';return 0U;}
static void setup(unsigned int c,unsigned int width)
{
    assert(!memory);cylinders=c;screen_width=width;
    allocations=releases=service_calls=input_calls=fail_service=fail_panel=fail_input=fail_alloc=bad_summary=next_volume=0U;
    panel_closes=line_count=0U;memset(lines,0,sizeof lines);
}
static void run(int expected)
{assert(diskmap_main(0,0)==expected&&!memory&&allocations==releases);}
int main(void)
{
    unsigned int i,first,last,previous;
    setup(100U,0U);run(0);assert(service_calls==2U&&allocations==1U&&line_count==9U);
    assert(!strcmp(lines[1],"100 cylinders x 15 tracks / 1 datasets"));
    assert(!strncmp(lines[5],"00-49 RRR",9U)&&!strncmp(lines[6],"50-99 ",6U)&&lines[6][55U]=='#');
    setup(65535U,40U);run(0);assert(line_count==10U&&panel_closes==1U&&allocation_bytes==983025U);
    assert(strstr(lines[1],"781 cyl/cell"));previous=0U;
    for(i=5U;i<8U;++i){assert(sscanf(lines[i],"%u-%u",&first,&last)==2&&first==previous&&last<cylinders);previous=last+1U;assert(strlen(lines[i])<=40U);}
    assert(previous==65535U&&lines[5][12U]=='R'&&lines[7][strlen(lines[7])-1U]=='+');
    setup(1024U,132U);run(0);assert(panel_closes==1U&&strstr(lines[1],"7 cyl/cell"));
    setup(1024U,78U);run(0);assert(line_count==9U&&panel_closes==1U&&strstr(lines[1],"11 cyl/cell"));
    assert(!strcmp(lines[8],"Enter refresh / N next volume / Q return"));
    setup(3U,0U);run(0);assert(!strcmp(lines[5],"00-02 RRR"));
    setup(100U,40U);next_volume=1U;run(0);assert(service_calls==4U&&allocations==2U&&panel_closes==1U);
    setup(100U,40U);fail_service=2U;run(20);assert(allocations==1U&&!panel_closes);
    setup(100U,40U);next_volume=1U;fail_service=4U;run(20);assert(allocations==2U&&panel_closes==1U);
    setup(100U,40U);fail_panel=TSA_IO_PANEL_TEXT;run(12);assert(panel_closes==1U);
    setup(100U,40U);fail_panel=TSA_IO_PANEL_CREATE;run(12);assert(!panel_closes);
    setup(100U,40U);fail_input=1U;run(12);assert(panel_closes==1U);
    setup(100U,0U);fail_alloc=1U;run(4);assert(!allocations);
    setup(100U,0U);bad_summary=1U;run(12);assert(!allocations);
    puts("DISKMAP: line/panel geometry, bounded aggregation and failure cleanup PASS");return 0;
}
