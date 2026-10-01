/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Original bounded Gate 1 macro interpreter. No host services or allocations
 * occur while streaming; definitions and per-depth arguments reuse arenas.
 */
#include "macro_internal.h"
#include <string.h>
#define MAX32 0xffffffffUL
#define NONE ((size_t)-1)
struct parameter { struct mf_span name, initial; int keyword; };
struct definition {
    struct mf_span name, label;
    struct mf_origin origin;
    size_t parameters, first, count;
};
struct frame { size_t definition, pc; };
struct member { struct mf_records records; unsigned identity; size_t owner; };
struct fingerprint { mf_u32 a, b; struct mf_u64 records, bytes; };
struct mf_macro {
    struct mf_macro_config config;
    struct mf_records records;
    struct mf_macro_observer observer;
    struct mf_macro_library library;
    struct member *members;
    size_t member_depth;
    int started;
    struct definition *definitions;
    struct parameter *parameters;
    struct mf_statement *models;
    struct frame *frames;
    struct mf_macro_frame_info *trace;
    struct mf_span *values;
    mf_octet *assigned, *arguments, *definition_bytes, *output, *pending;
    mf_octet card[80];
    size_t definition_count, model_count, definition_used, depth;
    unsigned long steps;
    struct fingerprint first, current;
    struct mf_origin last_origin;
    enum mf_status terminal;
    int complete, replaying;
};
/* Zero-terminated numeric ASCII keywords, never native execution text. */
static const mf_octet keywords[][8] = {
    {0x4d,0x41,0x43,0x52,0x4f,0}, {0x4d,0x45,0x4e,0x44,0},
    {0x43,0x4f,0x50,0x59,0}, {0x41,0x49,0x46,0}, {0x41,0x47,0x4f,0},
    {0x41,0x4e,0x4f,0x50,0}, {0x4d,0x45,0x58,0x49,0x54,0},
    {0x4c,0x43,0x4c,0x41,0}, {0x4c,0x43,0x4c,0x42,0}, {0x4c,0x43,0x4c,0x43,0},
    {0x47,0x42,0x4c,0x41,0}, {0x47,0x42,0x4c,0x42,0}, {0x47,0x42,0x4c,0x43,0},
    {0x53,0x45,0x54,0x41,0}, {0x53,0x45,0x54,0x42,0}, {0x53,0x45,0x54,0x43,0},
    {0x4d,0x4e,0x4f,0x54,0x45,0}, {0x4d,0x48,0x45,0x4c,0x50,0},
    {0x41,0x43,0x54,0x52,0}, {0x41,0x52,0x45,0x41,0x44,0},
    {0x53,0x59,0x53,0x4e,0x44,0x58,0}
};
static int word(struct mf_span s, size_t k)
{
    struct mf_span t; size_t n;
    for (n = 0; keywords[k][n]; ++n) { }
    t.data = keywords[k]; t.length = n; return mf_macro_same(s, t);
}
static int blocked(struct mf_span s)
{
    size_t i;
    for (i = 3; i < 20; ++i) if (word(s, i)) return 1;
    return 0;
}
static struct mf_span slice(struct mf_span s, size_t at, size_t n)
{
    struct mf_span r; r.data = s.data ? s.data + at : NULL; r.length = n; return r;
}
static int name_valid(struct mf_span s)
{
    size_t i;
    if (!s.length || !mf_macro_name_first(s.data[0])) return 0;
    for (i = 1; i < s.length; ++i) if (!mf_macro_name_rest(s.data[i])) return 0;
    return 1;
}
static int empty(struct mf_span s)
{ return !s.length || (s.length == 1 && s.data[0] == 0x2c); }
static void hash_init(struct fingerprint *f)
{
    memset(f, 0, sizeof *f); f->a = 2166136261UL; f->b = 0x7f4a7c15UL;
}
static void hash_byte(struct fingerprint *f, mf_octet b)
{
    f->a = ((f->a ^ b) * 16777619UL) & MAX32;
    f->b = (((f->b << 5) & MAX32) | (f->b >> 27));
    f->b = (f->b + (mf_u32)b + 0x9e3779b9UL) & MAX32;
}
static void hash_size(struct fingerprint *f, size_t n)
{
    size_t i;
    for (i = 0; i < sizeof n; ++i) { hash_byte(f, (mf_octet)(n & 255)); n /= 256; }
}
static void hash_ulong(struct fingerprint *f, unsigned long n)
{
    size_t i;
    for (i = 0; i < sizeof n; ++i) { hash_byte(f, (mf_octet)(n & 255)); n /= 256; }
}
static enum mf_status count(struct mf_u64 *v, size_t n)
{
    size_t i;
    for (i = 0; i < n; ++i) {
        if (v->lo == MAX32) {
            if (v->hi == MAX32) return MF_LIMIT;
            v->lo = 0; ++v->hi;
        } else ++v->lo;
    }
    return MF_OK;
}
static enum mf_status hash_record(struct fingerprint *f, const struct mf_record *r)
{
    size_t i; enum mf_status st;
    st = count(&f->records, 1); if (st != MF_OK) return st;
    st = count(&f->bytes, r->bytes.length); if (st != MF_OK) return st;
    hash_size(f, r->bytes.length); hash_ulong(f, r->origin.source);
    hash_ulong(f, r->origin.line); hash_ulong(f, r->origin.column);
    for (i = 0; i < r->bytes.length; ++i) hash_byte(f, r->bytes.data[i]);
    return MF_OK;
}
static int hashes_equal(const struct fingerprint *a, const struct fingerprint *b)
{
    return a->a == b->a && a->b == b->b &&
        a->records.hi == b->records.hi && a->records.lo == b->records.lo &&
        a->bytes.hi == b->bytes.hi && a->bytes.lo == b->bytes.lo;
}
static enum mf_status step(struct mf_macro *m)
{
    if (m->steps == m->config.max_steps) return MF_LIMIT;
    ++m->steps; return MF_OK;
}
static enum mf_status raw(struct mf_macro *m, struct mf_statement *s)
{
    struct mf_record r; struct mf_records *records; enum mf_status st; int skip;
    for (;;) {
        memset(&r, 0, sizeof r);
        records = m->member_depth ? &m->members[m->member_depth - 1].records : &m->records;
        st = records->next(records->cookie, &r);
        m->last_origin = r.origin;
        if (st != MF_OK) return st;
        if (r.bytes.length > 80 || (r.bytes.length && !r.bytes.data))
            return r.bytes.length > 80 ? MF_LIMIT : MF_SOURCE;
        st = step(m); if (st != MF_OK) return st;
        st = hash_record(&m->current, &r); if (st != MF_OK) return st;
        st = mf_macro_card(&r, records->encoding, m->card, sizeof m->card, s, &skip);
        m->last_origin = s->origin;
        if (st != MF_OK) return st;
        if (!skip) return MF_OK;
    }
}
static enum mf_status copy(struct mf_span s, mf_octet *bytes, size_t cap,
    size_t *used, struct mf_span *out)
{
    if (*used > cap || s.length > cap - *used) return MF_LIMIT;
    out->data = bytes + *used; out->length = s.length;
    if (s.length) memcpy(bytes + *used, s.data, s.length);
    *used += s.length; return MF_OK;
}
static enum mf_status retain(struct mf_macro *m, const struct mf_statement *s,
    struct mf_statement *out)
{
    enum mf_status st;
    out->origin = s->origin;
    st = copy(s->label, m->definition_bytes, m->config.max_definition_bytes,
        &m->definition_used, &out->label); if (st != MF_OK) return st;
    st = copy(s->operation, m->definition_bytes, m->config.max_definition_bytes,
        &m->definition_used, &out->operation); if (st != MF_OK) return st;
    return copy(s->operand, m->definition_bytes, m->config.max_definition_bytes,
        &m->definition_used, &out->operand);
}
static size_t find(struct mf_macro *m, struct mf_span name)
{
    size_t i;
    for (i = 0; i < m->definition_count; ++i)
        if (mf_macro_same(name, m->definitions[i].name)) return i;
    return NONE;
}
static size_t equals(struct mf_span s)
{
    size_t i, level; int quoted;
    level = 0; quoted = 0;
    for (i = 0; i < s.length; ++i) {
        if (s.data[i] == 0x27) quoted = !quoted;
        else if (!quoted && s.data[i] == 0x28) ++level;
        else if (!quoted && s.data[i] == 0x29) --level;
        else if (!quoted && !level && s.data[i] == 0x3d) return i;
    }
    return NONE;
}
static enum mf_status capture(struct mf_macro *m, const struct mf_statement *marker)
{
    struct mf_statement s, p; struct definition *d;
    struct parameter *a; struct mf_span arg, key; size_t cur, eq, i;
    enum mf_status st;
    if (marker->label.length || !empty(marker->operand)) return MF_SOURCE;
    if (m->definition_count == m->config.max_macros) return MF_LIMIT;
    st = raw(m, &s); if (st == MF_EOF) return MF_SOURCE; if (st != MF_OK) return st;
    if (!name_valid(s.operation) || blocked(s.operation) || word(s.operation, 0) || word(s.operation, 1))
        return MF_SOURCE;
    if (find(m, s.operation) != NONE) return MF_DUPLICATE;
    st = retain(m, &s, &p); if (st != MF_OK) return st;
    d = m->definitions + m->definition_count; memset(d, 0, sizeof *d);
    d->name = p.operation; d->origin = p.origin; d->first = m->model_count;
    if (p.label.length) {
        if (p.label.data[0] != 0x26) return MF_SOURCE;
        d->label = slice(p.label, 1, p.label.length - 1);
        if (!name_valid(d->label) || word(d->label, 20)) return MF_UNSUPPORTED;
    }
    a = m->parameters ? m->parameters + m->definition_count * m->config.max_parameters : NULL;
    if (p.operand.length) {
        cur = 0;
        while ((st = mf_macro_arguments(p.operand, &cur, &arg)) == MF_OK) {
            if (d->parameters == m->config.max_parameters) return MF_LIMIT;
            eq = equals(arg); key = slice(arg, 0, eq == NONE ? arg.length : eq);
            if (!key.length || key.data[0] != 0x26) return MF_SOURCE;
            key = slice(key, 1, key.length - 1);
            if (!name_valid(key) || word(key, 20)) return MF_UNSUPPORTED;
            if (d->label.length && mf_macro_same(key, d->label)) return MF_DUPLICATE;
            for (i = 0; i < d->parameters; ++i) if (mf_macro_same(key, a[i].name)) return MF_DUPLICATE;
            if (eq == NONE && d->parameters && a[d->parameters - 1].keyword) return MF_SOURCE;
            a[d->parameters].name = key; a[d->parameters].keyword = eq != NONE;
            a[d->parameters].initial = slice(arg, eq == NONE ? arg.length : eq + 1,
                eq == NONE ? 0 : arg.length - eq - 1);
            for (i = 0; i < a[d->parameters].initial.length; ++i)
                if (a[d->parameters].initial.data[i] == 0x26) return MF_UNSUPPORTED;
            ++d->parameters;
        }
        if (st != MF_EOF) return st;
    }
    for (;;) {
        st = raw(m, &s); if (st == MF_EOF) return MF_SOURCE; if (st != MF_OK) return st;
        if (word(s.operation, 1)) {
            if (!empty(s.operand)) return MF_SOURCE;
            ++m->definition_count; return MF_OK;
        }
        if (word(s.operation, 0) || word(s.operation, 2) || blocked(s.operation) ||
            (s.label.length && s.label.data[0] == 0x2e)) return MF_UNSUPPORTED;
        if (m->model_count == m->config.max_model_statements) return MF_LIMIT;
        st = retain(m, &s, m->models + m->model_count); if (st != MF_OK) return st;
        ++m->model_count; ++d->count;
    }
}
static enum mf_status push(struct mf_macro *m, size_t definition, const struct mf_statement *call)
{
    struct definition *d; struct parameter *p; struct mf_span *v, arg, key, value;
    size_t used, cur, eq, i, index, positional; int keywords_seen;
    mf_octet *b, *seen; enum mf_status st;
    if (m->depth == m->config.max_depth) return MF_LIMIT;
    d = m->definitions + definition;
    p = m->parameters ? m->parameters + definition * m->config.max_parameters : NULL;
    v = m->values + m->depth * (m->config.max_parameters + 1);
    seen = m->assigned + m->depth * (m->config.max_parameters + 1);
    b = m->arguments + m->depth * m->config.max_argument_bytes;
    memset(seen, 0, m->config.max_parameters + 1); used = 0;
    for (i = 0; i < d->parameters; ++i) v[i] = p[i].initial;
    if (call->label.length && !d->label.length) return MF_UNSUPPORTED;
    st = copy(call->label, b, m->config.max_argument_bytes, &used, v + m->config.max_parameters);
    if (st != MF_OK) return st;
    if (call->operand.length) {
        cur = positional = 0; keywords_seen = 0;
        while ((st = mf_macro_arguments(call->operand, &cur, &arg)) == MF_OK) {
            eq = equals(arg); value = arg;
            if (eq != NONE) {
                keywords_seen = 1; key = slice(arg, 0, eq);
                for (index = 0; index < d->parameters; ++index)
                    if (p[index].keyword && mf_macro_same(key, p[index].name)) break;
                if (index == d->parameters) return MF_SOURCE;
                value = slice(arg, eq + 1, arg.length - eq - 1);
            } else {
                if (keywords_seen) return MF_SOURCE;
                index = positional++;
                if (index == d->parameters || p[index].keyword) return MF_UNSUPPORTED;
            }
            if (seen[index]) return MF_DUPLICATE;
            seen[index] = 1;
            st = copy(value, b, m->config.max_argument_bytes, &used, v + index);
            if (st != MF_OK) return st;
        }
        if (st != MF_EOF) return st;
    }
    m->frames[m->depth].definition = definition; m->frames[m->depth].pc = 0;
    m->trace[m->depth].name = d->name; m->trace[m->depth].invocation = call->origin;
    m->trace[m->depth].definition = d->origin; m->trace[m->depth].model = d->origin;
    ++m->depth; return MF_OK;
}
static enum mf_status substitute(struct mf_macro *m, struct mf_span s,
    size_t *used, struct mf_span *out)
{
    struct definition *d; struct parameter *p; struct mf_span name, value;
    size_t i, start, j; int quoted; enum mf_status st;
    start = *used; i = 0; quoted = 0;
    while (i < s.length) {
        if (s.data[i] == 0x26) {
            if (!m->depth || i + 1 == s.length || s.data[i + 1] == 0x26) return MF_UNSUPPORTED;
            j = ++i;
            while (i < s.length && mf_macro_name_rest(s.data[i])) ++i;
            name = slice(s, j, i - j);
            if (!name_valid(name) || word(name, 20)) return MF_UNSUPPORTED;
            if (i < s.length && s.data[i] == 0x28) return MF_UNSUPPORTED;
            d = m->definitions + m->frames[m->depth - 1].definition;
            p = m->parameters ? m->parameters + m->frames[m->depth - 1].definition * m->config.max_parameters : NULL;
            for (j = 0; j < d->parameters; ++j) if (mf_macro_same(name, p[j].name)) break;
            if (j == d->parameters) {
                if (!d->label.length || !mf_macro_same(name, d->label)) return MF_UNDEFINED;
                j = m->config.max_parameters;
            }
            value = m->values[(m->depth - 1) * (m->config.max_parameters + 1) + j];
            if (value.length > m->config.max_statement_bytes - *used) return MF_LIMIT;
            if (value.length) memcpy(m->output + *used, value.data, value.length);
            *used += value.length;
            if (i < s.length && s.data[i] == 0x2e) ++i;
        } else {
            mf_octet c; c = mf_macro_upper(s.data[i]);
            if (!quoted && i + 1 < s.length && s.data[i + 1] == 0x27 &&
                (c == 0x4c || c == 0x4b || c == 0x4e || c == 0x54)) return MF_UNSUPPORTED;
            if (s.data[i] == 0x27) quoted = !quoted;
            if (*used == m->config.max_statement_bytes) return MF_LIMIT;
            m->output[(*used)++] = s.data[i++];
        }
    }
    out->data = m->output + start; out->length = *used - start;
    st = MF_OK; return st;
}
static enum mf_status expand(struct mf_macro *m, const struct mf_statement *s,
    struct mf_statement *out)
{
    size_t used; enum mf_status st;
    used = 0; out->origin = s->origin;
    st = substitute(m, s->label, &used, &out->label); if (st != MF_OK) return st;
    st = substitute(m, s->operation, &used, &out->operation); if (st != MF_OK) return st;
    st = substitute(m, s->operand, &used, &out->operand); if (st != MF_OK) return st;
    if ((out->label.length && !name_valid(out->label)) || !name_valid(out->operation)) return MF_SOURCE;
    return MF_OK;
}
static void observe(struct mf_macro *m, enum mf_status status, struct mf_span detail)
{
    struct mf_macro_event e;
    if (!m->observer.notify) return;
    e.status = status; e.origin = m->last_origin; e.detail = detail;
    e.frames = m->depth ? m->trace : NULL; e.depth = m->depth;
    m->observer.notify(m->observer.cookie, &e);
}
static enum mf_status member_pop(struct mf_macro *m)
{
    if (!m->member_depth) return MF_SOURCE;
    --m->member_depth;
    return m->library.close(m->library.cookie, &m->members[m->member_depth].records);
}
static enum mf_status member_push(struct mf_macro *m, struct mf_span name)
{
    struct member *v; size_t i; enum mf_status st;
    if (!m->library.open) return MF_UNSUPPORTED;
    if (!name_valid(name)) return MF_SOURCE;
    if (m->member_depth == m->config.max_depth) return MF_LIMIT;
    v = m->members + m->member_depth; memset(v, 0, sizeof *v);
    st = m->library.open(m->library.cookie, name, &v->records, &v->identity);
    if (st != MF_OK) return st;
    if (!v->identity || !v->records.next || !v->records.replay ||
        (v->records.encoding != MF_ASCII && v->records.encoding != MF_CP037)) st = MF_SOURCE;
    for (i = 0; st == MF_OK && i < m->member_depth; ++i)
        if (m->members[i].identity == v->identity) st = MF_SOURCE;
    if (st != MF_OK) { m->library.close(m->library.cookie, &v->records); return st; }
    v->owner = m->depth; ++m->member_depth; return MF_OK;
}
static enum mf_status library_macro(struct mf_macro *m, struct mf_span name, size_t *index)
{
    struct mf_statement s; enum mf_status st, closed;
    st = member_push(m, name); if (st != MF_OK) return st;
    st = raw(m, &s);
    if (st == MF_OK && !word(s.operation, 0)) st = MF_SOURCE;
    if (st == MF_OK) st = capture(m, &s);
    if (st == MF_OK) {
        *index = find(m, name);
        if (*index == NONE) st = MF_SOURCE;
    }
    if (st == MF_OK) {
        st = raw(m, &s);
        st = st == MF_EOF ? MF_OK : (st == MF_OK ? MF_SOURCE : st);
    }
    closed = member_pop(m); return st == MF_OK ? closed : st;
}
static enum mf_status provider_next(void *cookie, struct mf_statement *out)
{
    struct mf_macro *m; struct mf_statement s, expanded;
    enum mf_status st; size_t k; struct definition *d;
    struct mf_span detail;
    m = (struct mf_macro *)cookie; m->started = 1;
    if (!out) return MF_SOURCE;
    memset(out, 0, sizeof *out); memset(&s, 0, sizeof s);
    if (m->terminal != MF_OK) { out->origin = m->last_origin; return m->terminal; }
    for (;;) {
        while (m->depth && m->frames[m->depth - 1].pc ==
            m->definitions[m->frames[m->depth - 1].definition].count) --m->depth;
        if (m->depth && (!m->member_depth ||
            m->members[m->member_depth - 1].owner != m->depth)) {
            d = m->definitions + m->frames[m->depth - 1].definition;
            s = m->models[d->first + m->frames[m->depth - 1].pc++];
            m->trace[m->depth - 1].model = s.origin;
            m->last_origin = m->trace[0].invocation;
            st = step(m); if (st != MF_OK) break;
        } else {
            st = raw(m, &s);
            if (st == MF_EOF) {
                if (m->member_depth) { st = member_pop(m); if (st != MF_OK) break; continue; }
                if (m->replaying && !hashes_equal(&m->first, &m->current)) st = MF_REPLAY;
                else {
                    if (!m->replaying) m->first = m->current;
                    m->complete = 1; m->terminal = MF_EOF; return MF_EOF;
                }
            }
            if (st != MF_OK) break;
            if (word(s.operation, 0)) { st = capture(m, &s); if (st != MF_OK) break; continue; }
        }
        if (word(s.operation, 0) || word(s.operation, 1) || blocked(s.operation) ||
            (s.label.length && s.label.data[0] == 0x2e)) {
            st = MF_UNSUPPORTED; break;
        }
        st = expand(m, &s, &expanded); if (st != MF_OK) break;
        if (blocked(expanded.operation) || word(expanded.operation, 0) || word(expanded.operation, 1)) {
            st = MF_UNSUPPORTED; break;
        }
        if (word(expanded.operation, 2)) {
            if (expanded.label.length) { st = MF_UNSUPPORTED; break; }
            st = member_push(m, expanded.operand); if (st != MF_OK) break; continue;
        }
        k = find(m, expanded.operation);
        if (k == NONE && m->library.open) {
            /* The temporary output spans are overwritten by raw/capture.
             * Retain this one pending call in bounded workspace. */
            struct mf_statement pending; size_t used;
            used = 0;
            pending.origin = expanded.origin;
            st = copy(expanded.label, m->pending, m->config.max_statement_bytes, &used, &pending.label);
            if (st == MF_OK) st = copy(expanded.operation, m->pending, m->config.max_statement_bytes, &used, &pending.operation);
            if (st == MF_OK) st = copy(expanded.operand, m->pending, m->config.max_statement_bytes, &used, &pending.operand);
            if (st != MF_OK) break;
            st = library_macro(m, pending.operation, &k);
            expanded = pending;
            if (st != MF_OK && st != MF_UNDEFINED) break;
        }
        if (k != NONE) { st = push(m, k, &expanded); if (st != MF_OK) break; continue; }
        if (m->depth) expanded.origin = m->trace[0].invocation;
        m->last_origin = expanded.origin; *out = expanded;
        observe(m, MF_OK, expanded.operation); return MF_OK;
    }
    if (m->replaying && (st == MF_SOURCE || st == MF_UNDEFINED ||
        st == MF_DUPLICATE || st == MF_UNSUPPORTED)) st = MF_REPLAY;
    m->terminal = st; out->origin = m->last_origin;
    detail = s.operation;
    if (st == MF_IO || st == MF_REPLAY) { detail.data = NULL; detail.length = 0; }
    observe(m, st, detail); return st;
}
static enum mf_status provider_replay(void *cookie)
{
    struct mf_macro *m; enum mf_status st;
    m = (struct mf_macro *)cookie;
    if (!m->complete || m->terminal != MF_EOF) return MF_SOURCE;
    st = m->records.replay(m->records.cookie); if (st != MF_OK) return st;
    m->definition_count = m->model_count = m->definition_used = m->depth = 0;
    m->steps = 0; m->terminal = MF_OK; m->complete = 0; m->replaying = 1;
    memset(&m->last_origin, 0, sizeof m->last_origin); hash_init(&m->current);
    return MF_OK;
}
static enum mf_status allocate(const struct mf_storage *s, size_t n, size_t width, void **out)
{
    if (n && width > (size_t)-1 / n) return MF_LIMIT;
    *out = n ? s->acquire(s->cookie, n * width) : NULL;
    return n && !*out ? MF_LIMIT : MF_OK;
}
enum mf_status mf_macro_create(const struct mf_macro_config *c,
    const struct mf_storage *storage, const struct mf_records *records,
    const struct mf_macro_observer *observer, struct mf_macro **out,
    struct mf_statements *statements)
{
    struct mf_macro *m; size_t slots, params; enum mf_status st; void *p;
    if (out) *out = NULL;
    if (statements) memset(statements, 0, sizeof *statements);
    if (!c || !storage || !storage->acquire || !records || !records->next ||
        !records->replay || !out || !statements) return MF_SOURCE;
    if (records->encoding != MF_ASCII && records->encoding != MF_CP037) return MF_UNSUPPORTED;
    if (!c->max_macros || !c->max_model_statements || !c->max_definition_bytes ||
        !c->max_depth || !c->max_argument_bytes || !c->max_statement_bytes ||
        !c->max_steps || c->max_parameters == (size_t)-1) return MF_SOURCE;
    if (c->max_parameters && c->max_macros > (size_t)-1 / c->max_parameters) return MF_LIMIT;
    if (c->max_depth > (size_t)-1 / (c->max_parameters + 1) ||
        c->max_depth > (size_t)-1 / c->max_argument_bytes) return MF_LIMIT;
    slots = c->max_depth * (c->max_parameters + 1);
    params = c->max_macros * c->max_parameters;
    st = allocate(storage, 1, sizeof *m, &p); if (st != MF_OK) return st;
    m = (struct mf_macro *)p; memset(m, 0, sizeof *m); m->config = *c; m->records = *records;
    if (observer) m->observer = *observer;
#define ALLOC(field, n, type) do { st = allocate(storage, n, sizeof(type), &p); \
    if (st != MF_OK) return st; m->field = (type *)p; } while (0)
    ALLOC(definitions, c->max_macros, struct definition);
    ALLOC(parameters, params, struct parameter);
    ALLOC(models, c->max_model_statements, struct mf_statement);
    ALLOC(frames, c->max_depth, struct frame);
    ALLOC(trace, c->max_depth, struct mf_macro_frame_info);
    ALLOC(values, slots, struct mf_span);
    ALLOC(assigned, slots, mf_octet);
    ALLOC(arguments, c->max_depth * c->max_argument_bytes, mf_octet);
    ALLOC(definition_bytes, c->max_definition_bytes, mf_octet);
    ALLOC(output, c->max_statement_bytes, mf_octet);
    ALLOC(pending, c->max_statement_bytes, mf_octet);
    ALLOC(members, c->max_depth, struct member);
#undef ALLOC
    hash_init(&m->current);
    *out = m; statements->cookie = m; statements->next = provider_next;
    statements->replay = provider_replay; return MF_OK;
}
enum mf_status mf_macro_set_library(struct mf_macro *m, const struct mf_macro_library *library)
{
    if (!m || m->started || !library || !library->open || !library->close) return MF_SOURCE;
    m->library = *library; return MF_OK;
}
void mf_macro_destroy(struct mf_macro *m)
{
    if (m) while (m->member_depth) { (void)member_pop(m); }
}
