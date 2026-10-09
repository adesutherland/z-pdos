/* SPDX-License-Identifier: MIT */
#include "twospace_gate.h"

static int resolve(const TSGCONTEXT *gate, TSPADDR address,
                   unsigned int direction, unsigned int *real)
{
    TSPADDR translated;
    unsigned int rights;
    int status = TSDLOOKUP(gate->u_tables,address,&translated);
    if (status == TSD_MISSING) return TSG_UNMAPPED;
    if (status != TSD_OK || translated.hi || gate->real_bytes < 4096U ||
        (translated.lo & ~4095U) > gate->real_bytes - 4096U)
        return TSG_BAD;
    rights = gate->rights(translated.lo & ~4095U,gate->rights_context);
    if ((rights & direction) != direction) return TSG_DENIED;
    *real = translated.lo;
    return TSG_OK;
}

static int probe(const TSGCONTEXT *gate,const TSGREQUEST *request,unsigned int limit)
{
    TSPADDR at, last;
    unsigned int remaining, chunk, offset, real;
    int status;
    if (!gate || !request || !gate->u_tables ||
        !gate->real_aperture || !gate->rights ||
        !request->length || request->length > limit ||
        (request->direction != TSG_READ &&
         request->direction != TSG_WRITE)) return TSG_BAD;
    last.lo = request->address.lo + request->length - 1U;
    last.hi = request->address.hi +
              (last.lo < request->address.lo ? 1U : 0U);
    if (last.hi < request->address.hi) return TSG_BAD;
    at = request->address;
    remaining = request->length;
    while (remaining) {
        status = resolve(gate,at,request->direction,&real);
        if (status != TSG_OK) return status;
        offset = at.lo & 4095U;
        chunk = 4096U - offset;
        if (chunk > remaining) chunk = remaining;
        remaining -= chunk;
        if (remaining) {
            at.lo += chunk;
            if (at.lo == 0U) ++at.hi;
        }
    }
    return TSG_OK;
}

static int copy(const TSGCONTEXT *gate,const TSGREQUEST *request,
                   unsigned char *buffer,unsigned int capacity,unsigned int limit)
{
    TSPADDR at;
    unsigned int remaining, chunk, offset, real, i;
    int status;
    if (!buffer || !request || request->length > capacity) return TSG_BAD;
    status=probe(gate,request,limit);
    if (status != TSG_OK) return status;
    at = request->address;
    remaining = request->length;
    offset = 0U;
    while (remaining) {
        status = resolve(gate,at,request->direction,&real);
        if (status != TSG_OK) return status;
        chunk = 4096U - (at.lo & 4095U);
        if (chunk > remaining) chunk = remaining;
        for (i = 0U; i < chunk; ++i) {
            if (request->direction == TSG_READ)
                buffer[offset+i] = gate->real_aperture[real+i];
            else gate->real_aperture[real+i] = buffer[offset+i];
        }
        remaining -= chunk;
        offset += chunk;
        if (remaining) {
            at.lo += chunk;
            if (at.lo == 0U) ++at.hi;
        }
    }
    return TSG_OK;
}
int TSGPROBE(const TSGCONTEXT *gate,const TSGREQUEST *request)
{return probe(gate,request,TSG_MAX_COPY);}
int TSGCOPY(const TSGCONTEXT *gate,const TSGREQUEST *request,
              unsigned char *buffer,unsigned int capacity)
{return copy(gate,request,buffer,capacity,TSG_MAX_COPY);}
int TSGSPROB(const TSGCONTEXT *gate,const TSGREQUEST *request)
{return probe(gate,request,TSG_MAX_SPAN);}
int TSGSCOPY(const TSGCONTEXT *gate,const TSGREQUEST *request,
               unsigned char *buffer,unsigned int capacity)
{return copy(gate,request,buffer,capacity,TSG_MAX_SPAN);}
