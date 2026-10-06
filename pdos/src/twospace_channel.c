/* SPDX-License-Identifier: MIT */
#include "twospace_channel.h"

static void put16(unsigned char *p, unsigned int value)
{ p[0]=(unsigned char)(value>>8); p[1]=(unsigned char)value; }
static void put32(unsigned char *p, unsigned int value)
{
    p[0]=(unsigned char)(value>>24); p[1]=(unsigned char)(value>>16);
    p[2]=(unsigned char)(value>>8); p[3]=(unsigned char)value;
}
static unsigned int get16(const unsigned char *p)
{ return ((unsigned int)p[0]<<8) | (unsigned int)p[1]; }
static unsigned int get32(const unsigned char *p)
{
    return ((unsigned int)p[0]<<24) | ((unsigned int)p[1]<<16) |
           ((unsigned int)p[2]<<8) | (unsigned int)p[3];
}
static void clear(unsigned char *p, unsigned int length)
{
    unsigned int i;
    for (i=0U; i<length; ++i) p[i]=0U;
}
static void ccw(unsigned char *p, unsigned int command, unsigned int flags,
                unsigned int count, unsigned int real)
{
    p[0]=(unsigned char)command; p[1]=(unsigned char)flags;
    put16(p+2U,count); put32(p+4U,real);
}

int TSCINIT(TSCSTATE *s, unsigned char *aperture,
            unsigned int real_bytes, unsigned int region_real)
{
    if (!s || !aperture || !real_bytes || (real_bytes & 4095U) ||
        (region_real & 4095U) || region_real >= 0x01000000U ||
        real_bytes < TSC_REGION_BYTES ||
        region_real > real_bytes-TSC_REGION_BYTES ||
        region_real > 0x01000000U-TSC_REGION_BYTES)
        return TSC_BAD;
    s->aperture=aperture;
    s->real_bytes=real_bytes;
    s->region_real=region_real;
    return TSC_OK;
}

int TSCBUILDREAD(TSCSTATE *s, unsigned int cylinder, unsigned int head,
                 unsigned int record, unsigned int command,
                 unsigned int capacity)
{
    unsigned char *base, *orb, *seek, *search, *chain;
    unsigned int real;
    if (!s || !s->aperture || cylinder >= 100U || head >= 15U ||
        !record || record > 255U ||
        (command != 0x06U && command != 0x0eU && command != 0x1eU) ||
        !capacity || capacity > TSC_MAX_RECORD)
        return TSC_BAD;
    real=s->region_real;
    base=s->aperture+real;
    clear(base,TSC_REGION_BYTES);
    orb=base+TSC_ORB_OFFSET;
    seek=base+TSC_SEEK_OFFSET;
    search=base+TSC_SEARCH_OFFSET;
    chain=base+TSC_CCW_OFFSET;
    /* Format-1 ORB and four 8-byte CCWs use real, not K virtual, addresses. */
    put32(orb+4U,0x0080ff00U);
    put32(orb+8U,real+TSC_CCW_OFFSET);
    put16(seek+2U,cylinder); put16(seek+4U,head);
    put16(search,cylinder); put16(search+2U,head);
    search[4]=(unsigned char)record;
    ccw(chain,0x07U,0x40U,6U,real+TSC_SEEK_OFFSET);
    ccw(chain+8U,0x31U,0x40U,5U,real+TSC_SEARCH_OFFSET);
    ccw(chain+16U,0x08U,0U,0U,real+TSC_CCW_OFFSET+8U);
    ccw(chain+24U,command,0x20U,capacity,real+TSC_DATA_OFFSET);
    return TSC_OK;
}

int TSCCHECKREAD(const TSCSTATE *s, unsigned int capacity,
                 unsigned int *transferred)
{
    const unsigned char *irb;
    unsigned int residual;
    if (!s || !s->aperture || !transferred ||
        !capacity || capacity > TSC_MAX_RECORD) return TSC_BAD;
    irb=s->aperture+s->region_real+TSC_IRB_OFFSET;
    residual=get16(irb+10U);
    if (irb[8U] != 0x0cU || irb[9U] != 0U || residual > capacity ||
        get32(irb+4U) != s->region_real+TSC_CCW_OFFSET+32U)
        return TSC_IO;
    *transferred=capacity-residual;
    return TSC_OK;
}

unsigned char *TSCDATA(const TSCSTATE *s)
{ return s && s->aperture ? s->aperture+s->region_real+TSC_DATA_OFFSET : 0; }
unsigned char *TSCORB(const TSCSTATE *s)
{ return s && s->aperture ? s->aperture+s->region_real+TSC_ORB_OFFSET : 0; }
unsigned char *TSCIRB(const TSCSTATE *s)
{ return s && s->aperture ? s->aperture+s->region_real+TSC_IRB_OFFSET : 0; }
