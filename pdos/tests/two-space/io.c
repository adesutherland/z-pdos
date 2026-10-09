/* SPDX-License-Identifier: MIT; independent channel event/ownership controls. */
#include "twospace_io.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef struct {int rc;unsigned int device,channel,cpa,residual,control;} EVENT;
typedef struct {
    EVENT event[12];unsigned int events,at,starts,polls,waits,clears,notices;
    int start[12],wait_error,clear_error,clear_missing,defer;
    unsigned int default_cpa;
} MOCK;
static void word(unsigned char *p,unsigned int n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static int start(unsigned int ssid,unsigned char *orb,unsigned char *irb,void *v)
{MOCK *m=(MOCK *)v;(void)ssid;(void)orb;(void)irb;return m->start[m->starts++];}
static int poll(unsigned int ssid,unsigned char *irb,void *v)
{
    MOCK *m=(MOCK *)v;EVENT *e;(void)ssid;++m->polls;
    if(m->at==m->events)return 1;
    e=&m->event[m->at++];if(e->rc)return e->rc;
    memset(irb,0,64U);irb[3U]=(unsigned char)e->control;irb[8U]=(unsigned char)e->device;irb[9U]=(unsigned char)e->channel;
    word(irb+4U,e->cpa?e->cpa:m->default_cpa);irb[10U]=(unsigned char)(e->residual>>8);irb[11U]=(unsigned char)e->residual;return 0;
}
static int wait_event(unsigned int ssid,unsigned char *irb,unsigned int idle,void *v)
{MOCK *m=(MOCK *)v;(void)ssid;(void)irb;(void)idle;++m->waits;return m->wait_error||m->waits>8U?-1:0;}
static int clear_event(unsigned int ssid,unsigned char *irb,void *v)
{MOCK *m=(MOCK *)v;(void)ssid;++m->clears;if(m->clear_error)return -1;memset(irb,0,64U);if(!m->clear_missing)irb[2U]=0x10U;return 0;}
static int notice(TSCSTATE *s,unsigned int ssid,const unsigned char *irb,void *v)
{MOCK *m=(MOCK *)v;(void)s;(void)ssid;(void)irb;++m->notices;return m->defer?TIO_DEFER:0;}
static void reset(MOCK *m,TIOOPS *o,unsigned int cpa)
{memset(m,0,sizeof *m);m->default_cpa=cpa;o->start=start;o->poll=poll;o->wait=wait_event;o->clear=clear_event;o->notice=notice;o->context=m;}
static void event(MOCK *m,int rc,unsigned int device,unsigned int control,unsigned int cpa,unsigned int residual)
{EVENT *e=&m->event[m->events++];e->rc=rc;e->device=device;e->control=control;e->cpa=cpa;e->residual=residual;e->channel=0U;}
int main(void)
{
    unsigned char *memory=(unsigned char *)calloc(4U,TSC_REGION_BYTES);TIOPOOL p;TSCSTATE disk,tape,console,alias;TIOOPS o;MOCK m;
    unsigned int a,b,got;
    assert(memory);TIOINIT(&p);
    assert(!TSCINIT(&disk,memory,4U*TSC_REGION_BYTES,0U));assert(!TSCINIT(&tape,memory,4U*TSC_REGION_BYTES,TSC_REGION_BYTES));assert(!TSCINIT(&console,memory,4U*TSC_REGION_BYTES,2U*TSC_REGION_BYTES));
    assert(!TIOBIND(&p,&disk)&&!TIOBIND(&p,&tape)&&!TIOBIND(&p,&console));alias=disk;assert(TIOBIND(&p,&alias)==TIO_BAD);
    reset(&m,&o,0x220U);assert(!TSCBUILDREAD(&disk,0U,0U,3U,14U,80U));event(&m,0,12U,7U,0U,0U);
    assert(!TIOEXEC(&p,&disk,0x10001U,42U,&o)&&!TSCCHECKREAD(&disk,80U,&got)&&got==80U&&m.starts==1U&&!m.clears&&!disk.occupied);
    reset(&m,&o,TSC_REGION_BYTES+0x208U);assert(!TSCBUILDTAPE(&tape,2U,80U));event(&m,0,13U,7U,0U,80U);
    assert(!TIOEXEC(&p,&tape,0x10002U,42U,&o)&&!TSCCHECKTAPE(&tape,2U,80U,&got)&&got==0U);
    reset(&m,&o,2U*TSC_REGION_BYTES+0x208U);assert(!TSCBUILDCONSWRITE(&console,80U));m.start[0]=-2;event(&m,0,128U,17U,0U,0U);event(&m,0,140U,7U,0U,0U);
    assert(!TIOEXEC(&p,&console,0x10003U,42U,&o)&&m.starts==2U&&m.notices==2U&&!TSCCHECKOUT(&console));
    reset(&m,&o,0x220U);assert(!TSCBUILDREAD(&disk,0U,0U,3U,14U,80U));event(&m,0,4U,5U,0U,7U);event(&m,1,0U,0U,0U,0U);event(&m,0,8U,3U,0xffffffffU,0U);
    assert(!TIOEXEC(&p,&disk,0x10001U,42U,&o)&&m.waits==2U&&!TSCCHECKREAD(&disk,80U,&got)&&got==73U);
    reset(&m,&o,0x220U);assert(!TSCBUILDREAD(&disk,0U,0U,3U,14U,80U));event(&m,0,4U,5U,0U,7U);event(&m,0,128U,17U,0xffffffffU,0U);event(&m,0,8U,3U,0xffffffffU,0U);
    assert(!TIOEXEC(&p,&disk,0x10001U,42U,&o)&&m.notices==1U&&!TSCCHECKREAD(&disk,80U,&got)&&got==73U);
    reset(&m,&o,0x220U);assert(!TSCBUILDREAD(&disk,0U,0U,3U,14U,80U));m.start[0]=-2;event(&m,0,12U,7U,0U,0U);m.wait_error=1;
    assert(TIOEXEC(&p,&disk,0x10001U,42U,&o)==TIO_ERROR&&m.clears==1U&&m.starts==2U);
    reset(&m,&o,0x220U);assert(!TSCBUILDREAD(&disk,0U,0U,3U,14U,80U));assert(!TIOTAKE(&p,&disk,0x10001U,42U,&a));assert(!TIOSUB(&p,&disk,42U,a,&o));
    assert(TSCBUILDREAD(&disk,0U,0U,3U,14U,80U)==TSC_BAD&&TSCBUILDTAPE(&disk,2U,80U)==TSC_BAD&&TSCBUILDCONSWRITE(&disk,80U)==TSC_BAD);
    assert(!TSCBUILDTAPE(&tape,2U,80U));assert(TIOTAKE(&p,&tape,0x10001U,77U,&b)==TIO_BUSY);
    assert(TIOPOLL(&p,&disk,77U,a,&o)==TIO_STALE&&!m.polls);
    m.clear_error=1;assert(TIOCANCEL(&p,&disk,42U,a,&o)==TIO_ERROR&&disk.quarantined&&disk.occupied);
    assert(TIOTAKE(&p,&tape,0x10001U,77U,&b)==TIO_BUSY);m.clear_error=0;assert(!TIOCANCEL(&p,&disk,42U,a,&o)&&!disk.quarantined&&!disk.occupied);
    assert(!TSCBUILDREAD(&disk,0U,0U,3U,14U,80U));assert(!TIOTAKE(&p,&disk,0x10001U,77U,&b)&&b!=a);assert(!TIOSUB(&p,&disk,77U,b,&o));assert(TIOPOLL(&p,&disk,42U,a,&o)==TIO_STALE);
    m.clear_missing=1;assert(TIOCANCEL(&p,&disk,77U,b,&o)==TIO_ERROR&&disk.quarantined);m.clear_missing=0;assert(!TIOCANCEL(&p,&disk,77U,b,&o));
    reset(&m,&o,0x220U);assert(!TSCBUILDREAD(&disk,0U,0U,3U,14U,80U));event(&m,0,14U,7U,0U,0U);
    assert(TIOEXEC(&p,&disk,0x10001U,42U,&o)==TIO_ERROR&&m.clears==1U&&TIOSTATE(&p,&disk)->status[8U]==14U);
    reset(&m,&o,0x220U);assert(!TSCBUILDREAD(&disk,0U,0U,3U,14U,80U));event(&m,0,12U,7U,0x228U,0U);
    assert(TIOEXEC(&p,&disk,0x10001U,42U,&o)==TIO_ERROR&&m.clears==1U);
    reset(&m,&o,2U*TSC_REGION_BYTES+0x208U);assert(!TSCBUILDCONSWRITE(&console,80U));m.start[0]=-2;m.defer=1;event(&m,0,128U,17U,0U,0U);
    assert(TIOEXEC(&p,&console,0x10003U,42U,&o)==TIO_DEFER&&m.starts==1U&&!console.occupied);
    assert(!TIOQUIESCE(&p,&console,0x10003U,42U,&o));p.next_ticket=0xffffffffU;
    assert(!TSCBUILDCONSWRITE(&console,80U));assert(TIOTAKE(&p,&console,0x10003U,42U,&a)==TIO_BAD&&!console.occupied);
    free(memory);puts("shared I/O: disk/tape/console, split/combined status, stale tickets, cancellation, quarantine and bounded ownership pass");return 0;
}
