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
struct frame { size_t definition, pc; unsigned long index; };
struct variable { mf_octet name[64]; size_t length, used, global; int type; long number; };
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
    struct variable *variables; mf_octet *variable_text;
    size_t *variable_counts; size_t variable_slots;
    unsigned long invocation_index;
    mf_octet skip_name[64]; size_t skip_length, skip_nesting;
    struct definition *definitions;
    struct parameter *parameters;
    struct mf_statement *models;
    struct frame *frames;
    struct mf_macro_frame_info *trace;
    struct mf_span *values;
    mf_octet *assigned, *arguments, *definition_bytes, *output, *pending, *logical;
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
    for (i = 17; i < 20; ++i) if (word(s, i)) return 1;
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
/* Every physical card remains a separately bounded, fingerprinted input. */
static enum mf_status physical(struct mf_macro *m, struct mf_records *records,
    struct mf_record *r)
{
    enum mf_status st;
    memset(r,0,sizeof *r); st = records->next(records->cookie,r);
    m->last_origin = r->origin;
    if (st != MF_OK) return st;
    if (r->bytes.length > 80 || (r->bytes.length && !r->bytes.data))
        return r->bytes.length > 80 ? MF_LIMIT : MF_SOURCE;
    st = step(m); if (st != MF_OK) return st;
    return hash_record(&m->current,r);
}
static enum mf_status card_byte(const struct mf_record *r, enum mf_encoding encoding,
    size_t at, mf_octet *c)
{
    enum mf_status st;
    *c = at < r->bytes.length ? r->bytes.data[at] : 0x20;
    if (at < r->bytes.length && encoding == MF_CP037) {
        st = mf_ebcdic_to_ascii(*c,c); if (st != MF_OK) return st;
    }
    return *c >= 0x20 && *c <= 0x7e ? MF_OK : MF_SOURCE;
}
static enum mf_status joined(struct mf_macro *m, struct mf_records *records,
    struct mf_record *r, struct mf_statement *s, int *skip)
{
    struct mf_record text; struct mf_origin origin; enum mf_status st;
    size_t i, begin, used, operand, level; int quoted, continued, first;
    mf_octet c, previous;
    origin = r->origin; used = level = 0; quoted = 0; first = 1; previous = 0;
    operand = (size_t)(s->operation.data - m->card) + s->operation.length;
    for (;;) {
        st = card_byte(r,records->encoding,71,&c); if (st != MF_OK) return st;
        continued = c != 0x20; begin = first ? 0 : 15;
        if (!first) for (i = 0; i < 15; ++i) {
            st = card_byte(r,records->encoding,i,&c); if (st != MF_OK) return st;
            if (c != 0x20) { m->last_origin.column = (unsigned)i+1; return MF_SOURCE; }
        }
        for (i = begin; i < 71 && i < r->bytes.length; ++i) {
            st = card_byte(r,records->encoding,i,&c); if (st != MF_OK) return st;
            if (first && i >= operand && !quoted && !level && c == 0x20) {
                /* Skip the separating spaces before the first operand. */
                if (!previous) { if (used == m->config.max_statement_bytes) return MF_LIMIT;
                    m->logical[used++] = c; continue; }
            }
            if ((!first || i >= operand) && !quoted && !level && c == 0x20 && previous) {
                if (continued && previous != 0x2c) return MF_UNSUPPORTED;
                break; /* comma-terminated card padding and remarks */
            }
            if (used == m->config.max_statement_bytes) return MF_LIMIT;
            m->logical[used++] = c;
            if (!first || i >= operand) {
                if (c == 0x27) {
                    int attribute; mf_octet before, after;
                    before = used >= 2 ? mf_macro_upper(m->logical[used-2]) : 0;
                    st = card_byte(r,records->encoding,i+1,&after); if (st != MF_OK) return st;
                    attribute = before == 0x4c || ((before == 0x54 || before == 0x4b || before == 0x4e) && after == 0x26);
                    if (quoted || !attribute) quoted = !quoted;
                }
                else if (!quoted && c == 0x28) ++level;
                else if (!quoted && c == 0x29) { if (!level) return MF_SOURCE; --level; }
                if (c != 0x20) previous = c;
            }
        }
        if (!continued) break;
        st = physical(m,records,r); if (st == MF_EOF) return MF_SOURCE;
        if (st != MF_OK) return st;
        first = 0;
    }
    text.bytes.data = m->logical; text.bytes.length = used; text.origin = origin;
    return mf_macro_card(&text,MF_ASCII,m->logical,m->config.max_statement_bytes,s,skip,0,1);
}
static enum mf_status raw(struct mf_macro *m, struct mf_statement *s)
{
    struct mf_record r; struct mf_records *records; enum mf_status st; int skip, active;
    for (;;) {
        records = m->member_depth ? &m->members[m->member_depth - 1].records : &m->records;
        st = physical(m,records,&r); if (st != MF_OK) return st;
        st = mf_macro_card(&r,records->encoding,m->card,sizeof m->card,s,&skip,1,0);
        active = !m->skip_length;
        if (st == MF_OK && !skip && m->skip_length && !m->skip_nesting) {
            struct mf_span target; target.data = m->skip_name; target.length = m->skip_length;
            if (mf_macro_same(target,s->label)) active = 1;
        }
        if (st == MF_OK && !skip && active) st = joined(m,records,&r,s,&skip);
        if (st != MF_OK) return st;
        m->last_origin = s->origin;
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
        if (s.data[i] == 0x27 && (quoted || !i || mf_macro_upper(s.data[i-1]) != 0x4c)) quoted = !quoted;
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
    if (!empty(p.operand)) {
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
        if (word(s.operation, 0) || word(s.operation, 2) || blocked(s.operation)) return MF_UNSUPPORTED;
        if (s.label.length && s.label.data[0] == 0x2e) {
            if (!name_valid(slice(s.label, 1, s.label.length - 1))) return MF_SOURCE;
            for (i = 0; i < d->count; ++i)
                if (mf_macro_same(s.label, m->models[d->first + i].label)) return MF_DUPLICATE;
        }
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
    if (call->operand.length && !(d->parameters == 0 && empty(call->operand))) {
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
    if (m->invocation_index == MAX32) return MF_LIMIT;
    m->frames[m->depth].index = ++m->invocation_index;
    m->variable_counts[m->depth + 2] = 0;
    m->frames[m->depth].definition = definition; m->frames[m->depth].pc = 0;
    m->trace[m->depth].name = d->name; m->trace[m->depth].invocation = call->origin;
    m->trace[m->depth].definition = d->origin; m->trace[m->depth].model = d->origin;
    ++m->depth; return MF_OK;
}
static size_t variable_find(struct mf_macro *m, size_t scope, struct mf_span name)
{
    size_t i, first; struct mf_span n;
    first = scope * m->config.max_model_statements;
    for (i = 0; i < m->variable_counts[scope]; ++i) {
        n.data = m->variables[first + i].name; n.length = m->variables[first + i].length;
        if (mf_macro_same(n, name)) return first + i;
    }
    return NONE;
}
static struct mf_span decimal(long number, mf_octet *buffer)
{
    struct mf_span v; size_t i, j; unsigned long n; mf_octet c;
    i = 0; n = number < 0 ? (unsigned long)(-(number + 1)) + 1UL : (unsigned long)number;
    do { buffer[i++] = (mf_octet)(0x30 + n % 10UL); n /= 10UL; } while (n);
    if (number < 0) buffer[i++] = 0x2d;
    for (j = 0; j < i / 2; ++j) { c = buffer[j]; buffer[j] = buffer[i - j - 1]; buffer[i - j - 1] = c; }
    v.data = buffer; v.length = i; return v;
}
static enum mf_status resolve(struct mf_macro *m, struct mf_span name,
    mf_octet *buffer, struct mf_span *value)
{
    struct definition *d; struct parameter *p; struct variable *v;
    size_t j, at;
    if (word(name, 20)) {
        unsigned long n;
        if (!m->depth) return MF_UNSUPPORTED;
        n = m->frames[m->depth - 1].index;
        if (n > 2147483647UL) return MF_RANGE;
        *value = decimal((long)n, buffer);
        if (value->length < 4) {
            size_t i, pad; pad = 4 - value->length;
            for (i = value->length; i; --i) buffer[i - 1 + pad] = buffer[i - 1];
            for (i = 0; i < pad; ++i) buffer[i] = 0x30;
            value->length = 4;
        }
        return MF_OK;
    }
    if (m->depth) {
        d = m->definitions + m->frames[m->depth - 1].definition;
        p = m->parameters ? m->parameters + m->frames[m->depth - 1].definition * m->config.max_parameters : NULL;
        for (j = 0; j < d->parameters; ++j) if (mf_macro_same(name, p[j].name)) break;
        if (j < d->parameters || (d->label.length && mf_macro_same(name, d->label))) {
            if (j == d->parameters) j = m->config.max_parameters;
            *value = m->values[(m->depth - 1) * (m->config.max_parameters + 1) + j]; return MF_OK;
        }
    }
    at = variable_find(m, m->depth + 1, name);
    if (at == NONE) return MF_UNDEFINED;
    if (m->variables[at].global) at = m->variables[at].global - 1;
    v = m->variables + at;
    if (v->type == 2) { value->data = m->variable_text + at * m->config.max_statement_bytes; value->length = v->used; }
    else *value = decimal(v->number, buffer);
    return MF_OK;
}
static enum mf_status variable_add(struct mf_macro *m, size_t scope, struct mf_span name, int type, size_t *at)
{
    struct variable *v;
    if (m->variable_counts[scope] == m->config.max_model_statements) return MF_LIMIT;
    *at = scope * m->config.max_model_statements + m->variable_counts[scope]++;
    v = m->variables + *at; memset(v, 0, sizeof *v);
    memcpy(v->name, name.data, name.length); v->length = name.length; v->type = type;
    return MF_OK;
}
static enum mf_status declare(struct mf_macro *m, const struct mf_statement *s, size_t operation)
{
    size_t cur, at, scope, global, i; int type;
    struct mf_span arg, name; enum mf_status st;
    if (s->label.length && s->label.data[0] != 0x2e) return MF_SOURCE;
    scope = m->depth + 1; type = (int)((operation - 7) % 3); cur = 0;
    while ((st = mf_macro_arguments(s->operand, &cur, &arg)) == MF_OK) {
        name = arg.length && arg.data[0] == 0x26 ? slice(arg, 1, arg.length - 1) : arg;
        if (!name_valid(name)) return MF_UNSUPPORTED;
        if (name.length > 64) return MF_LIMIT;
        if (name.length >= 3 && mf_macro_upper(name.data[0]) == 0x53 &&
            mf_macro_upper(name.data[1]) == 0x59 && mf_macro_upper(name.data[2]) == 0x53) return MF_UNSUPPORTED;
        if (m->depth) {
            struct definition *d; struct parameter *p;
            d = m->definitions + m->frames[m->depth - 1].definition;
            p = m->parameters ? m->parameters + m->frames[m->depth - 1].definition * m->config.max_parameters : NULL;
            if (d->label.length && mf_macro_same(name, d->label)) return MF_DUPLICATE;
            for (i = 0; i < d->parameters; ++i) if (mf_macro_same(name, p[i].name)) return MF_DUPLICATE;
        }
        at = variable_find(m, scope, name);
        if (operation < 10) {
            if (at != NONE) return MF_DUPLICATE;
            st = variable_add(m, scope, name, type, &at); if (st != MF_OK) return st;
        } else {
            global = variable_find(m, 0, name);
            if (global == NONE) { st = variable_add(m, 0, name, type, &global); if (st != MF_OK) return st; }
            else if (m->variables[global].type != type) return MF_SOURCE;
            if (at == NONE) { st = variable_add(m, scope, name, type, &at); if (st != MF_OK) return st; }
            else if (!m->variables[at].global) return MF_DUPLICATE;
            m->variables[at].global = global + 1;
        }
    }
    return st == MF_EOF ? MF_OK : st;
}
static int formal_parameter(struct mf_macro *m, struct mf_span name)
{
    struct definition *d; struct parameter *p; size_t j;
    if (!m->depth) return 0;
    d = m->definitions + m->frames[m->depth - 1].definition;
    if (d->label.length && mf_macro_same(name, d->label)) return 1;
    p = m->parameters ? m->parameters + m->frames[m->depth - 1].definition * m->config.max_parameters : NULL;
    for (j = 0; j < d->parameters; ++j) if (mf_macro_same(name, p[j].name)) return 1;
    return 0;
}
static enum mf_status substitute(struct mf_macro *m, struct mf_span s,
    size_t *used, struct mf_span *out)
{
    struct mf_span name, value; mf_octet buffer[32], attribute;
    struct mf_u64 magnitude; int negative;
    size_t i, start, j; int quoted; enum mf_status st;
    start = *used; i = 0; quoted = 0;
    while (i < s.length) {
        attribute = 0;
        if (!quoted && i + 2 < s.length && (mf_macro_upper(s.data[i]) == 0x54 ||
            mf_macro_upper(s.data[i]) == 0x4b || mf_macro_upper(s.data[i]) == 0x4e) &&
            s.data[i + 1] == 0x27 && s.data[i + 2] == 0x26) { attribute = mf_macro_upper(s.data[i]); i += 2; }
        if (s.data[i] == 0x26) {
            if (i + 1 == s.length || s.data[i + 1] == 0x26) return MF_UNSUPPORTED;
            j = ++i;
            while (i < s.length && mf_macro_name_rest(s.data[i])) ++i;
            name = slice(s, j, i - j);
            if (!name_valid(name)) return MF_UNSUPPORTED;
            st = resolve(m, name, buffer, &value); if (st != MF_OK) return st;
            if (i < s.length && s.data[i] == 0x28) {
                size_t wanted, cursor, index; struct mf_span item; int sublist;
                if (!formal_parameter(m, name)) return MF_UNSUPPORTED;
                wanted = 0; ++i;
                if (i == s.length || s.data[i] < 0x30 || s.data[i] > 0x39) return MF_UNSUPPORTED;
                while (i < s.length && s.data[i] >= 0x30 && s.data[i] <= 0x39) {
                    if (wanted > ((size_t)-1 - (size_t)(s.data[i] - 0x30)) / 10) return MF_RANGE;
                    wanted = wanted * 10 + (size_t)(s.data[i++] - 0x30);
                }
                if (!wanted) return MF_RANGE;
                if (i == s.length || s.data[i++] != 0x29) return MF_UNSUPPORTED;
                sublist = value.length >= 2 && value.data[0] == 0x28 && value.data[value.length - 1] == 0x29;
                if (sublist) value = slice(value, 1, value.length - 2);
                if (!sublist && wanted != 1) value.length = 0;
                else if (sublist) {
                    cursor = 0; index = 0; item = value;
                    while ((st = mf_macro_arguments(value, &cursor, &item)) == MF_OK) if (++index == wanted) break;
                    if (st != MF_OK && st != MF_EOF) return st;
                    value = item; if (st == MF_EOF) value.length = 0;
                }
            }
            if (attribute) {
                /* This selected attribute describes immediate argument text.
                 * Ordinary-symbol attributes still need an engine query. */
                if (!formal_parameter(m, name)) return MF_UNSUPPORTED;
                if (attribute == 0x54) {
                    st = mf_u64_parse(value, &magnitude, &negative);
                    buffer[0] = buffer[2] = 0x27;
                    buffer[1] = !value.length ? 0x4f : st == MF_OK ? 0x4e : 0x55;
                    value.data = buffer; value.length = 3;
                } else {
                    size_t number, cursor; struct mf_span item;
                    number = value.length;
                    if (attribute == 0x4e) {
                        number = value.length ? 1 : 0;
                        if (value.length >= 2 && value.data[0] == 0x28 && value.data[value.length - 1] == 0x29) {
                            value = slice(value, 1, value.length - 2); cursor = number = 0;
                            while ((st = mf_macro_arguments(value, &cursor, &item)) == MF_OK) ++number;
                            if (st != MF_EOF) return st;
                        }
                    }
                    if (number > 2147483647UL) return MF_RANGE;
                    value = decimal((long)number, buffer);
                }
            }
            if (value.length > m->config.max_statement_bytes - *used) return MF_LIMIT;
            if (value.length) memcpy(m->output + *used, value.data, value.length);
            *used += value.length;
            if (i < s.length && s.data[i] == 0x2e) ++i;
        } else {
            mf_octet c; c = mf_macro_upper(s.data[i]);
            if (!quoted && i + 1 < s.length && s.data[i + 1] == 0x27 &&
                (c == 0x4b || c == 0x4e || c == 0x54)) return MF_UNSUPPORTED;
            if (s.data[i] == 0x27 && (quoted || !i || mf_macro_upper(s.data[i-1]) != 0x4c)) quoted = !quoted;
            if (*used == m->config.max_statement_bytes) return MF_LIMIT;
            m->output[(*used)++] = s.data[i++];
        }
    }
    out->data = m->output + start; out->length = *used - start;
    return MF_OK;
}
static enum mf_status expand(struct mf_macro *m, const struct mf_statement *s,
    struct mf_statement *out)
{
    size_t used; enum mf_status st;
    used = 0; out->origin = s->origin;
    if (s->label.length && s->label.data[0] == 0x2e) {
        if (!name_valid(slice(s->label, 1, s->label.length - 1))) return MF_SOURCE;
        out->label.data = m->output; out->label.length = 0;
    }
    else { st = substitute(m, s->label, &used, &out->label); if (st != MF_OK) return st; }
    st = substitute(m, s->operation, &used, &out->operation); if (st != MF_OK) return st;
    st = substitute(m, s->operand, &used, &out->operand); if (st != MF_OK) return st;
    if ((out->label.length && !name_valid(out->label)) || !name_valid(out->operation)) return MF_SOURCE;
    return MF_OK;
}
static enum mf_status conditional(struct mf_macro *m, const struct mf_statement *s, size_t op)
{
    struct mf_span expression, target, name; struct mf_macro_value value;
    size_t used, at, i, start, level; enum mf_status st; struct variable *v; int quoted, branch;
    if (op == 16) {
        size_t cursor; struct mf_span severity, message; struct mf_macro_value code;
        cursor = 0;
        st = mf_macro_arguments(s->operand, &cursor, &severity); if (st != MF_OK) return st;
        st = mf_macro_eval(severity, 32, &code); if (st != MF_OK) return st;
        st = mf_macro_arguments(s->operand, &cursor, &message);
        if (st != MF_OK || code.character || cursor != NONE) return MF_SOURCE;
        return code.number >= 8 && code.number <= 255 ? MF_SOURCE : MF_UNSUPPORTED;
    }
    if (op >= 7 && op <= 12) return declare(m, s, op);
    if (op >= 13 && op <= 15) {
        if (!s->label.length || s->label.data[0] != 0x26) return MF_SOURCE;
        name = slice(s->label, 1, s->label.length - 1);
        at = variable_find(m, m->depth + 1, name);
        if (at == NONE) {
            struct mf_statement declaration; declaration = *s;
            declaration.label.length = 0; declaration.operand = s->label;
            st = declare(m, &declaration, op - 6); if (st != MF_OK) return st;
            at = variable_find(m, m->depth + 1, name);
        }
        if (m->variables[at].global) at = m->variables[at].global - 1;
        v = m->variables + at; if (v->type != (int)(op - 13)) return MF_SOURCE;
        used = 0; st = substitute(m, s->operand, &used, &expression); if (st != MF_OK) return st;
        st = mf_macro_eval(expression, 32, &value); if (st != MF_OK) return st;
        if (v->type == 2) {
            if (!value.character) return MF_SOURCE;
            v->used = 0;
            for (i = 0; i < value.text.length; ++i) {
                mf_octet c; c = value.text.data[i];
                if (c == 0x27 && i + 1 < value.text.length && value.text.data[i + 1] == 0x27) ++i;
                m->variable_text[at * m->config.max_statement_bytes + v->used++] = c;
            }
        } else {
            if (value.character || (v->type == 1 && value.number != 0 && value.number != 1)) return MF_SOURCE;
            v->number = value.number;
        }
        return MF_OK;
    }
    if (op == 5) return empty(s->operand) ? MF_OK : MF_SOURCE;
    if (op == 6) {
        if (!m->depth) return MF_UNSUPPORTED;
        if (!empty(s->operand)) return MF_SOURCE;
        --m->depth; return MF_OK;
    }
    branch = 1; target = s->operand;
    if (op == 3) {
        if (!target.length || target.data[0] != 0x28) return MF_SOURCE;
        level = 1; quoted = 0; start = 1;
        for (i = 1; i < target.length; ++i) {
            if (!quoted && i + 2 < target.length && (mf_macro_upper(target.data[i]) == 0x54 || mf_macro_upper(target.data[i]) == 0x4b ||
                mf_macro_upper(target.data[i]) == 0x4e) &&
                target.data[i + 1] == 0x27 && target.data[i + 2] == 0x26) { ++i; continue; }
            if (target.data[i] == 0x27) quoted = !quoted;
            else if (!quoted && target.data[i] == 0x28) ++level;
            else if (!quoted && target.data[i] == 0x29 && --level == 0) break;
        }
        if (i == target.length) return MF_SOURCE;
        expression = slice(target, start, i - start);
        target = slice(target, i + 1, target.length - i - 1);
        used = 0; st = substitute(m, expression, &used, &expression); if (st != MF_OK) return st;
        st = mf_macro_eval(expression, 32, &value); if (st != MF_OK) return st;
        if (value.character || (value.number != 0 && value.number != 1)) return MF_SOURCE;
        branch = value.number != 0;
    }
    if (!target.length || target.data[0] != 0x2e ||
        !name_valid(slice(target, 1, target.length - 1))) return MF_SOURCE;
    if (!branch) return MF_OK;
    if (m->depth) {
        struct definition *d; d = m->definitions + m->frames[m->depth - 1].definition;
        for (i = 0; i < d->count; ++i) if (mf_macro_same(target, m->models[d->first + i].label)) break;
        if (i == d->count) return MF_UNDEFINED;
        m->frames[m->depth - 1].pc = i;
    } else {
        if (target.length > sizeof m->skip_name) return MF_LIMIT;
        memcpy(m->skip_name, target.data, target.length); m->skip_length = target.length;
    }
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
                if (m->skip_length) { st = MF_UNDEFINED; break; }
                if (m->member_depth) { st = member_pop(m); if (st != MF_OK) break; continue; }
                if (m->replaying && !hashes_equal(&m->first, &m->current)) st = MF_REPLAY;
                else {
                    if (!m->replaying) m->first = m->current;
                    m->complete = 1; m->terminal = MF_EOF; return MF_EOF;
                }
            }
            if (st != MF_OK) break;
            if (m->skip_length) {
                struct mf_span target; target.data = m->skip_name; target.length = m->skip_length;
                if (word(s.operation,0)) {
                    if (m->skip_nesting == m->config.max_depth) { st = MF_LIMIT; break; }
                    ++m->skip_nesting;
                } else if (word(s.operation,1) && m->skip_nesting) { --m->skip_nesting; continue; }
                if (m->skip_nesting || !mf_macro_same(target, s.label)) continue;
                m->skip_length = 0;
            }
            if (word(s.operation, 0)) { st = capture(m, &s); if (st != MF_OK) break; continue; }
        }
        if (word(s.operation, 0) || word(s.operation, 1) || blocked(s.operation)) {
            st = MF_UNSUPPORTED; break;
        }
        for (k = 3; k <= 16; ++k) if (word(s.operation, k)) break;
        if (k <= 16) { st = conditional(m, &s, k); if (st != MF_OK) break; continue; }
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
    m->invocation_index = 0; m->skip_length = m->skip_nesting = 0;
    memset(m->variable_counts, 0, (m->config.max_depth + 2) * sizeof(size_t));
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
    if (c->max_depth > (size_t)-1 - 2 ||
        c->max_model_statements > (size_t)-1 / (c->max_depth + 2)) return MF_LIMIT;
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
    ALLOC(logical, c->max_statement_bytes, mf_octet);
    ALLOC(members, c->max_depth, struct member);
#undef ALLOC
    m->variable_slots = c->max_model_statements * (c->max_depth + 2);
    st = allocate(storage, m->variable_slots, sizeof(struct variable), &p); if (st != MF_OK) return st;
    m->variables = (struct variable *)p;
    st = allocate(storage, m->variable_slots, c->max_statement_bytes, &p); if (st != MF_OK) return st;
    m->variable_text = (mf_octet *)p;
    st = allocate(storage, c->max_depth + 2, sizeof(size_t), &p); if (st != MF_OK) return st;
    m->variable_counts = (size_t *)p; memset(p, 0, (c->max_depth + 2) * sizeof(size_t));
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
