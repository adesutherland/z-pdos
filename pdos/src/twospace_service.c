/* SPDX-License-Identifier: MIT
 * Classic C31 endpoint. The nucleus passes only a descriptor in K storage;
 * translation and copying happen through K-owned table and real apertures.
 * SVC numbers select distinct CMS and TSO personalities.
 */
#include "twospace_gate.h"
#include "twospace_fixture.h"
#include "twospace_memory.h"
#include "twospace_channel.h"
#include "twospace_dataset.h"

static unsigned int user_rights(unsigned int frame, void *unused);
static TSDSTATE u_tables;
static TSMSTATE storage;
static TSCSTATE channel;
static unsigned int storage_ready;
static unsigned int console_ssid;
static unsigned int console_read_phase;
static unsigned int console_read_count;
void TSFPURGE(void *unused);
void TSKEYSET(unsigned int real_page, unsigned int key);

static void purge(void *unused)
{
    (void)unused;
    ++*(volatile unsigned int *)0x40acU;
    TSFPURGE(0);
}
static void set_key(unsigned int real_page, unsigned int key, void *unused)
{ (void)unused; TSKEYSET(real_page,key); }

static int attach(void)
{
    TSPADDR at;
    if (storage_ready) return 0;
    if (TSDATTACH(&u_tables,(unsigned char *)TSF_UPOOL_VA,0x140000U,
                  TSF_POOL_BYTES,
                  *(volatile const unsigned int *)0x4098U,
                  *(volatile const unsigned int *)0x400cU) != TSD_OK ||
        TSDLIVE(&u_tables,purge,0) != TSD_OK ||
        TSMINIT(&storage,&u_tables,(unsigned char *)TSF_KAPERTURE_VA,
                TSF_REAL_BYTES,TSF_CORE_BYTES,set_key,0) != TSM_OK ||
        TSCINIT(&channel,(unsigned char *)TSF_KAPERTURE_VA,
                TSF_REAL_BYTES,TSF_CHANNEL_REAL) != TSC_OK)
        return -1;
    at.hi=0U; at.lo=0x20000U;
    if (TSMRESERVE(&storage,1U,24U,at,0x2000U) != TSP_OK)
        return -1;
    at.lo=0x02000000U;
    if (TSMRESERVE(&storage,2U,31U,at,0x1000U) != TSP_OK)
        return -1;
    at.hi=1U; at.lo=0x10000000U;
    if (TSMRESERVE(&storage,3U,64U,at,0x3000U) != TSP_OK)
        return -1;
    storage_ready=1U;
    return 0;
}

int TSCIO(unsigned int subchannel, unsigned char *orb,
          unsigned char *irb);
int TSCDEV(unsigned int subchannel, unsigned char *schib);
int TSCENABL(unsigned int subchannel, unsigned char *schib);
int TSCSTART(unsigned int subchannel, unsigned char *orb,
             unsigned char *irb);
int TSCPOLL(unsigned int subchannel, unsigned char *irb);

static int channel_record(void *context, unsigned int cylinder,
                          unsigned int head, unsigned int record,
                          unsigned int capacity, const unsigned char **data)
{
    TSCSTATE *current=(TSCSTATE *)context;
    unsigned int count;
    unsigned int subchannel=*(volatile const unsigned int *)0x40bcU;
    if (!subchannel || !data || !capacity || capacity > TSC_MAX_RECORD ||
        TSCBUILDREAD(current,cylinder,head,record,0x0eU,capacity) != TSC_OK ||
        TSCIO(subchannel,TSCORB(current),TSCIRB(current)) != 0 ||
        TSCCHECKREAD(current,capacity,&count) != TSC_OK)
        return -1;
    *data=TSCDATA(current);
    return (int)count;
}

/* This is a K-only post-handover channel proof. A dataset service can use
 * the same real-buffer path once its extent and record policy is installed. */
static unsigned int volume_service(const TSGREQUEST *request)
{
    const unsigned char *label;
    unsigned int count, subchannel;
    if (request->length || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    subchannel=*(volatile const unsigned int *)0x40bcU;
    if (!subchannel) return 0xfffffffbU;
    if (TSCBUILDREAD(&channel,0U,0U,3U,0x0eU,80U) != TSC_OK ||
        TSCIO(subchannel,TSCORB(&channel),TSCIRB(&channel)) != 0 ||
        TSCCHECKREAD(&channel,80U,&count) != TSC_OK || count < 24U)
        return 12U;
    label=TSCDATA(&channel);
    if (label[4] != 0xe5U || label[5] != 0xd6U ||
        label[6] != 0xd3U || label[7] != 0xf1U) return 8U;
    return 0U;
}

static unsigned int dataset_service(const TSGREQUEST *request)
{
    static const unsigned char name[] =
        {0xd2U,0xc3U,0xd6U,0xd9U,0xc5U,0x4bU,0xc2U,0xc9U,0xd5U};
    const unsigned char *first;
    TSKEXTENT extent;
    int result, count;
    if (request->length || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    if (*(volatile const unsigned int *)0x40bcU == 0U) return 0xfffffffbU;
    result=TSKFIND(channel_record,&channel,name,sizeof name,&extent);
    if (result != TSK_OK) return result == TSK_ABSENT ? 4U : 12U;
    if (extent.record_format != 0x80U ||
        extent.block_length != 18452U ||
        extent.logical_length != 18452U ||
        !TSKWITHIN(&extent,extent.start_cylinder,extent.start_head))
        return 8U;
    count=channel_record(&channel,extent.start_cylinder,
                         extent.start_head,1U,18452U,&first);
    if (count != 18452 || first[0] != 0x54U || first[1] != 0x53U ||
        first[2] != 0x50U || first[3] != 0x32U) return 12U;
    return 0U;
}

static unsigned int terminal_service(const TSGREQUEST *request)
{
    static const unsigned char message[] = {
        0xd2U,0x40U,0xe2U,0xc5U,0xd9U,0xe5U,0xc9U,0xc3U,0xc5U,
        0x40U,0xd9U,0xc5U,0xc1U,0xc4U,0xe8U
    };
    unsigned char *screen;
    unsigned int ssid, i;
    int io_result;
    if (request->length || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    if (*(volatile const unsigned int *)0x40bcU == 0U) return 0xfffffffbU;
    ssid=0U;
    for (i=0U; i<256U; ++i) {
        if (TSCDEV(0x10000U+i,TSCSCHIB(&channel)) == 9) {
            ssid=0x10000U+i;
            break;
        }
    }
    if (!ssid) return 0xfffffffbU;
    *(volatile unsigned int *)0x40c0U=ssid;
    screen=TSCDATA(&channel);
    for (i=0U; i<1773U; ++i) screen[i]=0x40U;
    screen[0]=0xc3U; screen[1]=0x11U; screen[2]=0x5dU;
    screen[3]=0x7fU; screen[4]=0x1dU; screen[5]=0xf0U;
    for (i=0U; i<sizeof message; ++i) screen[6U+i]=message[i];
    screen[1766]=0x1dU; screen[1767]=0U; screen[1768]=0x13U;
    screen[1769]=0x3cU; screen[1770]=0x5dU; screen[1771]=0x7fU;
    screen[1772]=0U;
    if (TSCBUILDCONSWRITE(&channel,1773U) != TSC_OK) return 20U;
    if (TSCENABL(ssid,TSCSCHIB(&channel)) != 0) return 23U;
    io_result=TSCIO(ssid,TSCORB(&channel),TSCIRB(&channel));
    *(volatile unsigned int *)0x40c4U=(unsigned int)io_result;
    if (io_result != 0) return 21U;
    if (TSCCHECKWRITE(&channel) != TSC_OK) return 22U;
    console_ssid=ssid;
    return 0U;
}

static unsigned int terminal_read_start(const TSGREQUEST *request)
{
    if (request->length || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    if (!console_ssid) return 0xfffffffbU;
    if (console_read_phase) return 4U;
    /* A 3270 READ MODIFIED issued before an AID returns NoAID with no
       modified fields. Wait for attention, then submit the read CCW. */
    console_read_phase=1U;
    return 0U;
}

static unsigned int terminal_read_poll(const TSGREQUEST *request)
{
    TSGCONTEXT gate;
    TSGREQUEST output;
    unsigned char bytes[TSG_MAX_COPY];
    unsigned int count, i;
    int status;
    if (request->length != TSG_MAX_COPY ||
        request->direction != TSG_WRITE) return 8U;
    if (!console_read_phase) return 0xfffffffbU;
    if (console_read_phase != 3U) {
        status=TSCPOLL(console_ssid,TSCIRB(&channel));
        if (status != 0) return 1U;
    }
    if (console_read_phase == 1U) {
        if ((TSCIRB(&channel)[8U] & 0x80U) == 0U) return 1U;
        if (TSCBUILDCONSREAD(&channel,252U) != TSC_OK ||
            TSCSTART(console_ssid,TSCORB(&channel),TSCIRB(&channel)) != 0) {
            console_read_phase=0U;
            return 12U;
        }
        console_read_phase=2U;
        return 1U;
    }
    if (console_read_phase == 2U) {
        if (TSCCHECKCONSREAD(&channel,252U,&count) != TSC_OK) {
            /* Initial status can precede the actual READ MODIFIED data. */
            if (TSCIRB(&channel)[8U] == 0U &&
                TSCIRB(&channel)[9U] == 0U) return 1U;
            console_read_phase=0U;
            return 12U;
        }
        if (count == 3U && TSCDATA(&channel)[0] == 0x60U) {
            console_read_phase=1U;
            return 1U;
        }
        if (count < 3U || count > 252U) {
            console_read_phase=0U;
            return 12U;
        }
        console_read_count=count;
        console_read_phase=3U;
    }
    count=console_read_count;
    for (i=0U; i<TSG_MAX_COPY; ++i) bytes[i]=0U;
    bytes[0]=(unsigned char)(count>>24);
    bytes[1]=(unsigned char)(count>>16);
    bytes[2]=(unsigned char)(count>>8);
    bytes[3]=(unsigned char)count;
    for (i=0U; i<count; ++i) bytes[4U+i]=TSCDATA(&channel)[i];
    gate.u_tables=&u_tables;
    gate.real_aperture=(unsigned char *)TSF_KAPERTURE_VA;
    gate.real_bytes=TSF_REAL_BYTES;
    gate.rights=user_rights;
    gate.rights_context=0;
    output=*request;
    status=TSGCOPY(&gate,&output,bytes,sizeof bytes);
    if (status == TSG_OK) console_read_phase=0U;
    return status == TSG_OK ? 0U :
           status == TSG_DENIED ? 0xfffffffcU : 0xfffffffdU;
}

/* The current fixture's SVC 120 path uses the active conditional GETMAIN
 * register convention: R0 length, R1 zero to allocate or base to free, R15
 * X'10' below-line or X'30' above-line. The old PSW belongs to K's saved
 * frame, never to caller-supplied memory. Failure has no low-memory fallback.
 */
static unsigned int storage_service(TSGREQUEST *request)
{
    unsigned int mask_hi, mask_lo, flags, mode, storage_mode, result;
    TSPADDR minimum, maximum, base;
    mask_hi=*(volatile const unsigned int *)0x3080U;
    mask_lo=*(volatile const unsigned int *)0x3084U;
    flags=*(volatile const unsigned int *)0x307cU;
    mode=(mask_hi & 1U) ? 64U : (mask_lo & 0x80000000U) ? 31U : 24U;
    if (request->address.hi == 0U && request->address.lo == 0U) {
        if ((flags & 0x30U) == 0x30U) {
            if (mode == 24U) return 4U;
            storage_mode=31U;
            minimum.hi=maximum.hi=0U;
            minimum.lo=0x02010000U; maximum.lo=0x7fffffffU;
        } else if ((flags & 0x30U) == 0x10U) {
            storage_mode=24U;
            minimum.hi=maximum.hi=0U;
            minimum.lo=0x20000U; maximum.lo=0x00ffffffU;
        } else return 8U;
        result=TSMALLOC(&storage,1U,storage_mode,minimum,maximum,
                        request->length,0,&base);
        if (result != TSM_OK) return result == TSM_NOMEM ? 4U : 8U;
        request->address=base;
        *(volatile unsigned int *)0x4098U=u_tables.used;
        return 0U;
    }
    result=TSMFREE(&storage,1U,request->address);
    if (result != TSM_OK) return 8U;
    *(volatile unsigned int *)0x4098U=u_tables.used;
    return 0U;
}

unsigned int pdosTwoSpaceService(TSGREQUEST *request)
{
    TSGCONTEXT gate;
    unsigned char bytes[TSG_MAX_COPY];
    unsigned int value;
    int result;
    if (!request) return 0xffffffffU;
    if (attach()) return 0xfffffffaU;
    /* The descriptor stores a 32-bit length for its bounded transfer and
       GETMAIN subset. Refuse any nonzero caller high half before dispatch. */
    if (*(volatile const unsigned int *)0x3000U != 0U)
        return request->svc == 120U ? 8U : 0xfffffffdU;
    if (request->svc == 120U) return storage_service(request);
    if (request->svc == 206U) return volume_service(request);
    if (request->svc == 207U) return dataset_service(request);
    if (request->svc == 208U) return terminal_service(request);
    if (request->svc == 209U) return terminal_read_start(request);
    if (request->svc == 210U) return terminal_read_poll(request);
    if (request->svc != 1U && request->svc != 202U &&
        request->svc != 204U && request->svc != 205U)
        return 0xfffffffbU;
    gate.u_tables = &u_tables;
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
    if (TSMRIGHTS(frame,&storage)) return TSG_READ | TSG_WRITE;
    if (frame == 0x7000U || frame == 0x12000U || frame == 0x13000U)
        return TSG_READ | TSG_WRITE;
    if (frame == 0x5000U || frame == 0x6000U || frame == 0x11000U)
        return TSG_READ;
    return 0U;
}
