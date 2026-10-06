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
#include "twospace_cms.h"

static unsigned int user_rights(unsigned int frame, void *unused);
static TSDSTATE u_tables;
static TSMSTATE storage;
static TSCSTATE channel;
static TSCSTATE console_channel;
static unsigned int storage_ready;
static unsigned int console_ssid;
static unsigned int console_read_phase;
static unsigned int console_read_count;
static unsigned int cms31_loaded;
static unsigned int cms24_loaded;
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
    if (TSDATTACH(&u_tables,(unsigned char *)TSF_UPOOL_VA,0x180000U,
                  TSF_UPOOL_BYTES,
                  *(volatile const unsigned int *)0x4098U,
                  *(volatile const unsigned int *)0x400cU) != TSD_OK ||
        TSDLIVE(&u_tables,purge,0) != TSD_OK ||
        TSMINIT(&storage,&u_tables,(unsigned char *)TSF_KAPERTURE_VA,
                TSF_REAL_BYTES,TSF_CORE_BYTES,set_key,0) != TSM_OK ||
        TSCINIT(&channel,(unsigned char *)TSF_KAPERTURE_VA,
                TSF_REAL_BYTES,TSF_CHANNEL_REAL) != TSC_OK ||
        TSCINIT(&console_channel,(unsigned char *)TSF_KAPERTURE_VA,
                TSF_REAL_BYTES,TSF_CONSOLE_REAL) != TSC_OK)
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
    at.hi=0U; at.lo=0U;
    if (TSMRESERVE(&storage,6U,24U,at,0x20000U) != TSP_OK)
        return -1;
    at.lo=0x00f00000U;
    if (TSMRESERVE(&storage,7U,24U,at,0x100000U) != TSP_OK)
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

static unsigned int cms24_header_service(const TSGREQUEST *request)
{
    static const unsigned char name[] = {
        0xc3U,0xd4U,0xe2U,0xf2U,0xf4U,0x4bU,0xd9U,0xe7U,
        0xe5U,0xd4U
    };
    const unsigned char *first;
    TSKEXTENT extent;
    TSHINFO info;
    int result, count;
    if (request->length || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    if (*(volatile const unsigned int *)0x40bcU == 0U) return 0xfffffffbU;
    result=TSKFIND(channel_record,&channel,name,sizeof name,&extent);
    if (result == TSK_ABSENT) return 4U;
    if (result != TSK_OK || extent.record_format != 0x80U ||
        extent.block_length != 18452U || extent.logical_length != 18452U)
        return 12U;
    count=channel_record(&channel,extent.start_cylinder,
                         extent.start_head,1U,18452U,&first);
    if (count != 18452 ||
        TSHHEADER(first,(unsigned int)count,24U,&info) != TSH_OK)
        return 12U;
    *(volatile unsigned int *)0x40c8U=info.origin;
    *(volatile unsigned int *)0x40ccU=info.end;
    *(volatile unsigned int *)0x40d0U=info.entry;
    *(volatile unsigned int *)0x40d4U=info.module_bytes;
    return 0U;
}

/* Temporary K-only stage backing is sized from the checked first block.
 * It is reserved against all U allocations until validation/materialization
 * ends; no fixed high-real hole is consumed between service calls. */
#define CMS_STAGE_CAPACITY 0x900000U
#define CMS_STAGE_OWNER 0x434d5332U

static unsigned int cms_stage_read(const unsigned char *name,
                                   unsigned int name_bytes,
                                   unsigned int profile,
                                   unsigned char **stage_out,
                                   unsigned int *stage_real,
                                   TSHINFO *info,
                                   unsigned int *block_count)
{
    const unsigned char *record;
    unsigned char *stage;
    TSKEXTENT extent;
    unsigned int cylinder, head, number, blocks, index, offset, total, span;
    unsigned int real;
    int found, count;
    found=TSKFIND(channel_record,&channel,name,name_bytes,&extent);
    if (found == TSK_ABSENT) return 4U;
    if (found != TSK_OK || extent.record_format != 0x80U ||
        extent.block_length != 18452U || extent.logical_length != 18452U)
        return 12U;
    cylinder=extent.start_cylinder; head=extent.start_head; number=1U;
    count=channel_record(&channel,cylinder,head,number,18452U,&record);
    if (count != 18452 || TSHHEADER(record,18452U,profile,info) != TSH_OK)
        return 12U;
    total=64U+info->module_bytes;
    blocks=(total+18451U)/18452U;
    if (!blocks || blocks > CMS_STAGE_CAPACITY/18452U) return 12U;
    span=(blocks*18452U+4095U)&~4095U;
    if (TSRALLOC(&storage.real,CMS_STAGE_OWNER,span,
                 TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,&real) != TSR_OK)
        return 4U;
    stage=(unsigned char *)TSF_KAPERTURE_VA+real;
    for (index=0U; index<blocks; ++index) {
        if (!TSKWITHIN(&extent,cylinder,head)) goto fail;
        if (index) {
            count=channel_record(&channel,cylinder,head,number,18452U,&record);
            if (count != 18452) goto fail;
        }
        offset=index*18452U;
        { unsigned int i;
          for (i=0U; i<18452U; ++i) stage[offset+i]=record[i]; }
        ++number;
        if (number == 4U) {
            number=1U; ++head;
            if (head == 15U) { head=0U; ++cylinder; }
        }
    }
    if (TSHVALIDATE(stage,blocks*18452U,profile,info) != TSH_OK)
        goto fail;
    *stage_out=stage;
    *stage_real=real;
    *block_count=blocks;
    return 0U;
fail:
    return TSRRELEASE(&storage.real,CMS_STAGE_OWNER,real) == TSR_OK ?
           12U : 0xfffffff0U;
}

static unsigned int cms24_full_service(const TSGREQUEST *request)
{
    static const unsigned char name[] = {
        0xc3U,0xd4U,0xe2U,0xf2U,0xf4U,0x4bU,0xd9U,0xe7U,
        0xe5U,0xd4U
    };
    unsigned char *stage;
    TSHINFO info;
    unsigned int blocks, result, stage_real;
    if (request->length || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    if (*(volatile const unsigned int *)0x40bcU == 0U) return 0xfffffffbU;
    result=cms_stage_read(name,sizeof name,24U,&stage,&stage_real,
                          &info,&blocks);
    if (result == 0U) {
        *(volatile unsigned int *)0x40d8U=CMS_STAGE_OWNER;
        *(volatile unsigned int *)0x40dcU=blocks;
    }
    if (result == 0U &&
        TSRRELEASE(&storage.real,CMS_STAGE_OWNER,stage_real) != TSR_OK)
        return 12U;
    return result;
}

/* The diagnostic fixture booted with two static U24 pages. Its U64 caller
 * has finished using them before this service replaces that interval with
 * the checked fixed-origin image. A normal loader starts with empty U. */
static unsigned int cms24_map_service(const TSGREQUEST *request)
{
    static const unsigned char name[] = {
        0xc3U,0xd4U,0xe2U,0xf2U,0xf4U,0x4bU,0xd9U,0xe7U,
        0xe5U,0xd4U
    };
    unsigned char *stage;
    TSPADDR first, second, expected_first, expected_second, old, placed;
    TSPADDR guard, stack, placed_stack;
    TSHINFO info;
    unsigned int blocks, stage_real, result, i, real=0U, stack_real=0U, entry;
    int allocation;
    if (request->length || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    if (*(volatile const unsigned int *)0x40bcU == 0U) return 0xfffffffbU;
    result=cms_stage_read(name,sizeof name,24U,&stage,&stage_real,
                          &info,&blocks);
    if (result != 0U) return result;
    first.hi=second.hi=expected_first.hi=expected_second.hi=0U;
    first.lo=0x20000U; second.lo=0x21000U;
    expected_first.lo=0x5000U; expected_second.lo=0x7000U;
    if (TSDLOOKUP(&u_tables,first,&old) != TSD_OK ||
        old.hi || old.lo != expected_first.lo ||
        TSDLOOKUP(&u_tables,second,&old) != TSD_OK ||
        old.hi || old.lo != expected_second.lo) {
        result=12U; goto release;
    }
    if (TSDUNMAP(&u_tables,first,&old) != TSD_OK ||
        old.lo != expected_first.lo) { result=12U; goto release; }
    if (TSDUNMAP(&u_tables,second,&old) != TSD_OK ||
        old.lo != expected_second.lo) {
        if (TSDMAP(&u_tables,first,expected_first) != TSD_OK)
            result=0xfffffff0U;
        else result=12U;
        goto release;
    }
    if (TSPRELS(&storage.virtuals,0x80000001U) != TSP_OK) {
        result=12U; goto restore_pages;
    }
    allocation=TSMALLOC(&storage,5U,24U,first,first,
                         info.image_bytes,1,&placed);
    if (allocation != TSM_OK) {
        result=allocation == TSM_NOMEM ? 4U : 12U;
        goto restore_reservation;
    }
    for (i=0U; i<TSM_ALLOCS; ++i)
        if (storage.allocations[i].handle &&
            storage.allocations[i].task == 5U &&
            storage.allocations[i].address.hi == placed.hi &&
            storage.allocations[i].address.lo == placed.lo &&
            storage.allocations[i].bytes >= info.image_bytes) {
            real=storage.allocations[i].real;
            break;
        }
    if (!real || TSHIMAGE(stage,blocks*18452U,24U,placed.lo,
                          (unsigned char *)TSF_KAPERTURE_VA+real,
                          info.image_bytes,&entry) != TSH_OK) {
        result=12U; goto free_image;
    }
    guard.hi=stack.hi=0U;
    guard.lo=0x00f00000U; stack.lo=0x00f01000U;
    if (TSPRELS(&storage.virtuals,0x80000007U) != TSP_OK) {
        result=0xfffffff0U; goto free_image;
    }
    if (TSMRESERVE(&storage,7U,24U,guard,4096U) != TSP_OK) {
        result=0xfffffff0U; goto restore_stack_reservation;
    }
    allocation=TSMALLOC(&storage,5U,24U,stack,stack,0x000ff000U,
                         1,&placed_stack);
    if (allocation != TSM_OK) {
        result=allocation == TSM_NOMEM ? 4U : 12U;
        goto restore_guard;
    }
    for (i=0U; i<TSM_ALLOCS; ++i)
        if (storage.allocations[i].handle &&
            storage.allocations[i].task == 5U &&
            storage.allocations[i].address.hi == placed_stack.hi &&
            storage.allocations[i].address.lo == placed_stack.lo &&
            storage.allocations[i].bytes == 0x000ff000U) {
            stack_real=storage.allocations[i].real;
            break;
        }
    if (!stack_real) { result=0xfffffff0U; goto free_stack; }
    *(volatile unsigned int *)0x4100U=real;
    *(volatile unsigned int *)0x4104U=entry;
    *(volatile unsigned int *)0x4108U=info.image_bytes;
    *(volatile unsigned int *)0x410cU=TSMLOWFREE(&storage);
    *(volatile unsigned int *)0x4114U=stack_real;
    *(volatile unsigned int *)0x4118U=0x000ff000U;
    *(volatile unsigned int *)0x4098U=u_tables.used;
    cms24_loaded=1U;
    result=0U; goto release;
free_stack:
    if (TSMFREE(&storage,5U,placed_stack) != TSM_OK)
        { result=0xfffffff0U; goto release; }
restore_guard:
    if (TSPRELS(&storage.virtuals,0x80000007U) != TSP_OK)
        { result=0xfffffff0U; goto release; }
restore_stack_reservation:
    if (TSMRESERVE(&storage,7U,24U,guard,0x100000U) != TSP_OK)
        { result=0xfffffff0U; goto release; }
free_image:
    if (TSMFREE(&storage,5U,placed) != TSM_OK)
        { result=0xfffffff0U; goto release; }
restore_reservation:
    if (TSMRESERVE(&storage,1U,24U,first,0x2000U) != TSP_OK)
        { result=0xfffffff0U; goto release; }
restore_pages:
    if (TSDMAP(&u_tables,first,expected_first) != TSD_OK ||
        TSDMAP(&u_tables,second,expected_second) != TSD_OK)
        result=0xfffffff0U;
release:
    if (TSRRELEASE(&storage.real,CMS_STAGE_OWNER,stage_real) != TSR_OK)
        return 0xfffffff0U;
    return result;
}

static unsigned int cms31_map_service(const TSGREQUEST *request)
{
    static const unsigned char name[] = {
        0xc3U,0xd4U,0xe2U,0xf3U,0xf1U,0x4bU,0xd9U,0xe7U,
        0xe5U,0xd4U
    };
    unsigned char *stage;
    TSPADDR base, placed;
    TSHINFO info;
    unsigned int blocks, result, i, real=0U, entry, stage_real;
    int allocation;
    if (request->length || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    if (*(volatile const unsigned int *)0x40bcU == 0U) return 0xfffffffbU;
    result=cms_stage_read(name,sizeof name,31U,&stage,&stage_real,
                          &info,&blocks);
    if (result != 0U) return result;
    base.hi=0U; base.lo=0x03000000U;
    allocation=TSMALLOC(&storage,4U,31U,base,base,info.image_bytes,1,&placed);
    if (allocation != TSM_OK) {
        result=allocation == TSM_NOMEM ? 4U : 12U;
        goto release;
    }
    for (i=0U; i<TSM_ALLOCS; ++i)
        if (storage.allocations[i].handle &&
            storage.allocations[i].task == 4U &&
            storage.allocations[i].address.hi == placed.hi &&
            storage.allocations[i].address.lo == placed.lo &&
            storage.allocations[i].bytes >= info.image_bytes) {
            real=storage.allocations[i].real;
            break;
        }
    if (!real || TSHIMAGE(stage,blocks*18452U,31U,placed.lo,
                          (unsigned char *)TSF_KAPERTURE_VA+real,
                          info.image_bytes,&entry) != TSH_OK) {
        result=TSMFREE(&storage,4U,placed) == TSM_OK ? 12U : 0xfffffff0U;
        goto release;
    }
    *(volatile unsigned int *)0x40e0U=real;
    *(volatile unsigned int *)0x40e4U=entry;
    *(volatile unsigned int *)0x40e8U=info.image_bytes;
    *(volatile unsigned int *)0x40ecU=blocks;
    *(volatile unsigned int *)0x4098U=u_tables.used;
    cms31_loaded=1U;
release:
    if (TSRRELEASE(&storage.real,CMS_STAGE_OWNER,stage_real) != TSR_OK)
        return 12U;
    return result;
}

static unsigned int cms31_overlay_push(const TSGREQUEST *request)
{
    /* GNU s390 -m64 -march=z900: SVC 217; LGHI R15,0x3456; BR R14.
       The child runs AMODE31 in U and returns via its link register. */
    static const unsigned char child[] =
        {0x0aU,0xd9U,0xa7U,0xf9U,0x34U,0x56U,0x07U,0xfeU};
    TSPADDR base;
    int result;
    if (request->length || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    if (!cms31_loaded) return 4U;
    base.hi=0U; base.lo=0x03000000U;
    result=TSMOVERLAYPUSH(&storage,4U,base,child,sizeof child);
    if (result != TSM_OK) return result == TSM_NOMEM ? 4U : 12U;
    *(volatile unsigned int *)0x4098U=u_tables.used;
    return 0U;
}

static unsigned int cms31_overlay_pop(const TSGREQUEST *request)
{
    TSPADDR base;
    unsigned int returned;
    if (request->address.hi || request->address.lo || request->direction)
        return 8U;
    if (!cms31_loaded) return 4U;
    base.hi=0U; base.lo=0x03000000U;
    if (TSMOVERLAYPOP(&storage,4U,base,request->length,&returned)
        != TSM_OK) return 12U;
    *(volatile unsigned int *)0x4098U=u_tables.used;
    return returned;
}

static unsigned int cms31_child_probe(void)
{
    unsigned int mask_hi=*(volatile const unsigned int *)0x3080U;
    unsigned int mask_lo=*(volatile const unsigned int *)0x3084U;
    unsigned int mode=(mask_hi & 1U) ? 64U :
                      (mask_lo & 0x80000000U) ? 31U : 24U;
    *(volatile unsigned int *)0x40f4U=mode;
    return mode == 31U ? 0U : 8U;
}

static unsigned int cms24_overlay_push(const TSGREQUEST *request)
{
    /* GNU s390 z900: SAM24; SVC 219; LHI R3,-4096; MVI 0(R3),0x5a;
       LGHI R15,0x2468; SAM64; BR R14. The 24-bit address is 0xfff000,
       inside the backed stack's final page. */
    static const unsigned char child[] = {
        0x01U,0x0cU,0x0aU,0xdbU,0xa7U,0x38U,0xf0U,0x00U,
        0x92U,0x5aU,0x30U,0x00U,0xa7U,0xf9U,0x24U,0x68U,
        0x01U,0x0eU,0x07U,0xfeU
    };
    TSPADDR base;
    int result;
    if (request->length || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    if (!cms24_loaded) return 4U;
    base.hi=0U; base.lo=0x20000U;
    result=TSMOVERLAYPUSH(&storage,5U,base,child,sizeof child);
    if (result != TSM_OK) return result == TSM_NOMEM ? 4U : 12U;
    *(volatile unsigned int *)0x4098U=u_tables.used;
    return 0U;
}

static unsigned int cms24_overlay_pop(const TSGREQUEST *request)
{
    TSPADDR base;
    unsigned int returned;
    if (request->address.hi || request->address.lo || request->direction)
        return 8U;
    if (!cms24_loaded) return 4U;
    base.hi=0U; base.lo=0x20000U;
    if (TSMOVERLAYPOP(&storage,5U,base,request->length,&returned)
        != TSM_OK) return 12U;
    *(volatile unsigned int *)0x4098U=u_tables.used;
    return returned;
}

static unsigned int cms24_child_probe(void)
{
    unsigned int mask_hi=*(volatile const unsigned int *)0x3080U;
    unsigned int mask_lo=*(volatile const unsigned int *)0x3084U;
    unsigned int mode=(mask_hi & 1U) ? 64U :
                      (mask_lo & 0x80000000U) ? 31U : 24U;
    *(volatile unsigned int *)0x4110U=mode;
    return mode == 24U ? 0U : 8U;
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
        if (TSCDEV(0x10000U+i,TSCSCHIB(&console_channel)) == 9) {
            ssid=0x10000U+i;
            break;
        }
    }
    if (!ssid) return 0xfffffffbU;
    *(volatile unsigned int *)0x40c0U=ssid;
    screen=TSCDATA(&console_channel);
    for (i=0U; i<1773U; ++i) screen[i]=0x40U;
    screen[0]=0xc3U; screen[1]=0x11U; screen[2]=0x5dU;
    screen[3]=0x7fU; screen[4]=0x1dU; screen[5]=0xf0U;
    for (i=0U; i<sizeof message; ++i) screen[6U+i]=message[i];
    screen[1766]=0x1dU; screen[1767]=0U; screen[1768]=0x13U;
    screen[1769]=0x3cU; screen[1770]=0x5dU; screen[1771]=0x7fU;
    screen[1772]=0U;
    if (TSCBUILDCONSWRITE(&console_channel,1773U) != TSC_OK) return 20U;
    if (TSCENABL(ssid,TSCSCHIB(&console_channel)) != 0) return 23U;
    io_result=TSCIO(ssid,TSCORB(&console_channel),TSCIRB(&console_channel));
    *(volatile unsigned int *)0x40c4U=(unsigned int)io_result;
    if (io_result != 0) return 21U;
    if (TSCCHECKWRITE(&console_channel) != TSC_OK) return 22U;
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
        status=TSCPOLL(console_ssid,TSCIRB(&console_channel));
        if (status < 0) {
            console_read_phase=0U;
            return 12U;
        }
        if (status != 0) return 1U;
    }
    if (console_read_phase == 1U) {
        if ((TSCIRB(&console_channel)[8U] & 0x80U) == 0U) return 1U;
        if (TSCBUILDCONSREAD(&console_channel,252U) != TSC_OK ||
            TSCSTART(console_ssid,TSCORB(&console_channel),
                     TSCIRB(&console_channel)) != 0) {
            console_read_phase=0U;
            return 12U;
        }
        console_read_phase=2U;
        return 1U;
    }
    if (console_read_phase == 2U) {
        if (TSCCHECKCONSREAD(&console_channel,252U,&count) != TSC_OK) {
            /* Initial status can precede the actual READ MODIFIED data. */
            if (TSCIRB(&console_channel)[8U] == 0U &&
                TSCIRB(&console_channel)[9U] == 0U) return 1U;
            console_read_phase=0U;
            return 12U;
        }
        if (count == 3U && TSCDATA(&console_channel)[0] == 0x60U) {
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
    for (i=0U; i<count; ++i) bytes[4U+i]=TSCDATA(&console_channel)[i];
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

/* Diagnostic selector only: an absent subchannel must fail immediately,
 * rather than becoming an unbounded terminal poll or a false I/O success. */
static unsigned int absent_channel_probe(const TSGREQUEST *request)
{
    unsigned char *schib;
    if (request->length || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    if (TSCPOLL(0x0001ffffU,TSCIRB(&channel)) != -1) return 12U;
    schib=TSCSCHIB(&channel);
    schib[6U]=0x12U; schib[7U]=0x34U;
    if (TSCDEV(0x0001ffffU,schib) != 0) return 20U;
    if (TSCSTART(0x0001ffffU,TSCORB(&channel),TSCIRB(&channel)) != -1)
        return 24U;
    if (TSCIO(0x0001ffffU,TSCORB(&channel),TSCIRB(&channel)) != -2)
        return 16U;
    return 0U;
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

/* Internal U64 proof selector. It does not change the existing SVC 120
 * convention or claim an IBM high-storage ABI. */
static unsigned int high_storage_service(TSGREQUEST *request)
{
    TSPADDR minimum, maximum, base;
    unsigned int result;
    if (!(*(volatile const unsigned int *)0x3080U & 1U)) return 8U;
    if (request->address.hi == 0U && request->address.lo == 0U) {
        minimum.hi=maximum.hi=1U;
        minimum.lo=0x20000000U;
        maximum.lo=0x7fffffffU;
        result=TSMALLOC(&storage,1U,64U,minimum,maximum,
                        request->length,0,&base);
        if (result != TSM_OK) return result == TSM_NOMEM ? 4U : 8U;
        request->address=base;
    } else {
        if (request->address.hi != 1U ||
            request->address.lo < 0x20000000U ||
            request->address.lo > 0x7fffffffU) return 8U;
        result=TSMFREE(&storage,1U,request->address);
        if (result != TSM_OK) return 8U;
    }
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
        return request->svc == 120U || request->svc == 223U ?
               8U : 0xfffffffdU;
    if (request->svc == 120U) return storage_service(request);
    if (request->svc == 223U) return high_storage_service(request);
    if (request->svc == 206U) return volume_service(request);
    if (request->svc == 207U) return dataset_service(request);
    if (request->svc == 208U) return terminal_service(request);
    if (request->svc == 209U) return terminal_read_start(request);
    if (request->svc == 210U) return terminal_read_poll(request);
    if (request->svc == 212U) return cms24_header_service(request);
    if (request->svc == 213U) return cms24_full_service(request);
    if (request->svc == 214U) return cms31_map_service(request);
    if (request->svc == 215U) return cms31_overlay_push(request);
    if (request->svc == 216U) return cms31_overlay_pop(request);
    if (request->svc == 217U) return cms31_child_probe();
    if (request->svc == 218U) return cms24_map_service(request);
    if (request->svc == 219U) return cms24_child_probe();
    if (request->svc == 220U) return cms24_overlay_push(request);
    if (request->svc == 221U) return cms24_overlay_pop(request);
    if (request->svc == 222U) return absent_channel_probe(request);
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
