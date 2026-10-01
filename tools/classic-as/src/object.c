/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Original classic object writer. Format facts: IBM z/VM 7.4 OBJSTMT,
 * https://www.ibm.com/support/pages/zvm/pubs/cp740/objstmt.html
 * (ESD/TXT/RLD/END field positions and AMODE/RMODE/sign bits).
 */
#include "mf_classic.h"
#include <string.h>

struct obj_section {
    unsigned id, esdid;
    mf_u32 length, cursor, text_start, text_end;
    unsigned amode, rmode;
    int dummy;
    mf_octet name[8];
    int named;
};
struct obj_symbol {
    unsigned id, esdid, section;
    enum mf_symbol_kind kind;
    mf_u32 offset;
    mf_octet name[8];
};
struct mf_obj {
    struct mf_sink sink;
    struct obj_section *sections;
    struct obj_symbol *symbols;
    size_t max_sections, max_symbols, section_count, symbol_count;
    enum mf_status error;
    int begun, closed;
    struct mf_entry entry;
    mf_octet text[56];
    size_t text_count;
    unsigned text_section;
    mf_u32 text_offset;
};
static enum mf_status fail(struct mf_obj *o, enum mf_status s)
{
    if (o->error == MF_OK) o->error = s;
    return o->error;
}
static void store(mf_octet *p, mf_u32 n, unsigned bytes)
{
    while (bytes != 0) { --bytes; p[bytes] = (mf_octet)(n & 255UL); n >>= 8; }
}
static void card(mf_octet *p, unsigned a, unsigned b, unsigned c)
{
    memset(p, 0x40, 80); p[0] = 2;
    p[1] = (mf_octet)a; p[2] = (mf_octet)b; p[3] = (mf_octet)c;
}
static enum mf_status emit(struct mf_obj *o, const mf_octet *p)
{
    enum mf_status s;
    if (o->error != MF_OK) return o->error;
    s = o->sink.write(o->sink.cookie, p, 80);
    return s == MF_OK ? MF_OK : fail(o, s);
}
static enum mf_status name(struct mf_span n, mf_octet *out, int empty)
{
    size_t i;
    enum mf_status s;
    if ((!n.data && n.length) || (!n.length && !empty)) return MF_OBJECT;
    if (n.length > 8) return MF_LIMIT;
    memset(out, 0x40, 8);
    for (i = 0; i < n.length; ++i) {
        if (n.data[i] < 0x21 || n.data[i] > 0x7e) return MF_UNSUPPORTED;
        s = mf_ascii_to_ebcdic(n.data[i], out + i);
        if (s != MF_OK) return s;
    }
    return MF_OK;
}
static struct obj_section *section(struct mf_obj *o, unsigned id)
{
    size_t i;
    for (i = 0; i < o->section_count; ++i)
        if (o->sections[i].id == id) return &o->sections[i];
    return NULL;
}
static struct obj_symbol *symbol(struct mf_obj *o, unsigned id)
{
    size_t i;
    for (i = 0; i < o->symbol_count; ++i)
        if (o->symbols[i].id == id) return &o->symbols[i];
    return NULL;
}
static enum mf_status active(struct mf_obj *o)
{
    if (o->error != MF_OK) return o->error;
    if (!o->begun || o->closed) return fail(o, MF_OBJECT);
    return MF_OK;
}
static enum mf_status flush(struct mf_obj *o)
{
    mf_octet p[80];
    if (!o->text_count) return o->error;
    card(p, 0xe3, 0xe7, 0xe3);
    store(p + 5, o->text_offset, 3); store(p + 10, (mf_u32)o->text_count, 2);
    store(p + 14, o->text_section, 2);
    memcpy(p + 16, o->text, o->text_count);
    o->text_count = 0;
    return emit(o, p);
}
static enum mf_status begin(void *cookie, const struct mf_section *secs,
    size_t ns, const struct mf_symbol *syms, size_t ny)
{
    struct mf_obj *o = (struct mf_obj *)cookie;
    struct obj_section *s, *owner;
    struct obj_symbol *y;
    size_t i, j;
    mf_u32 next = 1;
    enum mf_status st;
    mf_octet p[80];
    if (o->error != MF_OK) return o->error;
    if (o->begun || o->closed) return fail(o, MF_OBJECT);
    o->begun = 1;
    if (ns > o->max_sections || ny > o->max_symbols) return fail(o, MF_LIMIT);
    if ((!secs && ns) || (!syms && ny)) return fail(o, MF_OBJECT);
    o->section_count = ns; o->symbol_count = ny;
    for (i = 0; i < ns; ++i) {
        s = &o->sections[i];
        memset(s, 0, sizeof(*s));
        s->id = secs[i].id; s->length = secs[i].length;
        s->amode = secs[i].amode; s->rmode = secs[i].rmode;
        s->dummy = secs[i].dummy; s->named = secs[i].name.length != 0;
        if (!s->id) return fail(o, MF_OBJECT);
        for (j = 0; j < i; ++j)
            if (o->sections[j].id == s->id) return fail(o, MF_DUPLICATE);
        if (s->length > 0xffffffUL) return fail(o, MF_RANGE);
        if ((s->amode != 0 && s->amode != 24 && s->amode != 31) ||
            (s->rmode != 24 && s->rmode != 31)) return fail(o, MF_UNSUPPORTED);
        if (!s->dummy) {
            st = name(secs[i].name, s->name, 1);
            if (st != MF_OK) return fail(o, st);
        }
        for (j = 0; j < i; ++j)
            if (!s->dummy && s->named && !o->sections[j].dummy &&
                o->sections[j].named && !memcmp(s->name, o->sections[j].name, 8))
                return fail(o, MF_DUPLICATE);
        if (!s->dummy) {
            if (next > 65535UL) return fail(o, MF_LIMIT);
            s->esdid = (unsigned)next++;
        }
    }
    for (i = 0; i < ny; ++i) {
        y = &o->symbols[i]; memset(y, 0, sizeof(*y));
        y->id = syms[i].id; y->kind = syms[i].kind;
        y->section = syms[i].section; y->offset = syms[i].offset;
        if (!y->id) return fail(o, MF_OBJECT);
        for (j = 0; j < i; ++j)
            if (o->symbols[j].id == y->id) return fail(o, MF_DUPLICATE);
        if (y->kind == MF_LOCAL) continue;
        if (y->kind != MF_EXPORT && y->kind != MF_EXTERNAL)
            return fail(o, MF_OBJECT);
        st = name(syms[i].name, y->name, 0);
        if (st != MF_OK) return fail(o, st);
        for (j = 0; j < i; ++j)
            if (o->symbols[j].kind == y->kind &&
                !memcmp(y->name, o->symbols[j].name, 8)) return fail(o, MF_DUPLICATE);
        for (j = 0; j < ns; ++j)
            if (y->kind != MF_EXTERNAL && !o->sections[j].dummy && o->sections[j].named &&
                !memcmp(y->name, o->sections[j].name, 8)) return fail(o, MF_DUPLICATE);
        if (y->kind == MF_EXPORT) {
            owner = section(o, y->section);
            if (!owner || owner->dummy || y->offset >= owner->length)
                return fail(o, MF_RANGE);
        } else {
            if (y->section || y->offset) return fail(o, MF_OBJECT);
            if (next > 65535UL) return fail(o, MF_LIMIT);
            y->esdid = (unsigned)next++;
        }
    }
    st = o->sink.begin(o->sink.cookie);
    if (st != MF_OK) return fail(o, st);
    /* One independently addressable ESD item per card; LD has no ESD ID. */
    for (i = 0; i < ns; ++i) {
        s = &o->sections[i]; if (s->dummy) continue;
        card(p, 0xc5, 0xe2, 0xc4); store(p + 10, 16, 2);
        store(p + 14, s->esdid, 2); memcpy(p + 16, s->name, 8);
        p[24] = (mf_octet)(s->named ? 0 : 4); store(p + 25, 0, 3);
        p[28] = (mf_octet)((s->amode == 0 ? 3 : s->amode == 24 ? 1 : 2) |
            (s->rmode == 31 ? 4 : 0)); store(p + 29, s->length, 3);
        if (emit(o, p) != MF_OK) return o->error;
    }
    for (i = 0; i < ny; ++i) {
        y = &o->symbols[i]; if (y->kind != MF_EXTERNAL) continue;
        card(p, 0xc5, 0xe2, 0xc4); store(p + 10, 16, 2);
        store(p + 14, y->esdid, 2); memcpy(p + 16, y->name, 8); p[24] = 2;
        if (emit(o, p) != MF_OK) return o->error;
    }
    for (i = 0; i < ny; ++i) {
        y = &o->symbols[i]; if (y->kind != MF_EXPORT) continue;
        owner = section(o, y->section);
        card(p, 0xc5, 0xe2, 0xc4); store(p + 10, 16, 2);
        memcpy(p + 16, y->name, 8); p[24] = 1;
        store(p + 25, y->offset, 3); p[28] = 0x40;
        store(p + 29, owner->esdid, 3);
        if (emit(o, p) != MF_OK) return o->error;
    }
    return MF_OK;
}
static enum mf_status text(void *cookie, unsigned id, mf_u32 off,
    const mf_octet *bytes, size_t length)
{
    struct mf_obj *o = (struct mf_obj *)cookie;
    struct obj_section *s;
    size_t n;
    if (active(o) != MF_OK) return o->error;
    s = section(o, id);
    if (!s || s->dummy) return fail(o, MF_OBJECT);
    if (off > s->length || length > s->length - off) return fail(o, MF_RANGE);
    if (off < s->cursor) return fail(o, MF_UNSUPPORTED);
    if (!bytes && length) return fail(o, MF_OBJECT);
    if (length) {
        if (off != s->text_end) s->text_start = off;
        s->text_end = off + (mf_u32)length;
    }
    if (o->text_count && (o->text_section != s->esdid ||
        off != o->text_offset + (mf_u32)o->text_count))
        if (flush(o) != MF_OK) return o->error;
    s->cursor = off + (mf_u32)length;
    while (length) {
        if (!o->text_count) { o->text_section = s->esdid; o->text_offset = off; }
        n = 56 - o->text_count; if (n > length) n = length;
        memcpy(o->text + o->text_count, bytes, n);
        o->text_count += n; bytes += n; length -= n; off += (mf_u32)n;
        if (o->text_count == 56 && flush(o) != MF_OK) return o->error;
    }
    return MF_OK;
}
static enum mf_status gap(void *cookie, unsigned id, mf_u32 off, mf_u32 length)
{
    struct mf_obj *o = (struct mf_obj *)cookie;
    struct obj_section *s;
    if (active(o) != MF_OK) return o->error;
    s = section(o, id);
    if (!s || s->dummy) return fail(o, MF_OBJECT);
    if (off > s->length || length > s->length - off) return fail(o, MF_RANGE);
    if (off < s->cursor) return fail(o, MF_UNSUPPORTED);
    if (flush(o) != MF_OK) return o->error;
    s->cursor = off + length;
    s->text_start = s->cursor; s->text_end = s->cursor;
    return MF_OK;
}
static enum mf_status fixup(void *cookie, const struct mf_fixup *f)
{
    struct mf_obj *o = (struct mf_obj *)cookie;
    struct obj_section *s, *target_section;
    struct obj_symbol *target_symbol;
    unsigned target;
    mf_octet p[80];
    if (active(o) != MF_OK) return o->error;
    if (!f) return fail(o, MF_OBJECT);
    if (f->width != 4) return fail(o, MF_UNSUPPORTED);
    if (f->address_kind != MF_ADDRESS_A && f->address_kind != MF_ADDRESS_V)
        return fail(o, MF_UNSUPPORTED);
    s = section(o, f->section);
    if (!s || s->dummy) return fail(o, MF_OBJECT);
    if (f->offset > s->length || 4UL > s->length - f->offset)
        return fail(o, MF_RANGE);
    if (f->offset < s->text_start || f->offset > s->text_end ||
        4UL > s->text_end - f->offset) return fail(o, MF_OBJECT);
    if (f->target_kind == MF_REF_SECTION) {
        target_section = section(o, f->target);
        if (!target_section || target_section->dummy) return fail(o, MF_UNDEFINED);
        target = target_section->esdid;
    } else if (f->target_kind == MF_REF_EXTERNAL) {
        target_symbol = symbol(o, f->target);
        if (!target_symbol || target_symbol->kind != MF_EXTERNAL)
            return fail(o, MF_UNDEFINED);
        target = target_symbol->esdid;
    } else return fail(o, MF_OBJECT);
    if (flush(o) != MF_OK) return o->error;
    card(p, 0xd9, 0xd3, 0xc4); store(p + 10, 8, 2);
    store(p + 16, target, 2); store(p + 18, s->esdid, 2);
    p[20] = (mf_octet)(0x0c | (f->address_kind == MF_ADDRESS_V ? 0x10 : 0) |
        (f->subtract ? 2 : 0)); store(p + 21, f->offset, 3);
    return emit(o, p);
}
static enum mf_status entry(void *cookie, const struct mf_entry *e)
{
    struct mf_obj *o = (struct mf_obj *)cookie;
    struct obj_section *s;
    if (active(o) != MF_OK) return o->error;
    if (!e) return fail(o, MF_OBJECT);
    if (e->present) {
        s = section(o, e->section);
        if (!s || s->dummy || e->offset >= s->length) return fail(o, MF_RANGE);
    }
    o->entry = *e;
    return MF_OK;
}
static enum mf_status finish(void *cookie, int valid)
{
    struct mf_obj *o = (struct mf_obj *)cookie;
    enum mf_status st;
    mf_octet p[80];
    if (o->closed) return o->error == MF_OK ? MF_OBJECT : o->error;
    if (!o->begun && valid) fail(o, MF_OBJECT);
    if (!valid) fail(o, MF_SOURCE);
    if (o->error == MF_OK && flush(o) == MF_OK) {
        card(p, 0xc5, 0xd5, 0xc4);
        if (o->entry.present) {
            store(p + 5, o->entry.offset, 3);
            store(p + 14, section(o, o->entry.section)->esdid, 2);
        }
        emit(o, p);
    }
    o->closed = 1;
    if (o->error != MF_OK) {
        st = o->sink.finish(o->sink.cookie, 0);
        if (st != MF_OK) fail(o, st);
        return o->error;
    }
    st = o->sink.finish(o->sink.cookie, 1);
    if (st != MF_OK) {
        fail(o, st); o->sink.finish(o->sink.cookie, 0);
    }
    return o->error;
}
enum mf_status mf_obj_create(const struct mf_storage *storage,
    const struct mf_sink *sink, size_t max_sections, size_t max_symbols,
    struct mf_obj **result, struct mf_object_writer *writer)
{
    struct mf_obj *o;
    if (!result || !writer) return MF_OBJECT;
    *result = NULL; memset(writer, 0, sizeof(*writer));
    if (!storage || !storage->acquire || !sink || !sink->begin ||
        !sink->write || !sink->finish) return MF_OBJECT;
    if (max_sections > (size_t)-1 / sizeof(struct obj_section) ||
        max_symbols > (size_t)-1 / sizeof(struct obj_symbol)) return MF_LIMIT;
    o = (struct mf_obj *)storage->acquire(storage->cookie, sizeof(*o));
    if (!o) return MF_LIMIT;
    memset(o, 0, sizeof(*o)); o->sink = *sink;
    o->max_sections = max_sections; o->max_symbols = max_symbols;
    if (max_sections) {
        o->sections = (struct obj_section *)storage->acquire(storage->cookie,
            max_sections * sizeof(struct obj_section));
        if (!o->sections) return MF_LIMIT;
    }
    if (max_symbols) {
        o->symbols = (struct obj_symbol *)storage->acquire(storage->cookie,
            max_symbols * sizeof(struct obj_symbol));
        if (!o->symbols) return MF_LIMIT;
    }
    *result = o; writer->cookie = o; writer->begin = begin; writer->text = text;
    writer->gap = gap; writer->fixup = fixup; writer->entry = entry;
    writer->finish = finish; return MF_OK;
}
void mf_obj_destroy(struct mf_obj *o)
{
    if (o && o->begun && !o->closed) finish(o, 0);
}
