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
#include "twospace_cmsfile.h"
#include "twospace_cmscursor.h"
#include "twospace_tso.h"
#include "twospace_invocation.h"

static unsigned int user_rights(unsigned int frame, void *unused);
static unsigned int cms_word(const unsigned char *p);
static void cms_put_word(unsigned char *p, unsigned int value);
static int native_clean(unsigned int token, const TSVRESOURCE *resource,
                        void *context);
static TSDSTATE u_tables;
static TSMSTATE storage;
static TSCSTATE channel;
static TSCSTATE console_channel;
static TSVSTACK native_invocations;
static unsigned int storage_ready;
static unsigned int console_ssid;
static unsigned int console_read_phase;
static unsigned int console_read_count;
static unsigned int console_read_owner;
static unsigned int console_read_io_handle;
static unsigned int console_next_io_handle;
static unsigned int cms31_loaded;
static unsigned int cms31_secondary_loaded;
static unsigned int cms24_loaded;
static unsigned int tso31_loaded;
static unsigned int tso64_loaded;
static unsigned int tso24_loaded;
static unsigned int cms31_lowcore_real;
static unsigned char cms_lowcore_template[4096U];
static unsigned int cms_lowcore_template_ready;
#define CMS_LOWCORE_SAVE_OWNER 0x434d5353U
#define CMS31_LINES_REAL 0x20000U
#define CMS24_EXTRA_LINES_REAL 0x26000U
typedef struct {
    TSPADDR address;
    unsigned int bytes;
    unsigned int handle;
} CMSHEAP;
/* Heap requests belong to the active CMS invocation, including nested calls. */
static CMSHEAP cms_heaps[TSV_MAX_DEPTH];
#define CMS_FILE_OWNER 0x434d5346U
#define TSO_STAGE_OWNER 0x54534f31U
#define TSO_IMAGE_OWNER 0x54534f32U
/* Each open input, including one reopened by a nested app, owns its cursor. */
static TSISTATE cms_inputs;
#define CMS_OUTPUT_SLOTS 4U
#define CMS_OUTPUT_BYTES 0x200000U
typedef struct {
    unsigned char id[18];
    unsigned int real, length, cursor, records, source_bytes;
    unsigned int closed, profile, token;
} CMSOUTPUT;
static CMSOUTPUT cms_outputs[CMS_OUTPUT_SLOTS];
#define CMS_OUTPUT_HANDLE_BASE 0x100U
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
    if (TSDATTACH(&u_tables,(unsigned char *)TSF_UPOOL_VA,TSF_UPOOL_REAL,
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
    TSVINIT(&native_invocations);
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
int TSCCLEAR(unsigned int subchannel, unsigned char *irb);

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
#define CMS31_LOWCORE_OWNER 0x434d534eU

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
    TSPADDR base, placed, lowcore_address;
    TSHINFO info;
    unsigned int blocks, result, i, real=0U, entry, stage_real;
    unsigned int lowcore_real=0U;
    unsigned char *lowcore, *pc;
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
    if (TSRALLOC(&storage.real,CMS31_LOWCORE_OWNER,4096U,
                 TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,
                 &lowcore_real)!=TSR_OK) {
        result=4U; goto discard_image;
    }
    lowcore=(unsigned char *)TSF_KAPERTURE_VA+lowcore_real;
    for (i=0U; i<4096U; ++i) lowcore[i]=0U;
    /* Both compatibility pointers live in U-owned lowcore backing. PC-cp
       enters the U veneer, whose SVC transfers the checked plist to K. */
    cms_put_word(lowcore+0x10U,0x500U);  /* PSA -> U CVT */
    lowcore[0x16U]=1U;       /* U 0x14 -> U SYSREF 0x100 */
    lowcore[0x10eU]=2U;      /* SYSREF+12 -> U veneer 0x200 */
    lowcore[0x200U]=0x0aU;  /* SVC 205, then BR R14 */
    lowcore[0x201U]=0xcdU;
    lowcore[0x202U]=0x07U;
    lowcore[0x203U]=0xfeU;
    lowcore[0x400U]=0x0aU;  /* SVC 233, then stacking PROGRAM RETURN */
    lowcore[0x401U]=0xe9U;
    lowcore[0x402U]=0x01U;  /* PR; exact opcode checked against GNU as */
    lowcore[0x403U]=0x01U;
    cms_put_word(lowcore+0x804U,0xa00U); /* CVT+772 -> U SFT */
    TSKEYSET(lowcore_real,0x80U);
    base.hi=0U; base.lo=0U;
    lowcore_address.hi=0U; lowcore_address.lo=lowcore_real;
    if (TSDMAP(&u_tables,base,lowcore_address)!=TSD_OK) {
        result=12U; goto discard_lowcore;
    }
    pc=(unsigned char *)TSF_KAPERTURE_VA+TSF_PC_REAL;
    for (i=0U; i<TSF_PC_BYTES; ++i) pc[i]=0U;
    cms_put_word(pc+24U,0x80000000U|TSF_PC_REAL+0x100U);
    for (i=0U; i<32U; ++i)
        cms_put_word(pc+0x100U+4U*i,0x80000000U);
    cms_put_word(pc+0x100U,TSF_PC_REAL+0x200U|3U);
    cms_put_word(pc+0x200U+14U*32U+4U,0x80000400U);
    cms_put_word(pc+0x200U+14U*32U+8U,0xffff0000U);
    cms_put_word(pc+0x200U+14U*32U+16U,0x80000000U);
    pc[0x1008U]=0x09U;
    pc[0x100aU]=0x0fU;
    pc[0x100bU]=0xa0U;
    *(volatile unsigned int *)0x40f8U=lowcore_real;
    cms31_lowcore_real=lowcore_real;
    for (i=0U; i<4096U; ++i) cms_lowcore_template[i]=lowcore[i];
    cms_lowcore_template_ready=1U;
    *(volatile unsigned int *)0x40e0U=real;
    *(volatile unsigned int *)0x40e4U=entry;
    *(volatile unsigned int *)0x40e8U=info.image_bytes;
    *(volatile unsigned int *)0x40ecU=blocks;
    *(volatile unsigned int *)0x4098U=u_tables.used;
    cms31_loaded=1U;
    goto release;
discard_lowcore:
    if (TSRRELEASE(&storage.real,CMS31_LOWCORE_OWNER,lowcore_real)!=TSR_OK)
        result=0xfffffff0U;
discard_image:
    if (TSMFREE(&storage,4U,placed)!=TSM_OK) result=0xfffffff0U;
release:
    if (TSRRELEASE(&storage.real,CMS_STAGE_OWNER,stage_real) != TSR_OK)
        return 12U;
    return result;
}

/* A second, independently backed relocation of the same unchanged MODULE.
 * Each application invocation gets fresh module data and its own U interval. */
static unsigned int cms31_secondary_map_service(const TSGREQUEST *request)
{
    static const unsigned char name[] = {
        0xc3U,0xd4U,0xe2U,0xf3U,0xf1U,0x4bU,
        0xd9U,0xe7U,0xe5U,0xd4U
    };
    unsigned char *stage;
    TSPADDR base, placed;
    TSHINFO info;
    unsigned int blocks, result, i, real=0U, entry, stage_real;
    int allocation;
    if (request->length || request->address.hi || request->address.lo ||
        request->direction || !cms31_loaded || cms31_secondary_loaded)
        return 8U;
    result=cms_stage_read(name,sizeof name,31U,&stage,&stage_real,
                          &info,&blocks);
    if (result!=0U) return result;
    base.hi=0U; base.lo=0x05000000U;
    allocation=TSMALLOC(&storage,8U,31U,base,base,
                         info.image_bytes,1,&placed);
    if (allocation!=TSM_OK) {
        result=allocation==TSM_NOMEM ? 4U : 12U;
        goto release;
    }
    for (i=0U; i<TSM_ALLOCS; ++i)
        if (storage.allocations[i].handle &&
            storage.allocations[i].task==8U &&
            storage.allocations[i].address.hi==placed.hi &&
            storage.allocations[i].address.lo==placed.lo &&
            storage.allocations[i].bytes>=info.image_bytes) {
            real=storage.allocations[i].real;
            break;
        }
    if (!real || TSHIMAGE(stage,blocks*18452U,31U,placed.lo,
                          (unsigned char *)TSF_KAPERTURE_VA+real,
                          info.image_bytes,&entry)!=TSH_OK) {
        result=TSMFREE(&storage,8U,placed)==TSM_OK ?
               12U : 0xfffffff0U;
        goto release;
    }
    *(volatile unsigned int *)0x4120U=real;
    *(volatile unsigned int *)0x4124U=entry;
    *(volatile unsigned int *)0x4128U=info.image_bytes;
    *(volatile unsigned int *)0x412cU=blocks;
    *(volatile unsigned int *)0x4098U=u_tables.used;
    cms31_secondary_loaded=1U;
release:
    if (TSRRELEASE(&storage.real,CMS_STAGE_OWNER,stage_real)!=TSR_OK)
        return 0xfffffff0U;
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

/* A second invocation needs pristine fixed-origin RXVM bytes. Keep the
 * exited first image as suspended parent backing and copy the checked
 * MODULE into a temporary K-owned real interval before the overlay push. */
static unsigned int cms24_fresh_push(const TSGREQUEST *request)
{
    static const unsigned char name[] = {
        0xc3U,0xd4U,0xe2U,0xf2U,0xf4U,0x4bU,0xd9U,0xe7U,
        0xe5U,0xd4U
    };
    unsigned char *stage, *image;
    TSHINFO info;
    TSPADDR base;
    unsigned int blocks, stage_real, image_real=0U, span, entry, result;
    unsigned int pushed=0U, returned;
    int status;
    if (request->length || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    if (!cms24_loaded || storage.depth) return 4U;
    result=cms_stage_read(name,sizeof name,24U,&stage,&stage_real,
                          &info,&blocks);
    if (result) return result;
    span=(info.image_bytes+4095U)&~4095U;
    if (TSRALLOC(&storage.real,CMS_FILE_OWNER+6U,span,
                 TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,&image_real)
            !=TSR_OK) { result=4U; goto release_stage; }
    image=(unsigned char *)TSF_KAPERTURE_VA+image_real;
    base.hi=0U; base.lo=0x20000U;
    if (TSHIMAGE(stage,blocks*18452U,24U,base.lo,image,
                 info.image_bytes,&entry)!=TSH_OK || entry!=base.lo) {
        result=12U; goto release_image;
    }
    status=TSMOVERLAYPUSH(&storage,5U,base,image,info.image_bytes);
    result=status==TSM_OK ? 0U : status==TSM_NOMEM ? 4U : 12U;
    if (!result) {
        pushed=1U;
        *(volatile unsigned int *)0x4098U=u_tables.used;
    }
release_image:
    if (TSRRELEASE(&storage.real,CMS_FILE_OWNER+6U,image_real)!=TSR_OK)
        result=0xfffffff0U;
release_stage:
    if (TSRRELEASE(&storage.real,CMS_STAGE_OWNER,stage_real)!=TSR_OK)
        result=0xfffffff0U;
    if (result && pushed) {
        if (TSMOVERLAYPOP(&storage,5U,base,0U,&returned)!=TSM_OK)
            result=0xfffffff0U;
        *(volatile unsigned int *)0x4098U=u_tables.used;
    }
    return result;
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
    static const unsigned char retry_message[] = {
        0xd2U,0x40U,0xd9U,0xc5U,0xe3U,0xd9U,0xe8U,0x40U,
        0xd9U,0xc5U,0xc1U,0xc4U,0xe8U
    };
    const unsigned char *label;
    unsigned int label_length;
    unsigned char *screen;
    unsigned int ssid, i;
    int io_result;
    if (request->length>1U || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    label=request->length ? retry_message : message;
    label_length=request->length ? (unsigned int)sizeof retry_message :
                                   (unsigned int)sizeof message;
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
    for (i=0U; i<label_length; ++i) screen[6U+i]=label[i];
    screen[1766]=0x1dU; screen[1767]=0U; screen[1768]=0x13U;
    screen[1769]=0x3cU; screen[1770]=0x5dU; screen[1771]=0x7fU;
    screen[1772]=0U;
    if (TSCBUILDCONSWRITE(&console_channel,1773U) != TSC_OK) return 20U;
    if (!console_ssid &&
        TSCENABL(ssid,TSCSCHIB(&console_channel)) != 0) return 23U;
    io_result=TSCIO(ssid,TSCORB(&console_channel),TSCIRB(&console_channel));
    *(volatile unsigned int *)0x40c4U=(unsigned int)io_result;
    if (io_result != 0) return 21U;
    if (TSCCHECKWRITE(&console_channel) != TSC_OK) return 22U;
    console_ssid=ssid;
    return 0U;
}

static unsigned int terminal_read_start(const TSGREQUEST *request)
{
    const TSVFRAME *frame=TSVTOP(&native_invocations);
    if (request->length || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    if (!console_ssid) return 0xfffffffbU;
    if (console_read_phase) return 4U;
    if (frame && TSVOWN(&native_invocations,frame->token,TSV_TERMINAL,
                        console_ssid)!=TSV_OK) return 4U;
    /* A 3270 READ MODIFIED issued before an AID returns NoAID with no
       modified fields. Wait for attention, then submit the read CCW. */
    console_read_owner=frame ? frame->token : 0U;
    console_read_io_handle=0U;
    console_read_phase=1U;
    return 0U;
}

static unsigned int terminal_io_complete(unsigned int token)
{
    const TSVFRAME *frame=TSVTOP(&native_invocations);
    if (!console_read_io_handle) return 0U;
    if (!frame || frame->token!=token ||
        TSVCOMPLETE(&native_invocations,token,console_read_io_handle)
            !=TSV_OK ||
        TSVFORGET(&native_invocations,token,TSV_IO,
                  console_read_io_handle)!=TSV_OK) return 12U;
    console_read_io_handle=0U;
    return 0U;
}

static unsigned int terminal_read_cancel(unsigned int token)
{
    const TSVFRAME *frame=TSVTOP(&native_invocations);
    if (!console_read_phase) return 0U;
    if (token!=console_read_owner ||
        (token && (!frame || frame->token!=token))) return 8U;
    /* A phase-one attention may already be pending when its owner exits.
     * Quiesce that subchannel before another owner may submit a read. */
    if (console_read_phase==1U || console_read_phase==2U ||
        console_read_phase==4U) {
        if (TSCCLEAR(console_ssid,TSCIRB(&console_channel))!=0 ||
            terminal_io_complete(token)!=0U) {
            console_read_phase=4U; /* quarantine real workspace */
            return 12U;
        }
    }
    if (token && TSVFORGET(&native_invocations,token,TSV_TERMINAL,
                           console_ssid)!=TSV_OK) return 12U;
    console_read_phase=console_read_count=console_read_owner=0U;
    console_read_io_handle=0U;
    return 0U;
}

static unsigned int terminal_read_poll(const TSGREQUEST *request)
{
    const TSVFRAME *frame=TSVTOP(&native_invocations);
    TSGCONTEXT gate;
    TSGREQUEST output;
    unsigned char bytes[TSG_MAX_COPY];
    unsigned int count, i;
    int status;
    if (request->length != TSG_MAX_COPY ||
        request->direction != TSG_WRITE) return 8U;
    if (!console_read_phase) return 0xfffffffbU;
    if ((frame ? frame->token : 0U)!=console_read_owner) return 8U;
    if (console_read_phase==4U) return 12U;
    if (console_read_phase != 3U) {
        status=TSCPOLL(console_ssid,TSCIRB(&console_channel));
        if (status < 0) {
            console_read_phase=4U;
            terminal_read_cancel(console_read_owner);
            return 12U;
        }
        if (status != 0) return 1U;
    }
    if (console_read_phase == 1U) {
        if ((TSCIRB(&console_channel)[8U] & 0x80U) == 0U) return 1U;
        if (TSCBUILDCONSREAD(&console_channel,252U) != TSC_OK) {
            terminal_read_cancel(console_read_owner);
            return 12U;
        }
        if (frame) {
            if (console_next_io_handle==0xffffffffU) return 4U;
            ++console_next_io_handle;
            if (TSVOWN(&native_invocations,frame->token,TSV_IO,
                       console_next_io_handle)!=TSV_OK) return 4U;
            console_read_io_handle=console_next_io_handle;
        }
        console_read_phase=2U;
        if (TSCSTART(console_ssid,TSCORB(&console_channel),
                     TSCIRB(&console_channel)) != 0) {
            terminal_read_cancel(console_read_owner);
            return 12U;
        }
        return 1U;
    }
    if (console_read_phase == 2U) {
        if (TSCCHECKCONSREAD(&console_channel,252U,&count) != TSC_OK) {
            /* Initial status can precede the actual READ MODIFIED data. */
            if (TSCIRB(&console_channel)[8U] == 0U &&
                TSCIRB(&console_channel)[9U] == 0U) return 1U;
            terminal_read_cancel(console_read_owner);
            return 12U;
        }
        if (terminal_io_complete(console_read_owner)!=0U) return 12U;
        if (count == 3U && TSCDATA(&console_channel)[0] == 0x60U) {
            console_read_phase=1U;
            return 1U;
        }
        if (count < 3U || count > 252U) {
            console_read_phase=3U;
            terminal_read_cancel(console_read_owner);
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
    if (status == TSG_OK && terminal_read_cancel(console_read_owner)!=0U)
        return 12U;
    return status == TSG_OK ? 0U :
           status == TSG_DENIED ? 0xfffffffcU : 0xfffffffdU;
}

static unsigned int terminal_cancel_probe(const TSGREQUEST *request)
{
    const TSVFRAME *frame=TSVTOP(&native_invocations);
    if (request->length || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    return terminal_read_cancel(frame ? frame->token : 0U);
}

static unsigned int terminal_phase_probe(const TSGREQUEST *request)
{
    if (request->length || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    return console_read_phase;
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
    if (TSCCLEAR(0x0001ffffU,TSCIRB(&channel)) != -1)
        return 28U;
    return 0U;
}

/* Private channel-cancellation gate. An idle clear still has an asynchronous
 * completion; success requires TSCH to report the clear-function event. */
static unsigned int terminal_clear_probe(const TSGREQUEST *request)
{
    if (request->length || request->address.hi || request->address.lo ||
        request->direction || !console_ssid || console_read_phase)
        return 8U;
    return TSCCLEAR(console_ssid,TSCIRB(&console_channel))==0 ? 0U : 12U;
}

/* Diagnostic native-entry gate. The real launcher will call this from K
 * before transferring to U; the current machine fixture uses SVC 235/236
 * around its unchanged entry to exercise the same K-owned descriptor. */
static unsigned int native_image_base(unsigned int mode)
{ return mode==24U ? 0x00400000U :
         mode==31U ? 0x07000000U : 0x09000000U; }

static unsigned int native_receipt_base(unsigned int mode)
{ return mode==24U ? 0x4680U :
         mode==31U ? 0x4600U : 0x4640U; }

static unsigned int native_loaded(unsigned int personality,
                                  unsigned int mode,
                                  unsigned int image_owner)
{
    if (personality==TSV_TSO)
        return mode==24U ? tso24_loaded :
               mode==31U ? tso31_loaded : tso64_loaded;
    if (personality==TSV_CMS)
        return mode==24U ? cms24_loaded :
               image_owner==8U ? cms31_secondary_loaded : cms31_loaded;
    return 0U;
}

static unsigned int native_image_bytes(unsigned int personality,
                                       unsigned int mode,
                                       unsigned int image_owner)
{
    if (personality==TSV_CMS)
        return *(volatile const unsigned int *)(mode==24U ? 0x4108U :
            image_owner==8U ? 0x4128U : 0x40e8U);
    return *(volatile const unsigned int *)(native_receipt_base(mode)+4U);
}

static const TSVFRAME *native_caller(unsigned int pc)
{
    const TSVFRAME *frame=TSVTOP(&native_invocations);
    unsigned int base, bytes, mask_hi, mask_lo;
    if (!frame || !native_loaded(frame->personality,frame->amode,
                                  frame->image_owner)) return 0;
    if (frame->personality==TSV_CMS) {
        base=frame->amode==24U ? 0x00020000U :
             frame->image_owner==8U ? 0x05000000U : 0x03000000U;
    } else {
        base=native_image_base(frame->amode);
    }
    bytes=native_image_bytes(frame->personality,frame->amode,
                             frame->image_owner);
    mask_hi=*(volatile const unsigned int *)0x3080U;
    mask_lo=*(volatile const unsigned int *)0x3084U;
    if (!bytes || pc<base || pc-base>=bytes ||
        (frame->amode==64U ?
            (!(mask_hi&1U) && !(mask_lo&0x80000000U)) :
            (mask_hi&1U)) ||
        (frame->amode==24U && (mask_lo&0x80000000U)) ||
        (frame->amode==31U && !(mask_lo&0x80000000U))) return 0;
    return frame;
}

static unsigned int native_begin(TSGREQUEST *request)
{
    TSVCONTEXT caller;
    unsigned int i, mode, token, personality, image_owner, saved_real=0U;
    unsigned int runtime_owner;
    unsigned char *lowcore=(unsigned char *)TSF_KAPERTURE_VA+
                           cms31_lowcore_real;
    volatile const unsigned int *saved=(volatile const unsigned int *)0x3000U;
    int rc;
    /* The selector names a checked K image record. R0 never declares a
     * service personality, AMODE, owner or placement. */
    if (request->address.hi || request->address.lo || request->direction)
        return 8U;
    switch (request->length) {
    case 1U:
        personality=TSV_CMS; mode=24U; image_owner=5U; runtime_owner=1U;
        break;
    case 2U:
        personality=TSV_CMS; mode=31U; image_owner=4U; runtime_owner=1U;
        break;
    case 3U:
        personality=TSV_CMS; mode=31U; image_owner=8U; runtime_owner=1U;
        break;
    case 4U:
        personality=TSV_TSO; mode=24U; image_owner=18U; runtime_owner=18U;
        break;
    case 5U:
        personality=TSV_TSO; mode=31U; image_owner=15U; runtime_owner=16U;
        break;
    case 6U:
        personality=TSV_TSO; mode=64U; image_owner=17U; runtime_owner=17U;
        break;
    default:
        return 8U;
    }
    if (!native_loaded(personality,mode,image_owner) ||
        !native_image_bytes(personality,mode,image_owner)) return 8U;
    for (i=0U; i<16U; ++i) {
        caller.gpr[i].hi=saved[2U*i];
        caller.gpr[i].lo=saved[2U*i+1U];
    }
    caller.psw.hi=saved[0x80U/4U];
    caller.psw.lo=saved[0x84U/4U];
    caller.asce.hi=saved[0x90U/4U];
    caller.asce.lo=saved[0x94U/4U];
    caller.key=(caller.psw.hi>>20)&15U;
    rc=TSVBEGIN(&native_invocations,personality,mode,
                image_owner,runtime_owner,
                &caller,&token);
    if (rc!=TSV_OK) return rc==TSV_FULL ? 4U : 8U;
    if (cms_lowcore_template_ready && native_invocations.depth>1U) {
        if (TSRALLOC(&storage.real,CMS_LOWCORE_SAVE_OWNER,4096U,
                     TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,
                     &saved_real)!=TSR_OK) {
            rc=TSVEND(&native_invocations,token,native_clean,
                      &native_invocations.frame[
                        native_invocations.depth-1U]);
            return rc==TSV_OK ? 4U : 12U;
        }
        for (i=0U; i<4096U; ++i)
            ((unsigned char *)TSF_KAPERTURE_VA+saved_real)[i]=lowcore[i];
        if (TSVOWN(&native_invocations,token,TSV_LOWCORE,saved_real)
            !=TSV_OK) {
            if (TSRRELEASE(&storage.real,CMS_LOWCORE_SAVE_OWNER,
                           saved_real)!=TSR_OK) return 12U;
            rc=TSVEND(&native_invocations,token,native_clean,
                      &native_invocations.frame[
                        native_invocations.depth-1U]);
            return rc==TSV_OK ? 4U : 12U;
        }
        for (i=0U; i<4096U; ++i) lowcore[i]=cms_lowcore_template[i];
    }
    cms_heaps[native_invocations.depth-1U].address.hi=0U;
    cms_heaps[native_invocations.depth-1U].address.lo=0U;
    cms_heaps[native_invocations.depth-1U].bytes=0U;
    cms_heaps[native_invocations.depth-1U].handle=0U;
    request->address.hi=0U;
    request->address.lo=token;
    return 0U;
}

static int native_clean(unsigned int token, const TSVRESOURCE *resource,
                        void *context)
{
    const TSVFRAME *frame=(const TSVFRAME *)context;
    unsigned int i;
    (void)token;
    if (!frame || !resource) return 1;
    if (resource->kind==TSV_LOWCORE) {
        unsigned char *saved=(unsigned char *)TSF_KAPERTURE_VA+
                             resource->handle;
        unsigned char *lowcore=(unsigned char *)TSF_KAPERTURE_VA+
                               cms31_lowcore_real;
        if (!cms31_lowcore_real || !cms_lowcore_template_ready ||
            resource->handle<TSF_CORE_BYTES ||
            resource->handle>TSF_REAL_BYTES-4096U) return 1;
        for (i=0U; i<4096U; ++i) lowcore[i]=saved[i];
        return TSRRELEASE(&storage.real,CMS_LOWCORE_SAVE_OWNER,
                          resource->handle)==TSR_OK ? 0 : 1;
    }
    if (resource->kind==TSV_FILE) {
        unsigned int owner;
        if (resource->handle>=1U && resource->handle<=TSI_SLOTS) {
            TSIINPUT *input=&cms_inputs.slot[resource->handle-1U];
            owner=TSIOWNER(&cms_inputs,input,CMS_FILE_OWNER+16U);
            if (!input->real || input->token!=frame->token || !owner ||
                TSRRELEASE(&storage.real,owner,input->real)!=TSR_OK)
                return 1;
            TSICLEAR(input);
            return 0;
        }
        if (resource->handle>CMS_OUTPUT_HANDLE_BASE &&
            resource->handle<=CMS_OUTPUT_HANDLE_BASE+CMS_OUTPUT_SLOTS) {
            unsigned int slot=resource->handle-CMS_OUTPUT_HANDLE_BASE-1U;
            CMSOUTPUT *out=&cms_outputs[slot];
            if (!out->real || out->token!=frame->token || out->closed ||
                TSRRELEASE(&storage.real,CMS_FILE_OWNER+slot+1U,
                           out->real)!=TSR_OK) return 1;
            out->real=out->length=out->cursor=0U;
            out->records=out->source_bytes=out->closed=0U;
            out->profile=out->token=0U;
            return 0;
        }
        return 1;
    }
    if (resource->kind!=TSV_ALLOCATION) return 1;
    for (i=0U; i<TSM_ALLOCS; ++i)
        if (storage.allocations[i].handle==resource->handle) {
            if (storage.allocations[i].task!=frame->runtime_owner ||
                TSMFREE(&storage,frame->runtime_owner,
                        storage.allocations[i].address)!=TSM_OK) return 1;
            *(volatile unsigned int *)0x4098U=u_tables.used;
            return 0;
        }
    /* Never call a stale or foreign handle successfully cleaned. */
    return 1;
}

static unsigned int native_end(const TSGREQUEST *request)
{
    const TSVFRAME *frame;
    int rc;
    if (request->length || request->direction || request->address.hi ||
        !request->address.lo) return 8U;
    frame=native_invocations.depth ?
        &native_invocations.frame[native_invocations.depth-1U] : 0;
    if (!frame || frame->token!=request->address.lo) return 12U;
    if (console_read_phase && console_read_owner==frame->token &&
        terminal_read_cancel(frame->token)!=0U) return 12U;
    rc=TSVEND(&native_invocations,request->address.lo,native_clean,
              (void *)frame);
    if (rc==TSV_OK) {
        cms_heaps[native_invocations.depth].address.hi=0U;
        cms_heaps[native_invocations.depth].address.lo=0U;
        cms_heaps[native_invocations.depth].bytes=0U;
        cms_heaps[native_invocations.depth].handle=0U;
    }
    return rc==TSV_OK ? 0U : 12U;
}

static unsigned int allocation_handle(unsigned int task, TSPADDR address)
{
    unsigned int i;
    for (i=0U; i<TSM_ALLOCS; ++i)
        if (storage.allocations[i].handle &&
            storage.allocations[i].task==task &&
            storage.allocations[i].address.hi==address.hi &&
            storage.allocations[i].address.lo==address.lo)
            return storage.allocations[i].handle;
    return 0U;
}

/* Diagnostic K leak fixture: give an active TSO31 invocation one
 * ordinary allocation, then let native_end reclaim it. SVC 237 is private
 * to the machine proof, never an application storage interface. */
static unsigned int native_reap_probe(TSGREQUEST *request)
{
    const TSVFRAME *frame=TSVTOP(&native_invocations);
    TSPADDR minimum, maximum, base;
    unsigned int handle, rc;
    if (!frame || frame->personality!=TSV_TSO || frame->amode!=31U ||
        request->length || request->direction || request->address.hi ||
        request->address.lo) return 8U;
    minimum.hi=maximum.hi=0U;
    minimum.lo=0x02010000U; maximum.lo=0x7fffffffU;
    rc=TSMALLOC(&storage,frame->runtime_owner,31U,minimum,maximum,
               4096U,0,&base);
    if (rc!=TSM_OK) return rc==TSM_NOMEM ? 4U : 8U;
    handle=allocation_handle(frame->runtime_owner,base);
    if (!handle || TSVOWN(&native_invocations,frame->token,
                          TSV_ALLOCATION,handle)!=TSV_OK) {
        if (TSMFREE(&storage,frame->runtime_owner,base)!=TSM_OK) return 12U;
        return 4U;
    }
    request->address=base;
    *(volatile unsigned int *)0x4098U=u_tables.used;
    return 0U;
}

/* The current fixture's SVC 120 path uses the active conditional GETMAIN
 * register convention: R0 length, R1 zero to allocate or base to free, R15
 * X'10' below-line or X'30' above-line. The old PSW belongs to K's saved
 * frame, never to caller-supplied memory. Failure has no low-memory fallback.
 */
static unsigned int storage_service(TSGREQUEST *request)
{
    const TSVFRAME *frame;
    unsigned int mask_hi, mask_lo, flags, mode, storage_mode, result, task, pc;
    unsigned int handle;
    TSPADDR minimum, maximum, base;
    mask_hi=*(volatile const unsigned int *)0x3080U;
    mask_lo=*(volatile const unsigned int *)0x3084U;
    flags=*(volatile const unsigned int *)0x307cU;
    mode=(mask_hi & 1U) ? 64U : (mask_lo & 0x80000000U) ? 31U : 24U;
    pc=*(volatile const unsigned int *)0x308cU;
    frame=native_caller(pc);
    if (TSVTOP(&native_invocations) && !frame) return 8U;
    task=frame ? frame->runtime_owner : 1U;
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
        result=TSMALLOC(&storage,task,storage_mode,minimum,maximum,
                        request->length,0,&base);
        if (result != TSM_OK) return result == TSM_NOMEM ? 4U : 8U;
        if (frame) {
            handle=allocation_handle(task,base);
            if (!handle || TSVOWN(&native_invocations,frame->token,
                                  TSV_ALLOCATION,handle)!=TSV_OK) {
                if (TSMFREE(&storage,task,base)!=TSM_OK) return 12U;
                return 4U;
            }
        }
        request->address=base;
        if (task==18U) {
            volatile unsigned int *receipt=(volatile unsigned int *)0x4cc0U;
            unsigned int count=receipt[0U];
            if (count<3U) {
                receipt[2U+2U*count]=base.lo;
                receipt[3U+2U*count]=request->length;
            }
            receipt[0U]=count+1U;
        }
        *(volatile unsigned int *)0x4098U=u_tables.used;
        return 0U;
    }
    handle=frame ? allocation_handle(task,request->address) : 0U;
    if (frame && (!handle ||
        TSVHAS(&native_invocations,frame->token,TSV_ALLOCATION,handle)
            !=TSV_OK)) return 8U;
    result=TSMFREE(&storage,task,request->address);
    if (result != TSM_OK) return 8U;
    if (frame && TSVFORGET(&native_invocations,frame->token,
                           TSV_ALLOCATION,handle)!=TSV_OK) return 12U;
    if (task==18U) ++*(volatile unsigned int *)0x4cc4U;
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

/* The selected IARV64 macro uses a stacking PC-cp to a U veneer. The veneer
 * invokes SVC 233; this K endpoint owns all translation and real backing.
 * Only the pinned GETSTOR/DETACH plist subset and 32/128 segment sizes are
 * accepted. Nothing from U is dereferenced as a K pointer. */
static unsigned int iarv64_service(const TSGREQUEST *request)
{
    const TSVFRAME *frame=TSVTOP(&native_invocations);
    TSGCONTEXT gate;
    TSGREQUEST copy;
    TSPADDR minimum, maximum, base;
    unsigned char plist[88], result_bytes[8];
    unsigned int segments, status, opcode, handle;
    volatile unsigned int *receipt=(volatile unsigned int *)0x4e00U;
    if (!tso64_loaded || !frame || frame->personality!=TSV_TSO ||
        frame->amode!=64U || frame->runtime_owner!=17U ||
        (*(volatile const unsigned int *)0x3080U&1U) ||
        !(*(volatile const unsigned int *)0x3084U&0x80000000U) ||
        *(volatile const unsigned int *)0x308cU!=0x402U ||
        request->address.hi || request->address.lo<0x09000000U ||
        request->address.lo>=0x80000000U-88U) return 8U;
    gate.u_tables=&u_tables;
    gate.real_aperture=(unsigned char *)TSF_KAPERTURE_VA;
    gate.real_bytes=TSF_REAL_BYTES;
    gate.rights=user_rights;
    gate.rights_context=0;
    copy=*request; copy.length=sizeof plist; copy.direction=TSG_READ;
    if (TSGCOPY(&gate,&copy,plist,sizeof plist)!=TSG_OK ||
        plist[0]!=0U || !(plist[5]&0x80U) ||
        cms_word(plist+8U)!=0U) return 8U;
    opcode=plist[1]; segments=cms_word(plist+12U);
    receipt[0U]+=1U; receipt[1U]=opcode; receipt[2U]=segments;
    if (opcode==1U) {
        if (segments!=32U && segments!=128U) return 8U;
        receipt[6U]=segments;
        copy.address.lo+=40U; copy.length=8U; copy.direction=TSG_WRITE;
        if (TSGPROBE(&gate,&copy)!=TSG_OK) return 8U;
        minimum.hi=maximum.hi=1U;
        minimum.lo=0x20000000U; maximum.lo=0x7fffffffU;
        status=TSMALLOC(&storage,17U,64U,minimum,maximum,
                       segments*0x100000U,0,&base);
        if (status!=TSM_OK) return status==TSM_NOMEM ? 4U : 8U;
        handle=allocation_handle(17U,base);
        if (!handle || TSVOWN(&native_invocations,frame->token,
                              TSV_ALLOCATION,handle)!=TSV_OK) {
            if (TSMFREE(&storage,17U,base)!=TSM_OK) return 12U;
            return 4U;
        }
        cms_put_word(result_bytes,base.hi);
        cms_put_word(result_bytes+4U,base.lo);
        if (TSGCOPY(&gate,&copy,result_bytes,sizeof result_bytes)!=TSG_OK) {
            if (TSMFREE(&storage,17U,base)!=TSM_OK) return 0xfffffff0U;
            if (TSVFORGET(&native_invocations,frame->token,
                          TSV_ALLOCATION,handle)!=TSV_OK) return 12U;
            return 8U;
        }
        receipt[3U]=base.hi; receipt[4U]=base.lo;
        *(volatile unsigned int *)0x4098U=u_tables.used;
        return 0U;
    }
    if (opcode==3U) {
        base.hi=cms_word(plist+56U);
        base.lo=cms_word(plist+60U);
        handle=allocation_handle(17U,base);
        if (base.hi!=1U || base.lo<0x20000000U || !handle ||
            TSVHAS(&native_invocations,frame->token,
                   TSV_ALLOCATION,handle)!=TSV_OK ||
            TSMFREE(&storage,17U,base)!=TSM_OK) return 8U;
        if (TSVFORGET(&native_invocations,frame->token,
                      TSV_ALLOCATION,handle)!=TSV_OK) return 12U;
        receipt[5U]+=1U;
        *(volatile unsigned int *)0x4098U=u_tables.used;
        return 0U;
    }
    return 8U;
}

static unsigned int cms_word(const unsigned char *p)
{
    return ((unsigned int)p[0]<<24) | ((unsigned int)p[1]<<16) |
           ((unsigned int)p[2]<<8) | (unsigned int)p[3];
}

static void cms_put_word(unsigned char *p, unsigned int value)
{
    p[0]=(unsigned char)(value>>24); p[1]=(unsigned char)(value>>16);
    p[2]=(unsigned char)(value>>8); p[3]=(unsigned char)value;
}

static unsigned int cms_file_close(TSIINPUT *input)
{
    const TSVFRAME *frame=TSVTOP(&native_invocations);
    unsigned int owner, handle;
    if (!input) return 12U;
    owner=TSIOWNER(&cms_inputs,input,CMS_FILE_OWNER+16U);
    handle=TSIOWNER(&cms_inputs,input,1U);
    if (!owner || !handle ||
        (input->token && (!frame || frame->token!=input->token ||
          TSVHAS(&native_invocations,frame->token,TSV_FILE,handle)
              !=TSV_OK))) return 12U;
    if (input->real &&
        TSRRELEASE(&storage.real,owner,input->real)
            !=TSR_OK)
        return 12U;
    if (input->token &&
        TSVFORGET(&native_invocations,input->token,TSV_FILE,handle)
            !=TSV_OK) return 12U;
    TSICLEAR(input);
    return 0U;
}

static CMSOUTPUT *cms_output_find(const unsigned char id[18],
                                  unsigned int profile, unsigned int token)
{
    unsigned int slot, i;
    for (slot=0U; slot<CMS_OUTPUT_SLOTS; ++slot) {
        if (!cms_outputs[slot].real ||
            cms_outputs[slot].profile!=profile ||
            cms_outputs[slot].token!=token) continue;
        for (i=0U; i<18U && cms_outputs[slot].id[i]==id[i]; ++i) {}
        if (i==18U) return &cms_outputs[slot];
    }
    return 0;
}

static CMSOUTPUT *cms_output_create(const unsigned char id[18],
                                    unsigned int profile, unsigned int token)
{
    unsigned int slot, real, i;
    CMSOUTPUT *out;
    if ((profile!=24U && profile!=31U) ||
        id[16U]!=0xc1U || id[17U]!=0xf1U) return 0;
    for (slot=0U; slot<CMS_OUTPUT_SLOTS; ++slot)
        if (!cms_outputs[slot].real) break;
    if (slot==CMS_OUTPUT_SLOTS ||
        TSRALLOC(&storage.real,CMS_FILE_OWNER+slot+1U,
                 CMS_OUTPUT_BYTES,TSF_CORE_BYTES,TSF_REAL_BYTES,
                 TSR_RUN,&real)!=TSR_OK) return 0;
    if (token && TSVOWN(&native_invocations,token,TSV_FILE,
                        CMS_OUTPUT_HANDLE_BASE+slot+1U)!=TSV_OK) {
        TSRRELEASE(&storage.real,CMS_FILE_OWNER+slot+1U,real);
        return 0;
    }
    out=&cms_outputs[slot];
    for (i=0U; i<18U; ++i) out->id[i]=id[i];
    out->real=real; out->length=out->cursor=64U;
    out->records=out->source_bytes=out->closed=0U;
    out->profile=profile; out->token=token;
    return out;
}

static unsigned int cms_output_discard(CMSOUTPUT *out)
{
    unsigned int slot;
    if (!out) return 12U;
    slot=(unsigned int)(out-cms_outputs);
    if (slot>=CMS_OUTPUT_SLOTS ||
        TSRRELEASE(&storage.real,CMS_FILE_OWNER+slot+1U,
                   out->real)!=TSR_OK) return 12U;
    out->real=out->length=out->cursor=0U;
    out->records=out->source_bytes=out->closed=out->profile=out->token=0U;
    return 0U;
}

static unsigned int cms_output_erase(CMSOUTPUT *out)
{
    const TSVFRAME *frame=TSVTOP(&native_invocations);
    unsigned int slot, handle, token, live;
    if (!out || !frame || out->token!=frame->token) return 12U;
    slot=(unsigned int)(out-cms_outputs);
    if (slot>=CMS_OUTPUT_SLOTS) return 12U;
    handle=CMS_OUTPUT_HANDLE_BASE+slot+1U;
    token=out->token;
    live=!out->closed;
    if (live && TSVHAS(&native_invocations,token,TSV_FILE,handle)
        !=TSV_OK) return 12U;
    if (cms_output_discard(out)!=0U) return 12U;
    if (live && TSVFORGET(&native_invocations,token,TSV_FILE,handle)
        !=TSV_OK) return 12U;
    return 0U;
}

/* The CMS file envelope is a checked sequence of variable records in a
 * fixed-block dataset. Keep its backing in K real storage, never U low VA. */
static unsigned int cms_file_open(const unsigned char id[18],
                                  unsigned int profile, TSIINPUT **opened)
{
    static const unsigned char prefix[6] =
        {0xc3U,0xd4U,0xe2U,0xf3U,0xf1U,0x4bU};
    unsigned char name[32], *stage;
    const unsigned char *record;
    TSKEXTENT extent;
    TSIFINFO info;
    TSIINPUT *input;
    const TSVFRAME *frame=TSVTOP(&native_invocations);
    unsigned int i, j, n=0U, blocks, span, real, token, handle;
    unsigned int cylinder, head, number;
    int found, count;
    if (!opened || (profile!=24U && profile!=31U) ||
        id[16]!=0xc1U || id[17]!=0xf1U) return 28U;
    *opened=0;
    token=frame ? frame->token : 0U;
    input=TSIFINDOWNED(&cms_inputs,id,profile,token);
    if (input) {
        handle=TSIOWNER(&cms_inputs,input,1U);
        if (token && TSVHAS(&native_invocations,token,TSV_FILE,handle)
            !=TSV_OK) return 12U;
        *opened=input;
        return 0U;
    }
    input=TSIEMPTY(&cms_inputs);
    if (!input) return 4U;
    handle=TSIOWNER(&cms_inputs,input,1U);
    if (!handle) return 12U;
    for (i=0U; i<6U; ++i) name[n++]=prefix[i];
    if (profile==24U) { name[3U]=0xf2U; name[4U]=0xf4U; }
    for (i=0U; i<8U && id[i]!=0x40U; ++i) {
        if (id[i]==0x4bU || (i==0U && id[i]==0x5cU)) return 28U;
        name[n++]=id[i];
    }
    if (!i) return 28U;
    for (j=i; j<8U; ++j) if (id[j]!=0x40U) return 28U;
    name[n++]=0x4bU;
    for (i=8U; i<16U && id[i]!=0x40U; ++i) {
        if (id[i]==0x4bU || (i==8U && id[i]==0x5cU)) return 28U;
        name[n++]=id[i];
    }
    if (i==8U) return 28U;
    for (j=i; j<16U; ++j) if (id[j]!=0x40U) return 28U;
    found=TSKFIND(channel_record,&channel,name,n,&extent);
    if (found==TSK_ABSENT) return 28U;
    if (found!=TSK_OK || extent.record_format!=0x80U ||
        extent.block_length!=18452U || extent.logical_length!=18452U)
        return 12U;
    cylinder=extent.start_cylinder; head=extent.start_head; number=1U;
    count=channel_record(&channel,cylinder,head,number,18452U,&record);
    if (count!=18452) return 12U;
    if (TSIFHEADER(record,(unsigned int)count,profile,&info)!=TSIF_OK)
        return 12U;
    blocks=info.blocks;
    span=(blocks*18452U+4095U)&~4095U;
    if (TSRALLOC(&storage.real,
                 TSIOWNER(&cms_inputs,input,CMS_FILE_OWNER+16U),span,
                 TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,&real)!=TSR_OK)
        return 4U;
    stage=(unsigned char *)TSF_KAPERTURE_VA+real;
    for (j=0U; j<blocks; ++j) {
        if (!TSKWITHIN(&extent,cylinder,head)) goto corrupt;
        if (j) {
            count=channel_record(&channel,cylinder,head,number,18452U,&record);
            if (count!=18452) goto corrupt;
        }
        for (i=0U; i<18452U; ++i) stage[j*18452U+i]=record[i];
        if (++number==4U) {
            number=1U; if (++head==15U) { head=0U; ++cylinder; }
        }
    }
    if (TSIFVALIDATE(stage,blocks*18452U,profile,&info)!=TSIF_OK)
        goto corrupt;
    if (token && TSVOWN(&native_invocations,token,TSV_FILE,handle)
        !=TSV_OK)
        return TSRRELEASE(&storage.real,
                          TSIOWNER(&cms_inputs,input,CMS_FILE_OWNER+16U),
                          real)==TSR_OK ? 4U : 0xfffffff0U;
    for (i=0U; i<18U; ++i) input->id[i]=id[i];
    input->real=real;
    input->profile=profile;
    input->token=token;
    input->length=64U+info.payload_bytes;
    input->records=info.records; input->cursor=64U;
    *opened=input;
    return 0U;
corrupt:
    return TSRRELEASE(&storage.real,
                      TSIOWNER(&cms_inputs,input,CMS_FILE_OWNER+16U),real)
                      ==TSR_OK ?
           12U : 0xfffffff0U;
}

/* Diagnostic guest gate: real staged files must retain independent cursors
 * while both are open, then release their distinct K real reservations. */
static unsigned int cms_cursor_guest_probe(const TSGREQUEST *request)
{
    static const unsigned char first_id[18]={
        0xc9U,0xd6U,0xd8U,0xe4U,0xc1U,0xd3U,0x40U,0x40U,
        0xd9U,0xe7U,0xc2U,0xc9U,0xd5U,0x40U,0x40U,0x40U,
        0xc1U,0xf1U};
    static const unsigned char second_id[18]={
        0xd3U,0xc9U,0xc2U,0xd9U,0xc1U,0xd9U,0xe8U,0x40U,
        0xd9U,0xe7U,0xc2U,0xc9U,0xd5U,0x40U,0x40U,0x40U,
        0xc1U,0xf1U};
    TSIINPUT *first=0, *second=0;
    unsigned int result=12U, owner_first, owner_second;
    if (request->length || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    if (cms_file_open(first_id,31U,&first)!=0U || !first)
        return 12U;
    first->cursor=123U;
    if (cms_file_open(second_id,31U,&second)!=0U || !second)
        goto done;
    owner_first=TSIOWNER(&cms_inputs,first,CMS_FILE_OWNER+16U);
    owner_second=TSIOWNER(&cms_inputs,second,CMS_FILE_OWNER+16U);
    if (first!=second && first->real!=second->real &&
        first->cursor==123U && second->cursor==64U &&
        owner_first && owner_second && owner_first!=owner_second &&
        TSIFIND(&cms_inputs,first_id,31U)==first &&
        TSIFIND(&cms_inputs,second_id,31U)==second)
        result=0U;
done:
    if (second && cms_file_close(second)!=0U) result=12U;
    if (first->cursor!=123U || cms_file_close(first)!=0U)
        result=12U;
    return result;
}

/* Load the pinned native TSO31 record stream from a checked 3390 extent.
 * Materialize privately, then publish one noncolliding U31 interval. */
static unsigned int tso_map_service(const TSGREQUEST *request,
                                    unsigned int mode)
{
    static const unsigned char name24[10]={
        0xe3U,0xe2U,0xd6U,0xf2U,0xf4U,0x4bU,
        0xd9U,0xe7U,0xe5U,0xd4U};
    static const unsigned char name31[10]={
        0xe3U,0xe2U,0xd6U,0xf3U,0xf1U,0x4bU,
        0xd9U,0xe7U,0xe5U,0xd4U};
    static const unsigned char name64[10]={
        0xe3U,0xe2U,0xd6U,0xf6U,0xf4U,0x4bU,
        0xd9U,0xe7U,0xe5U,0xd4U};
    const unsigned char *name=mode==24U ? name24 :
                              mode==31U ? name31 : name64;
    const unsigned char *record;
    TSKEXTENT extent;
    TSTINFO info;
    TSPADDR base, maximum, mapped;
    unsigned char *stage, *image, *destination;
    unsigned int bytes, blocks, stage_span, stage_real=0U, image_real=0U;
    unsigned int cylinder, head, number, i, j, slot, mapped_real=0U;
    unsigned int image_fnv=0x811c9dc5U;
    unsigned int task=mode==24U ? 18U : mode==31U ? 15U : 17U;
    volatile unsigned int *receipt=(volatile unsigned int *)(
        mode==24U ? 0x4680U : mode==31U ? 0x4600U : 0x4640U);
    int count, found, rc, failure=12;
    if (request->length || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    if (mode!=24U && mode!=31U && mode!=64U) return 8U;
    if (mode==24U ? tso24_loaded :
        mode==31U ? tso31_loaded : tso64_loaded) return 0U;
    found=TSKFIND(channel_record,&channel,name,10U,&extent);
    if (found==TSK_ABSENT) return 4U;
    if (found!=TSK_OK || extent.record_format!=0x80U ||
        extent.block_length!=TST_BLOCK ||
        extent.logical_length!=TST_BLOCK) return 12U;
    cylinder=extent.start_cylinder; head=extent.start_head; number=1U;
    if (!TSKWITHIN(&extent,cylinder,head)) return 12U;
    count=channel_record(&channel,cylinder,head,number,TST_BLOCK,&record);
    if (count!=(int)TST_BLOCK ||
        (mode==24U ? TSTSTAGEHEADER24(record,(unsigned int)count,
                                     &bytes,&blocks) :
         mode==31U ? TSTSTAGEHEADER(record,(unsigned int)count,
                                   &bytes,&blocks) :
         TSTSTAGEHEADER64(record,(unsigned int)count,
                          &bytes,&blocks))!=TST_OK)
        return 12U;
    stage_span=(blocks*TST_BLOCK+4095U)&~4095U;
    if (TSRALLOC(&storage.real,TSO_STAGE_OWNER,stage_span,
                 TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,&stage_real)!=TSR_OK)
        return 4U;
    stage=(unsigned char *)TSF_KAPERTURE_VA+stage_real;
    for (j=0U; j<blocks; ++j) {
        if (!TSKWITHIN(&extent,cylinder,head)) goto bad_stage;
        if (j) {
            count=channel_record(&channel,cylinder,head,number,
                                 TST_BLOCK,&record);
            if (count!=(int)TST_BLOCK) goto bad_stage;
        }
        for (i=0U; i<TST_BLOCK; ++i)
            stage[j*TST_BLOCK+i]=record[i];
        if (++number==4U) {
            number=1U; if (++head==15U) { head=0U; ++cylinder; }
        }
    }
    if (TSTSTAGEVALIDATE(stage,blocks*TST_BLOCK,mode,&info)!=TST_OK ||
        info.raw_bytes!=bytes) goto bad_stage;
    if (TSRALLOC(&storage.real,TSO_IMAGE_OWNER,TST_MAX_IMAGE,
                 TSF_CORE_BYTES,TSF_REAL_BYTES,TSR_RUN,&image_real)!=TSR_OK) {
        return TSRRELEASE(&storage.real,TSO_STAGE_OWNER,stage_real)
               ==TSR_OK ? 4U : 0xfffffff0U;
    }
    image=(unsigned char *)TSF_KAPERTURE_VA+image_real;
    base.hi=maximum.hi=0U;
    base.lo=mode==24U ? 0x00400000U :
            mode==31U ? 0x07000000U : 0x09000000U;
    if ((mode==24U ? TSTIMAGE24(stage+64U,bytes,base.lo,image,
                               TST_MAX_IMAGE,&info) :
         mode==31U ? TSTIMAGE31(stage+64U,bytes,base.lo,image,
                                TST_MAX_IMAGE,&info) :
         TSTIMAGE64ANY(stage+64U,bytes,base.lo,image,
                       TST_MAX_IMAGE,&info))!=TST_OK) goto bad_image;
    maximum.lo=mode==24U ? 0x00ffffffU : base.lo+0x00ffffffU;
    rc=TSMALLOC(&storage,task,mode,base,maximum,
                info.image_bytes,1,&mapped);
    if (rc!=TSM_OK) {
        if (mode==24U) {
            *(volatile unsigned int *)0x4ce0U=receipt[0U];
            *(volatile unsigned int *)0x4ce4U=tso24_loaded;
        }
        failure=(rc==TSM_NOMEM || rc==TSM_COLLISION) ? 4 : 12;
        goto bad_image;
    }
    for (slot=0U; slot<TSM_ALLOCS; ++slot)
        if (storage.allocations[slot].handle &&
            storage.allocations[slot].task==task &&
            storage.allocations[slot].address.hi==mapped.hi &&
            storage.allocations[slot].address.lo==mapped.lo) {
            mapped_real=storage.allocations[slot].real;
            break;
        }
    if (!mapped_real) {
        TSMFREE(&storage,task,mapped);
        goto bad_image;
    }
    destination=(unsigned char *)TSF_KAPERTURE_VA+mapped_real;
    for (i=0U; i<info.image_bytes; ++i) {
        destination[i]=image[i];
        image_fnv=(image_fnv^image[i])*0x01000193U;
    }
    rc=TSRRELEASE(&storage.real,TSO_IMAGE_OWNER,image_real);
    found=TSRRELEASE(&storage.real,TSO_STAGE_OWNER,stage_real);
    if (rc!=TSR_OK || found!=TSR_OK) return 0xfffffff0U;
    receipt[0U]=mapped_real;
    receipt[1U]=info.image_bytes;
    receipt[2U]=base.lo+info.entry_offset;
    receipt[3U]=blocks;
    receipt[4U]=info.input_fnv;
    receipt[5U]=image_fnv; /* Full mapped image, before its writable data runs. */
    if (mode==24U) tso24_loaded=1U;
    else if (mode==31U) tso31_loaded=1U;
    else tso64_loaded=1U;
    *(volatile unsigned int *)0x4098U=u_tables.used;
    return 0U;
bad_image:
    rc=TSRRELEASE(&storage.real,TSO_IMAGE_OWNER,image_real);
    found=TSRRELEASE(&storage.real,TSO_STAGE_OWNER,stage_real);
    if (rc!=TSR_OK || found!=TSR_OK) return 0xfffffff0U;
    return (unsigned int)failure;
bad_stage:
    return TSRRELEASE(&storage.real,TSO_STAGE_OWNER,stage_real)==TSR_OK ?
           12U : 0xfffffff0U;
}

static unsigned int cms31_fst_lookup(TSGREQUEST *request)
{
    static const unsigned char prefix[6] =
        {0xc3U,0xd4U,0xe2U,0xf3U,0xf1U,0x4bU};
    const unsigned char *record;
    unsigned char id[18];
    unsigned char *fst;
    unsigned int cylinder, head, number, visits=0U, seen=0U;
    unsigned int i, j, name_len, type_len, at;
    int count;
    if (!cms31_lowcore_real || request->address.hi ||
        request->address.lo<0x1000000U ||
        request->address.lo>=0x80000000U || request->length>1024U)
        return 12U;
    count=channel_record(&channel,0U,0U,3U,80U,&record);
    if (count<24 || record[4]!=0xe5U || record[5]!=0xd6U ||
        record[6]!=0xd3U || record[7]!=0xf1U) return 12U;
    cylinder=((unsigned int)record[15U]<<8)|record[16U];
    head=((unsigned int)record[17U]<<8)|record[18U];
    number=record[19U];
    if (cylinder!=1U || head>=15U || !number) return 12U;
    while (cylinder<=2U && visits<1500U) {
        ++visits;
        count=channel_record(&channel,cylinder,head,number,140U,&record);
        if (count<0) {
            number=1U; ++head;
            if (head==15U) { head=0U; ++cylinder; }
            continue;
        }
        if (count<115 || !record) return 12U;
        if (record[0]==0U) return 1U;
        if (record[44U]==0xf1U) {
            for (i=0U; i<6U && record[i]==prefix[i]; ++i) {}
            if (i==6U) {
                at=6U;
                for (name_len=0U; name_len<8U &&
                     at<44U && record[at]!=0x4bU &&
                     record[at]!=0x40U; ++name_len,++at) {}
                if (name_len && at<44U && record[at++]==0x4bU) {
                    j=at;
                    for (type_len=0U; type_len<8U &&
                         at<44U && record[at]!=0x40U;
                         ++type_len,++at) {}
                    if (type_len && (at==44U || record[at]==0x40U)) {
                        for (i=at; i<44U && record[i]==0x40U; ++i) {}
                        if (i==44U) {
                            if (seen++==request->length) {
                                for (i=0U; i<18U; ++i) id[i]=0x40U;
                                for (i=0U; i<name_len; ++i)
                                    id[i]=record[6U+i];
                                for (i=0U; i<type_len; ++i)
                                    id[8U+i]=record[j+i];
                                id[16U]=0xc1U; id[17U]=0xf1U;
                                fst=(unsigned char *)TSF_KAPERTURE_VA+
                                    cms31_lowcore_real+0x300U;
                                for (i=0U; i<40U; ++i) fst[i]=0U;
                                for (i=0U; i<16U; ++i) fst[i]=id[i];
                                fst[16U]=0x10U; fst[17U]=0x04U;
                                fst[24U]=id[16U]; fst[25U]=id[17U];
                                fst[27U]=1U; fst[30U]=0xe5U;
                                fst[34U]=1U; /* 256-byte record cap */
                                fst[38U]=0xf2U; fst[39U]=0xf6U;
                                request->length=seen;
                                request->address.hi=0U;
                                request->address.lo=0x300U;
                                return 0U;
                            }
                        }
                    }
                }
            }
        }
        if (number==255U) {
            number=1U; ++head;
            if (head==15U) { head=0U; ++cylinder; }
        } else ++number;
    }
    return 12U;
}

static unsigned int cms_line_screen(const unsigned char *message,
                                    unsigned int length)
{
    unsigned char *screen;
    unsigned int i;
    if (!console_ssid || console_read_phase || !message || length>130U)
        return 12U;
    screen=TSCDATA(&console_channel);
    for (i=0U; i<1773U; ++i) screen[i]=0x40U;
    screen[0]=0xc3U; screen[1]=0x11U; screen[2]=0x5dU;
    screen[3]=0x7fU; screen[4]=0x1dU; screen[5]=0xf0U;
    for (i=0U; i<length; ++i) screen[6U+i]=message[i];
    screen[1766]=0x1dU; screen[1767]=0U; screen[1768]=0x13U;
    screen[1769]=0x3cU; screen[1770]=0x5dU; screen[1771]=0x7fU;
    screen[1772]=0U;
    if (TSCBUILDCONSWRITE(&console_channel,1773U)!=TSC_OK ||
        TSCIO(console_ssid,TSCORB(&console_channel),
              TSCIRB(&console_channel))!=0 ||
        TSCCHECKWRITE(&console_channel)!=TSC_OK) return 12U;
    return 0U;
}

/* Selected native TSO TPUT. R1's high bit denotes TGET in this ABI;
 * input is a separate service, so never interpret that flag as a U address. */
static unsigned int tso_terminal_service(TSGREQUEST *request)
{
    const TSVFRAME *frame=TSVTOP(&native_invocations);
    TSGCONTEXT gate;
    TSGREQUEST copy;
    unsigned char message[132];
    unsigned int i, status, mask_hi, mask_lo, mode24;
    unsigned int receipt_base=frame && frame->amode==64U ? 0x4d00U :
                              frame && frame->amode==31U ? 0x4700U : 0x4c00U;
    volatile unsigned char *receipt=(volatile unsigned char *)(receipt_base+16U);
    mask_hi=*(volatile const unsigned int *)0x3080U;
    mask_lo=*(volatile const unsigned int *)0x3084U;
    mode24=frame && frame->amode==24U;
    *(volatile unsigned int *)(receipt_base+0xa0U)+=1U;
    *(volatile unsigned int *)(receipt_base+0xa4U)=request->length;
    *(volatile unsigned int *)(receipt_base+0xa8U)=request->address.hi;
    *(volatile unsigned int *)(receipt_base+0xacU)=request->address.lo;
    *(volatile unsigned int *)(receipt_base+0xb0U)=request->direction;
    /* TPUT defines R0/R1; upper GPR halves and R2 are not arguments.
     * Check the native caller's AMODE before using its low address. */
    if ((mask_hi&1U) ||
        (mode24 ? (mask_lo&0x80000000U) :
                  !(mask_lo&0x80000000U)) ||
        !request->address.lo || (request->address.lo&0x80000000U) ||
        (mode24 && request->address.lo>=0x1000000U) ||
        request->length>132U) return 8U;
    gate.u_tables=&u_tables;
    gate.real_aperture=(unsigned char *)TSF_KAPERTURE_VA;
    gate.real_bytes=TSF_REAL_BYTES;
    gate.rights=user_rights;
    gate.rights_context=0;
    copy=*request;
    copy.address.hi=0U;
    copy.direction=TSG_READ;
    if (TSGCOPY(&gate,&copy,message,sizeof message)!=TSG_OK) return 8U;
    status=cms_line_screen(message,request->length);
    if (status) return status;
    *(volatile unsigned int *)receipt_base+=1U;
    *(volatile unsigned int *)(receipt_base+4U)=request->length;
    for (i=0U; i<request->length; ++i) receipt[i]=message[i];
    return 0U;
}

/* The fixed-origin CMS24 entry has its own SVC and flagged 24-bit
 * line-buffer convention. Keep it distinct from the CMS31 CMSCALL path. */
static unsigned int cms24_native_service(TSGREQUEST *request)
{
    static const unsigned char line[8] =
        {0xe3U,0xe8U,0xd7U,0xd3U,0xc9U,0xd5U,0x40U,0x40U};
    TSGCONTEXT gate;
    TSGREQUEST copy;
    unsigned char plist[16], message[130];
    unsigned int address, length, encoded, n, i;
    volatile unsigned char *out;
    if (request->address.hi || request->address.lo<0x20000U ||
        request->address.lo>=0x1000000U) return 12U;
    gate.u_tables=&u_tables;
    gate.real_aperture=(unsigned char *)TSF_KAPERTURE_VA;
    gate.real_bytes=TSF_REAL_BYTES;
    gate.rights=user_rights;
    gate.rights_context=0;
    copy=*request;
    copy.length=sizeof plist;
    copy.direction=TSG_READ;
    if (TSGCOPY(&gate,&copy,plist,sizeof plist)!=TSG_OK) return 12U;
    for (i=0U; i<8U && plist[i]==line[i]; ++i) {}
    if (i!=8U) return 12U;
    encoded=cms_word(plist+8U);
    length=cms_word(plist+12U);
    if ((encoded&0xff000000U)!=0x01000000U ||
        (length&0xffff0000U)!=0xc2800000U) return 12U;
    address=encoded&0x00ffffffU;
    length&=0xffffU;
    if (address<0x20000U || address>=0x1000000U ||
        length>130U || length>0x1000000U-address) return 12U;
    if (length) {
        copy.address.hi=0U;
        copy.address.lo=address;
        copy.length=length;
        if (TSGCOPY(&gate,&copy,message,sizeof message)!=TSG_OK)
            return 12U;
    }
    n=*(volatile unsigned int *)0x4300U;
    if (n>=128U) return 12U;
    out=n==0U ? (volatile unsigned char *)0x4304U :
        (volatile unsigned char *)(TSF_KAPERTURE_VA+
          CMS24_EXTRA_LINES_REAL+4U+(n-1U)*136U);
    *(volatile unsigned int *)out=length;
    for (i=0U; i<length; ++i) out[4U+i]=message[i];
    *(volatile unsigned int *)0x4300U=n+1U;
    if (n) *(volatile unsigned int *)(TSF_KAPERTURE_VA+
             CMS24_EXTRA_LINES_REAL)=n;
    return cms_line_screen(message,length);
}

/* First live CMS31 CMSCALL subset. The saved old PSW selects the mapped
 * module, and every U parameter is copied through the checked K gate. */
static unsigned int cms_native_service(TSGREQUEST *request,
                                       unsigned int profile)
{
    static const unsigned char obtain[8] =
        {0xc4U,0xd4U,0xe2U,0xc6U,0xd9U,0xd6U,0xe2U,0xe5U};
    static const unsigned char release[8] =
        {0xc4U,0xd4U,0xe2U,0xc6U,0xd9U,0xd9U,0xe2U,0xe5U};
    static const unsigned char line[8] =
        {0xd3U,0xc9U,0xd5U,0xc5U,0xe6U,0xd9U,0xe3U,0x40U};
    static const unsigned char state[8] =
        {0xe2U,0xe3U,0xc1U,0xe3U,0xc5U,0x40U,0x40U,0x40U};
    static const unsigned char rdbuf[8] =
        {0xd9U,0xc4U,0xc2U,0xe4U,0xc6U,0x40U,0x40U,0x40U};
    static const unsigned char finis[8] =
        {0xc6U,0xc9U,0xd5U,0xc9U,0xe2U,0x40U,0x40U,0x40U};
    static const unsigned char wrbuf[8] =
        {0xe6U,0xd9U,0xc2U,0xe4U,0xc6U,0x40U,0x40U,0x40U};
    static const unsigned char erase[8] =
        {0xc5U,0xd9U,0xc1U,0xe2U,0xc5U,0x40U,0x40U,0x40U};
    TSGCONTEXT gate;
    TSGREQUEST copy;
    TSPADDR minimum, maximum, address;
    unsigned char plist[44], message[256], fst[40];
    unsigned int i, n, length, result, flags, cursor, size, real;
    unsigned int records, handle;
    const TSVFRAME *frame=TSVTOP(&native_invocations);
    CMSHEAP *heap=frame ? &cms_heaps[native_invocations.depth-1U] : 0;
    CMSOUTPUT *file;
    TSIINPUT *input=0;
    volatile unsigned char *out;
    if ((profile!=24U && profile!=31U) || request->address.hi ||
        request->address.lo < (profile==24U ? 0x20000U : 0x1000000U) ||
        request->address.lo >= (profile==24U ? 0x1000000U : 0x80000000U))
        return 12U;
    gate.u_tables=&u_tables;
    gate.real_aperture=(unsigned char *)TSF_KAPERTURE_VA;
    gate.real_bytes=TSF_REAL_BYTES;
    gate.rights=user_rights;
    gate.rights_context=0;
    copy=*request;
    copy.length=32U;
    copy.direction=TSG_READ;
    if (TSGCOPY(&gate,&copy,plist,sizeof plist) != TSG_OK) return 12U;
    if (profile==24U) {
        static const unsigned char typlin[8] =
            {0xe3U,0xe8U,0xd7U,0xd3U,0xc9U,0xd5U,0x40U,0x40U};
        for (i=0U; i<8U && plist[i]==typlin[i]; ++i) {}
        if (i==8U) return cms24_native_service(request);
    }
    flags=*(volatile const unsigned int *)0x307cU;
    for (i=0U; i<8U && plist[i]==obtain[i]; ++i) {}
    if (i==8U && profile==31U) {
        length=cms_word(plist+16U);
        if (!frame || !heap || flags!=0x00e00000U || !length ||
            (length&7U) ||
            length!=request->length || plist[28U]!=0x82U ||
            plist[30U]!=3U || heap->handle) return 12U;
        minimum.hi=maximum.hi=0U;
        minimum.lo=0x08000000U; maximum.lo=0x7fffffffU;
        result=TSMALLOC(&storage,frame->runtime_owner,31U,minimum,maximum,
                        length,0,&address);
        if (result!=TSM_OK) return result==TSM_NOMEM ? 4U : 12U;
        handle=allocation_handle(frame->runtime_owner,address);
        if (!handle || TSVOWN(&native_invocations,frame->token,
                              TSV_ALLOCATION,handle)!=TSV_OK) {
            if (TSMFREE(&storage,frame->runtime_owner,address)!=TSM_OK)
                return 12U;
            return 4U;
        }
        heap->address=address;
        heap->bytes=length;
        heap->handle=handle;
        *(volatile unsigned int *)0x4514U=length;
        request->address=address;
        *(volatile unsigned int *)0x4510U=1U;
        *(volatile unsigned int *)0x4098U=u_tables.used;
        return 0U;
    }
    for (i=0U; i<8U && plist[i]==release[i]; ++i) {}
    if (i==8U && profile==31U) {
        address.hi=0U;
        address.lo=*(volatile const unsigned int *)0x3044U & 0x7fffffffU;
        if (!frame || !heap || flags!=0x00e00000U || !heap->handle ||
            address.lo!=heap->address.lo ||
            cms_word(plist+16U)!=heap->bytes ||
            cms_word(plist+24U)!=8U ||
            plist[28U]!=8U || plist[29U]!=2U ||
            TSVHAS(&native_invocations,frame->token,TSV_ALLOCATION,
                   heap->handle)!=TSV_OK ||
            TSMFREE(&storage,frame->runtime_owner,address)!=TSM_OK ||
            TSVFORGET(&native_invocations,frame->token,TSV_ALLOCATION,
                      heap->handle)!=TSV_OK) return 12U;
        heap->address.hi=0U;
        heap->address.lo=0U;
        heap->bytes=0U;
        heap->handle=0U;
        *(volatile unsigned int *)0x4510U=0U;
        *(volatile unsigned int *)0x4098U=u_tables.used;
        return 0U;
    }
    for (i=0U; i<8U && plist[i]==line[i]; ++i) {}
    if (i==8U && profile==31U) {
        length=cms_word(plist+12U);
        if (flags || length>130U || cms_word(plist+8U)<0x1000000U ||
            cms_word(plist+8U)>=0x80000000U) return 12U;
        if (length) {
            copy.address.hi=0U;
            copy.address.lo=cms_word(plist+8U);
            copy.length=length;
            if (TSGCOPY(&gate,&copy,message,sizeof message)!=TSG_OK)
                return 12U;
        }
        n=*(volatile unsigned int *)(TSF_KAPERTURE_VA+CMS31_LINES_REAL);
        if (n>=128U) return 12U;
        out=(volatile unsigned char *)(TSF_KAPERTURE_VA+
                                       CMS31_LINES_REAL+4U+n*136U);
        *(volatile unsigned int *)out=length;
        for (i=0U; i<length; ++i) out[4U+i]=message[i];
        *(volatile unsigned int *)(TSF_KAPERTURE_VA+CMS31_LINES_REAL)=n+1U;
        return cms_line_screen(message,length);
    }
    for (i=0U; i<8U && plist[i]==state[i]; ++i) {}
    if (i==8U) {
        if (!cms31_lowcore_real) return 12U;
        copy.address.lo=request->address.lo+28U;
        copy.length=4U; copy.direction=TSG_WRITE;
        if (TSGPROBE(&gate,&copy)!=TSG_OK) return 12U;
        file=cms_output_find(plist+8U,profile,frame->token);
        if (file) records=file->records;
        else {
            result=cms_file_open(plist+8U,profile,&input);
            if (result) return result;
            records=input->records;
        }
        for (i=0U; i<40U; ++i) fst[i]=0U;
        for (i=0U; i<16U; ++i) fst[i]=plist[8U+i];
        fst[16U]=0x10U; fst[17U]=0x04U;
        fst[24U]=plist[24U]; fst[25U]=plist[25U];
        fst[26U]=(unsigned char)(records>>8);
        fst[27U]=(unsigned char)records;
        fst[30U]=0xe5U; cms_put_word(fst+32U,256U);
        fst[38U]=0xf2U; fst[39U]=0xf6U;
        for (i=0U; i<40U; ++i)
            ((volatile unsigned char *)TSF_KAPERTURE_VA)
                [cms31_lowcore_real+0x300U+i]=fst[i];
        cms_put_word(plist+28U,0x300U);
        copy.address.lo=request->address.lo+28U;
        copy.length=4U; copy.direction=TSG_WRITE;
        return TSGCOPY(&gate,&copy,plist+28U,4U)==TSG_OK ? 0U : 12U;
    }
    for (i=0U; i<8U && plist[i]==rdbuf[i]; ++i) {}
    if (i==8U) {
        copy=*request; copy.length=44U; copy.direction=TSG_READ;
        if (TSGCOPY(&gate,&copy,plist,sizeof plist)!=TSG_OK) return 12U;
        copy.address.lo=request->address.lo+40U;
        copy.length=4U; copy.direction=TSG_WRITE;
        if (TSGPROBE(&gate,&copy)!=TSG_OK) return 12U;
        file=cms_output_find(plist+8U,profile,frame->token);
        if (file) {
            real=file->real;
            cursor=(plist[26U]==0U && plist[27U]==1U) ?
                   64U : file->cursor;
            length=file->length;
        } else {
            result=cms_file_open(plist+8U,profile,&input);
            if (result) return result;
            real=input->real;
            cursor=(plist[26U]==0U && plist[27U]==1U) ?
                   64U : input->cursor;
            length=input->length;
        }
        if (cursor>=length || cursor>length-2U) return 12U;
        size=((unsigned int)((unsigned char *)TSF_KAPERTURE_VA+
              real)[cursor]<<8) |
             ((unsigned char *)TSF_KAPERTURE_VA+
              real)[cursor+1U];
        if (!size || size>256U || size>length-cursor-2U ||
            cms_word(plist+28U)<
                (profile==24U ? 0x20000U : 0x1000000U) ||
            cms_word(plist+28U)>=
                (profile==24U ? 0x1000000U : 0x80000000U) ||
            cms_word(plist+32U)<size) return 12U;
        for (i=0U; i<size; ++i)
            message[i]=((unsigned char *)TSF_KAPERTURE_VA+
                        real)[cursor+2U+i];
        copy.address.lo=cms_word(plist+28U);
        copy.length=size; copy.direction=TSG_WRITE;
        if (TSGPROBE(&gate,&copy)!=TSG_OK) return 12U;
        if (TSGCOPY(&gate,&copy,message,sizeof message)!=TSG_OK)
            return 12U;
        cms_put_word(plist+40U,size);
        copy.address.lo=request->address.lo+40U;
        copy.length=4U;
        if (TSGCOPY(&gate,&copy,plist+40U,4U)!=TSG_OK)
            return 12U;
        if (file) file->cursor=cursor+2U+size;
        else input->cursor=cursor+2U+size;
        return 0U;
    }
    for (i=0U; i<8U && plist[i]==wrbuf[i]; ++i) {}
    if (i==8U) {
        copy=*request; copy.length=44U; copy.direction=TSG_READ;
        if (TSGCOPY(&gate,&copy,plist,sizeof plist)!=TSG_OK) return 12U;
        length=cms_word(plist+32U);
        if (!length || length>256U ||
            cms_word(plist+28U)<
                (profile==24U ? 0x20000U : 0x1000000U) ||
            cms_word(plist+28U)>=
                (profile==24U ? 0x1000000U : 0x80000000U)) return 12U;
        copy.address.lo=cms_word(plist+28U);
        copy.length=length;
        if (TSGCOPY(&gate,&copy,message,sizeof message)!=TSG_OK)
            return 12U;
        file=cms_output_find(plist+8U,profile,frame->token);
        if (!file) {
            result=cms_file_open(plist+8U,profile,&input);
            if (result!=28U) return 12U;
            file=cms_output_create(plist+8U,profile,frame->token);
            if (!file) return 4U;
        }
        if (file->closed ||
            TSVHAS(&native_invocations,frame->token,TSV_FILE,
                   CMS_OUTPUT_HANDLE_BASE+
                   (unsigned int)(file-cms_outputs)+1U)!=TSV_OK ||
            file->records>=65535U ||
            file->length>CMS_OUTPUT_BYTES-length-2U) return 12U;
        ((unsigned char *)TSF_KAPERTURE_VA+file->real)[file->length++]=
            (unsigned char)(length>>8);
        ((unsigned char *)TSF_KAPERTURE_VA+file->real)[file->length++]=
            (unsigned char)length;
        for (i=0U; i<length; ++i)
            ((unsigned char *)TSF_KAPERTURE_VA+file->real)[file->length+i]=
                message[i];
        file->length+=length; file->source_bytes+=length;
        ++file->records;
        return 0U;
    }
    for (i=0U; i<8U && plist[i]==finis[i]; ++i) {}
    if (i==8U) {
        file=cms_output_find(plist+8U,profile,frame->token);
        if (file) {
            unsigned int slot=(unsigned int)(file-cms_outputs);
            unsigned int handle=CMS_OUTPUT_HANDLE_BASE+slot+1U;
            if (!file->closed &&
                TSVHAS(&native_invocations,frame->token,TSV_FILE,handle)
                    !=TSV_OK) return 12U;
            if (!file->records) {
                ((unsigned char *)TSF_KAPERTURE_VA+file->real)[64U]=0U;
                ((unsigned char *)TSF_KAPERTURE_VA+file->real)[65U]=1U;
                ((unsigned char *)TSF_KAPERTURE_VA+file->real)[66U]=0x40U;
                file->length=67U; file->records=file->source_bytes=1U;
            }
            if (!file->closed &&
                TSVFORGET(&native_invocations,frame->token,TSV_FILE,handle)
                    !=TSV_OK) return 12U;
            file->closed=1U;
            return 0U;
        }
        input=TSIFINDOWNED(&cms_inputs,plist+8U,profile,
                          frame ? frame->token : 0U);
        return input ? cms_file_close(input) : 28U;
    }
    for (i=0U; i<8U && plist[i]==erase[i]; ++i) {}
    if (i==8U) {
        file=cms_output_find(plist+8U,profile,frame->token);
        return file ? cms_output_erase(file) : 28U;
    }
    {
        volatile unsigned char *diag=(volatile unsigned char *)(
            TSF_KAPERTURE_VA+0x25000U);
        ++*(volatile unsigned int *)diag;
        for (i=0U; i<8U; ++i) diag[4U+i]=plist[i];
        *(volatile unsigned int *)(diag+12U)=flags;
    }
    return 12U;
}

static unsigned int cms31_library_probe(const TSGREQUEST *request)
{
    static const unsigned char name[] = {
        0xc3U,0xd4U,0xe2U,0xf3U,0xf1U,0x4bU,
        0xd3U,0xc9U,0xc2U,0xd9U,0xc1U,0xd9U,0xe8U,0x4bU,
        0xd9U,0xe7U,0xc2U,0xc9U,0xd5U
    };
    TSKEXTENT extent;
    int result;
    if (request->length || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    result=TSKFIND(channel_record,&channel,name,sizeof name,&extent);
    if (result==TSK_ABSENT) return 4U;
    return result==TSK_OK && extent.record_format==0x80U &&
           extent.block_length==18452U &&
           extent.logical_length==18452U ? 0U : 12U;
}

static unsigned int cms24_io24_probe(const TSGREQUEST *request)
{
    static const unsigned char name[] = {
        0xc3U,0xd4U,0xe2U,0xf2U,0xf4U,0x4bU,
        0xc9U,0xd6U,0xf2U,0xf4U,0x4bU,
        0xd9U,0xe7U,0xc2U,0xc9U,0xd5U
    };
    TSKEXTENT extent;
    int result;
    if (request->length || request->address.hi || request->address.lo ||
        request->direction) return 8U;
    result=TSKFIND(channel_record,&channel,name,sizeof name,&extent);
    if (result==TSK_ABSENT) return 4U;
    return result==TSK_OK && extent.record_format==0x80U &&
           extent.block_length==18452U &&
           extent.logical_length==18452U ? 0U : 12U;
}

/* Fixture completion receipt. Keep hashes in K-only low real diagnostics,
 * then release each transient file buffer before the next workload. */
static unsigned int cms_file_audit(const TSGREQUEST *request,
                                   unsigned int profile)
{
    volatile unsigned char *receipt=(volatile unsigned char *)(
        TSF_KAPERTURE_VA+(profile==24U ? 0x25580U : 0x25500U));
    unsigned int slot, i, count=0U, hash, result=0U;
    CMSOUTPUT *out;
    if (request->length || request->address.hi || request->address.lo ||
        request->direction || TSVTOP(&native_invocations)) return 8U;
    for (slot=0U; slot<CMS_OUTPUT_SLOTS; ++slot) {
        out=&cms_outputs[slot];
        if (!out->real || out->profile!=profile) continue;
        if (!out->closed) result=12U;
        hash=0x811c9dc5U;
        for (i=64U; i<out->length; ++i)
            hash=(hash^((unsigned char *)TSF_KAPERTURE_VA+
                         out->real)[i])*0x01000193U;
        for (i=0U; i<18U; ++i)
            receipt[slot*32U+i]=out->id[i];
        *(volatile unsigned int *)(receipt+slot*32U+20U)=out->records;
        *(volatile unsigned int *)(receipt+slot*32U+24U)=out->source_bytes;
        *(volatile unsigned int *)(receipt+slot*32U+28U)=hash;
        ++count;
        if (cms_output_discard(out)!=0U) result=12U;
    }
    *(volatile unsigned int *)(TSF_KAPERTURE_VA+
        (profile==24U ? 0x254f4U : 0x254f0U))=count;
    for (slot=0U; slot<TSI_SLOTS; ++slot)
        if (cms_inputs.slot[slot].real &&
            cms_inputs.slot[slot].profile==profile &&
            cms_file_close(&cms_inputs.slot[slot])!=0U) result=12U;
    return result;
}

unsigned int pdosTwoSpaceService(TSGREQUEST *request)
{
    TSGCONTEXT gate;
    const TSVFRAME *native_frame;
    unsigned char bytes[TSG_MAX_COPY];
    unsigned int value, native_tso, caller_pc;
    int result;
    if (!request) return 0xffffffffU;
    if (attach()) return 0xfffffffaU;
    caller_pc=*(volatile const unsigned int *)0x308cU;
    native_frame=native_caller(caller_pc);
    native_tso=native_frame && native_frame->personality==TSV_TSO;
    /* The descriptor stores a 32-bit length. R0 is not an argument for the
       selected AMODE31 TPUT or IARV64 PC entry. Both routes validate their
       R1 arguments independently before touching U storage. */
    if (*(volatile const unsigned int *)0x3000U != 0U &&
        !((request->svc==93U && native_tso &&
           !(*(volatile const unsigned int *)0x3080U&1U)) ||
          (request->svc==233U && native_frame==0 &&
           TSVTOP(&native_invocations) &&
           TSVTOP(&native_invocations)->amode==64U &&
           caller_pc==0x402U &&
           !(*(volatile const unsigned int *)0x3080U&1U) &&
           (*(volatile const unsigned int *)0x3084U&0x80000000U))))
        return request->svc == 120U || request->svc == 223U ?
               8U : 0xfffffffdU;
    if (request->svc == 235U) return native_begin(request);
    if (request->svc == 236U) return native_end(request);
    if (request->svc == 237U) return native_reap_probe(request);
    if (request->svc == 239U) return terminal_clear_probe(request);
    if (request->svc == 240U) return terminal_cancel_probe(request);
    if (request->svc == 241U) return terminal_phase_probe(request);
    if (request->svc == 120U) return storage_service(request);
    if (request->svc == 223U) return high_storage_service(request);
    if (request->svc == 233U) return iarv64_service(request);
    if (request->svc == 202U && native_frame &&
        native_frame->personality==TSV_CMS &&
        native_frame->amode==24U)
        return cms_native_service(request,24U);
    if (request->svc == 204U && native_frame &&
        native_frame->personality==TSV_CMS &&
        native_frame->amode==31U)
        return cms_native_service(request,31U);
    if (request->svc == 205U &&
        TSVTOP(&native_invocations) &&
        TSVTOP(&native_invocations)->personality==TSV_CMS &&
        TSVTOP(&native_invocations)->amode==31U &&
        *(volatile const unsigned int *)0x308cU==0x202U)
        return cms31_fst_lookup(request);
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
    if (request->svc == 227U) return cms24_fresh_push(request);
    if (request->svc == 228U) return cms24_io24_probe(request);
    if (request->svc == 222U) return absent_channel_probe(request);
    if (request->svc == 224U) return cms31_secondary_map_service(request);
    if (request->svc == 225U) return cms31_library_probe(request);
    if (request->svc == 226U) return cms_file_audit(request,31U);
    if (request->svc == 229U) return cms_file_audit(request,24U);
    if (request->svc == 230U) return cms_cursor_guest_probe(request);
    if (request->svc == 231U) return tso_map_service(request,31U);
    if (request->svc == 232U) return tso_map_service(request,64U);
    if (request->svc == 234U) return tso_map_service(request,24U);
    if (request->svc == 93U && native_tso)
        return tso_terminal_service(request);
    if (request->svc != 1U && request->svc != 202U &&
        request->svc != 204U && request->svc != 205U)
    {
        if (native_tso &&
            *(volatile unsigned int *)(caller_pc>=0x09000000U ?
                0x4d08U : caller_pc>=0x07000000U ?
                0x4708U : 0x4c08U)==0U) {
            *(volatile unsigned int *)(caller_pc>=0x09000000U ?
                0x4d08U : caller_pc>=0x07000000U ?
                0x4708U : 0x4c08U)=request->svc;
            *(volatile unsigned int *)(caller_pc>=0x09000000U ?
                0x4d0cU : caller_pc>=0x07000000U ?
                0x470cU : 0x4c0cU)=caller_pc;
        }
        return 0xfffffffbU;
    }
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
