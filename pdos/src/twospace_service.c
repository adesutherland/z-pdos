/* SPDX-License-Identifier: MIT
 * Classic C31 endpoint. The nucleus passes only a descriptor in K storage;
 * translation and copying happen through K-owned table and real apertures.
 * SVC numbers select distinct CMS and TSO personalities.
 */
#include "twospace_gate.h"
#include "twospace_fixture.h"

static unsigned int user_rights(unsigned int frame, void *unused);

unsigned int pdosTwoSpaceService(const TSGREQUEST *request)
{
    TSDSTATE tables;
    TSGCONTEXT gate;
    unsigned char bytes[TSG_MAX_COPY];
    unsigned int value;
    int result;
    if (!request) return 0xffffffffU;
    if (request->svc != 1U && request->svc != 202U &&
        request->svc != 204U && request->svc != 205U)
        return 0xfffffffbU;
    if (TSDATTACH(&tables,(unsigned char *)TSF_UPOOL_VA,0x140000U,
                  TSF_POOL_BYTES,
                  *(volatile const unsigned int *)0x4098U,
                  *(volatile const unsigned int *)0x400cU) != TSD_OK)
        return 0xfffffffaU;
    gate.u_tables = &tables;
    gate.real_aperture = (unsigned char *)TSF_KAPERTURE_VA;
    gate.real_bytes = TSF_REAL_BYTES;
    gate.rights = user_rights;
    gate.rights_context = 0;
    if (request->direction == TSG_WRITE && request->length != 4U)
        return 0xfffffff9U;
    bytes[0]=0x55U; bytes[1]=0x66U; bytes[2]=0x77U; bytes[3]=0x88U;
    result = TSGCOPY(&gate,request,bytes,sizeof bytes);
    if (result != TSG_OK) return result == TSG_DENIED ? 0xfffffffcU :
                                 0xfffffffdU;
    if (request->direction == TSG_WRITE) return 0x77777777U;
    if (request->length != 4U) return 0xfffffff9U;
    value = ((unsigned int)bytes[0]<<24) | ((unsigned int)bytes[1]<<16) |
            ((unsigned int)bytes[2]<<8) | (unsigned int)bytes[3];
    if (value != 0xa1b2c3d4U) return 0xfffffffeU;
    if (request->svc == 202U) return 0x20202020U;
    if (request->svc == 204U) return 0x20420420U;
    if (request->svc == 205U) return 0x20520520U;
    return 0x2468ace0U;
}

static unsigned int user_rights(unsigned int frame, void *unused)
{
    (void)unused;
    if (frame == 0x7000U || frame == 0x12000U || frame == 0x13000U)
        return TSG_READ | TSG_WRITE;
    if (frame == 0x5000U || frame == 0x6000U || frame == 0x11000U)
        return TSG_READ;
    return 0U;
}
