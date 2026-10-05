/* SPDX-License-Identifier: MIT */
#include "twospace_memory.h"

static int same(TSPADDR a, TSPADDR b)
{ return a.hi == b.hi && a.lo == b.lo; }

static int rounded(unsigned int bytes, unsigned int *pages)
{
    if (!bytes || bytes > 0xfffff000U) return 0;
    *pages = (bytes + 4095U) & ~4095U;
    return *pages != 0U;
}

static TSPADDR plus_page(TSPADDR a, unsigned int offset)
{
    unsigned int before = a.lo;
    a.lo += offset;
    if (a.lo < before) ++a.hi;
    return a;
}

int TSMINIT(TSMSTATE *s, TSDSTATE *u, unsigned char *aperture,
            unsigned int real_bytes, unsigned int first_free_real,
            TSMKEY set_key, void *key_context)
{
    unsigned int i;
    if (!s || !u || !aperture || !set_key || !real_bytes ||
        (real_bytes & 4095U) || !first_free_real ||
        (first_free_real & 4095U) || first_free_real >= real_bytes ||
        TSRINIT(&s->real,real_bytes) != TSR_OK ||
        TSRRESERVE(&s->real,1U,0U,first_free_real,TSR_RUN) != TSR_OK)
        return TSM_BAD;
    s->u_tables=u; s->real_aperture=aperture;
    s->real_bytes=real_bytes; s->first_free_real=first_free_real;
    s->set_key=set_key; s->key_context=key_context;
    s->next_handle=2U;
    s->depth=0U;
    TSPINIT(&s->virtuals);
    for (i=0U; i<TSM_ALLOCS; ++i) s->allocations[i].handle=0U;
    return TSM_OK;
}

int TSMRESERVE(TSMSTATE *s, unsigned int owner, unsigned int mode,
               TSPADDR address, unsigned int bytes)
{
    if (!s || !owner || owner >= 0x80000000U) return TSM_BAD;
    return TSPRESV(&s->virtuals,owner | 0x80000000U,mode,address,bytes);
}

int TSMALLOC(TSMSTATE *s, unsigned int task, unsigned int mode,
             TSPADDR minimum, TSPADDR maximum, unsigned int bytes,
             int fixed, TSPADDR *address)
{
    TSPADDR at, va, pa, old;
    unsigned int span, real, i, mapped=0U, slot, handle;
    int rc;
    if (!s || !s->u_tables || !address || !task ||
        (mode != 24U && mode != 31U && mode != 64U) ||
        !rounded(bytes,&span) || s->next_handle >= 0x80000000U)
        return TSM_BAD;
    for (slot=0U; slot<TSM_ALLOCS; ++slot)
        if (!s->allocations[slot].handle) break;
    if (slot == TSM_ALLOCS) return TSM_NOMEM;
    if (fixed) at=minimum;
    else {
        rc=TSPFIND(&s->virtuals,mode,minimum,maximum,span,&at);
        if (rc != TSP_OK) return rc == TSP_FULL ? TSM_NOMEM : TSM_BAD;
    }
    handle=s->next_handle++;
    rc=TSPRESV(&s->virtuals,handle,mode,at,span);
    if (rc != TSP_OK)
        return rc == TSP_COLLISION ? TSM_COLLISION : TSM_BAD;
    rc=TSRALLOC(&s->real,handle,span,s->first_free_real,
                s->real_bytes,TSR_RUN,&real);
    if (rc != TSR_OK) {
        TSPRELS(&s->virtuals,handle);
        return TSM_NOMEM;
    }
    /* Prepare real storage before any U PTE exposes it. */
    for (i=0U; i<span; i+=4096U) {
        unsigned int j;
        for (j=0U; j<4096U; ++j) s->real_aperture[real+i+j]=0U;
        s->set_key(real+i,0x80U,s->key_context);
    }
    for (i=0U; i<span; i+=4096U) {
        va=plus_page(at,i); pa.hi=0U; pa.lo=real+i;
        if (TSDMAP(s->u_tables,va,pa) != TSD_OK) break;
        ++mapped;
    }
    if (i != span) {
        while (mapped) {
            --mapped;
            va=plus_page(at,mapped*4096U);
            if (TSDUNMAP(s->u_tables,va,&old) != TSD_OK ||
                old.hi || old.lo != real+mapped*4096U)
                return TSM_CORRUPT;
        }
        for (i=0U; i<span; i+=4096U)
            s->set_key(real+i,0U,s->key_context);
        TSRRELEASE(&s->real,handle,real);
        TSPRELS(&s->virtuals,handle);
        return TSM_NOMEM;
    }
    s->allocations[slot].handle=handle;
    s->allocations[slot].task=task;
    s->allocations[slot].mode=mode;
    s->allocations[slot].bytes=span;
    s->allocations[slot].real=real;
    s->allocations[slot].address=at;
    *address=at;
    return TSM_OK;
}

int TSMFREE(TSMSTATE *s, unsigned int task, TSPADDR address)
{
    unsigned int slot, i, j, real, bytes, handle;
    TSPADDR va, pa;
    if (!s || !task) return TSM_BAD;
    for (slot=0U; slot<TSM_ALLOCS; ++slot)
        if (s->allocations[slot].handle &&
            same(s->allocations[slot].address,address)) break;
    if (slot == TSM_ALLOCS) return TSM_ABSENT;
    if (s->allocations[slot].task != task) return TSM_BAD;
    if (s->depth && same(s->overlay[s->depth-1U].address,address))
        return TSM_COLLISION;
    real=s->allocations[slot].real;
    bytes=s->allocations[slot].bytes;
    handle=s->allocations[slot].handle;
    /* Preflight every mapping before the first unmap. */
    for (i=0U; i<bytes; i+=4096U) {
        va=plus_page(address,i);
        if (TSDLOOKUP(s->u_tables,va,&pa) != TSD_OK ||
            pa.hi || pa.lo != real+i) return TSM_CORRUPT;
    }
    for (i=0U; i<bytes; i+=4096U) {
        va=plus_page(address,i);
        if (TSDUNMAP(s->u_tables,va,&pa) != TSD_OK ||
            pa.hi || pa.lo != real+i) return TSM_CORRUPT;
        for (j=0U; j<4096U; ++j) s->real_aperture[real+i+j]=0U;
        s->set_key(real+i,0U,s->key_context);
    }
    if (TSRRELEASE(&s->real,handle,real) != TSR_OK ||
        TSPRELS(&s->virtuals,handle) != TSP_OK) return TSM_CORRUPT;
    s->allocations[slot].handle=0U;
    return TSM_OK;
}

static int mapping_is(const TSDSTATE *tables, TSPADDR address,
                      unsigned int real, unsigned int bytes)
{
    unsigned int i;
    TSPADDR va, pa;
    for (i=0U; i<bytes; i+=4096U) {
        va=plus_page(address,i);
        if (TSDLOOKUP(tables,va,&pa) != TSD_OK ||
            pa.hi || pa.lo != real+i) return 0;
    }
    return 1;
}

static int replace(TSMSTATE *s, TSPADDR address, unsigned int bytes,
                   unsigned int from, unsigned int to)
{
    unsigned int i, j;
    TSPADDR va, pa;
    if (!mapping_is(s->u_tables,address,from,bytes)) return TSM_CORRUPT;
    pa.hi=0U;
    for (i=0U; i<bytes; i+=4096U) {
        va=plus_page(address,i);
        if (TSDUNMAP(s->u_tables,va,&pa) != TSD_OK ||
            pa.hi || pa.lo != from+i) return TSM_CORRUPT;
        pa.hi=0U; pa.lo=to+i;
        if (TSDMAP(s->u_tables,va,pa) == TSD_OK) continue;
        pa.lo=from+i;
        if (TSDMAP(s->u_tables,va,pa) != TSD_OK) return TSM_CORRUPT;
        for (j=i; j; j-=4096U) {
            va=plus_page(address,j-4096U);
            if (TSDUNMAP(s->u_tables,va,&pa) != TSD_OK ||
                pa.hi || pa.lo != to+j-4096U) return TSM_CORRUPT;
            pa.hi=0U; pa.lo=from+j-4096U;
            if (TSDMAP(s->u_tables,va,pa) != TSD_OK) return TSM_CORRUPT;
        }
        return TSM_NOMEM;
    }
    return TSM_OK;
}

int TSMOVERLAYPUSH(TSMSTATE *s, unsigned int task, TSPADDR address,
                   const unsigned char *image, unsigned int image_bytes)
{
    unsigned int i, j, slot, bytes, parent, child, handle;
    int rc;
    if (!s || !task || !image || !image_bytes ||
        s->depth == TSM_OVERLAYS || s->next_handle >= 0x80000000U)
        return TSM_BAD;
    if (s->depth) {
        const TSMOVERLAY *top=&s->overlay[s->depth-1U];
        if (top->task != task || !same(top->address,address))
            return TSM_COLLISION;
        bytes=top->bytes; parent=top->child_real;
    } else {
        for (slot=0U; slot<TSM_ALLOCS; ++slot)
            if (s->allocations[slot].handle &&
                same(s->allocations[slot].address,address)) break;
        if (slot == TSM_ALLOCS) return TSM_ABSENT;
        if (s->allocations[slot].task != task) return TSM_BAD;
        bytes=s->allocations[slot].bytes;
        parent=s->allocations[slot].real;
    }
    if (image_bytes > bytes || !mapping_is(s->u_tables,address,parent,bytes))
        return TSM_BAD;
    handle=s->next_handle++;
    rc=TSRALLOC(&s->real,handle,bytes,s->first_free_real,
                s->real_bytes,TSR_RUN,&child);
    if (rc != TSR_OK) return TSM_NOMEM;
    for (i=0U; i<bytes; ++i)
        s->real_aperture[child+i]=i<image_bytes ? image[i] : 0U;
    for (i=0U; i<bytes; i+=4096U)
        s->set_key(child+i,0x80U,s->key_context);
    rc=replace(s,address,bytes,parent,child);
    if (rc != TSM_OK) {
        for (j=0U; j<bytes; ++j) s->real_aperture[child+j]=0U;
        for (j=0U; j<bytes; j+=4096U)
            s->set_key(child+j,0U,s->key_context);
        if (TSRRELEASE(&s->real,handle,child) != TSR_OK)
            return TSM_CORRUPT;
        return rc;
    }
    i=s->depth++;
    s->overlay[i].handle=handle;
    s->overlay[i].task=task;
    s->overlay[i].parent_real=parent;
    s->overlay[i].child_real=child;
    s->overlay[i].bytes=bytes;
    s->overlay[i].address=address;
    return TSM_OK;
}

int TSMOVERLAYPOP(TSMSTATE *s, unsigned int task, TSPADDR address,
                  unsigned int child_rc, unsigned int *returned_rc)
{
    TSMOVERLAY *top;
    unsigned int i;
    int rc;
    if (!s || !task || !returned_rc || !s->depth) return TSM_BAD;
    top=&s->overlay[s->depth-1U];
    if (top->task != task || !same(top->address,address)) return TSM_BAD;
    rc=replace(s,address,top->bytes,top->child_real,top->parent_real);
    if (rc != TSM_OK) return rc;
    for (i=0U; i<top->bytes; ++i)
        s->real_aperture[top->child_real+i]=0U;
    for (i=0U; i<top->bytes; i+=4096U)
        s->set_key(top->child_real+i,0U,s->key_context);
    if (TSRRELEASE(&s->real,top->handle,top->child_real) != TSR_OK)
        return TSM_CORRUPT;
    --s->depth;
    *returned_rc=child_rc;
    return TSM_OK;
}

unsigned int TSMRIGHTS(unsigned int real_page, void *context)
{
    const TSMSTATE *s=(const TSMSTATE *)context;
    unsigned int i;
    if (!s || (real_page & 4095U)) return 0U;
    for (i=0U; i<TSM_ALLOCS; ++i) {
        const TSMALLOCATION *a=&s->allocations[i];
        if (!a->handle) continue;
        if (s->depth &&
            same(a->address,s->overlay[s->depth-1U].address)) continue;
        if (real_page >= a->real &&
            real_page-a->real < a->bytes) return 3U;
    }
    if (s->depth) {
        const TSMOVERLAY *a=&s->overlay[s->depth-1U];
        if (real_page >= a->child_real &&
            real_page-a->child_real < a->bytes) return 3U;
    }
    return 0U;
}

unsigned int TSMLOWFREE(const TSMSTATE *s)
{
    unsigned int i, used=0U;
    if (!s) return 0U;
    for (i=0U; i<TSP_SLOTS; ++i) {
        const TSPENTRY *e=&s->virtuals.entry[i];
        if (e->owner && e->first.hi == 0U &&
            e->first.lo < 0x01000000U) {
            unsigned int end=e->last.hi ? 0x00ffffffU : e->last.lo;
            if (end > 0x00ffffffU) end=0x00ffffffU;
            used += end-e->first.lo+1U;
        }
    }
    return 0x01000000U-used;
}
