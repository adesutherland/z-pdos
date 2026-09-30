/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Original bounded two-pass bootstrap assembler. Text constants below are
 * numeric ASCII: this translation unit does not assume an ASCII C host.
 */
#include "mf_classic.h"
#include <string.h>

#define U32MAX 0xffffffffUL
#define NIL ((size_t)-1)

struct value {
    struct mf_u64 magnitude;
    int negative, coefficient;
    enum mf_reference_kind kind;
    unsigned target;
};
struct symbol_state { struct value value; int defined; };
struct section_state { mf_u32 position; unsigned mode_mask; };
struct base_state { int active; unsigned section; mf_u32 offset; };
struct fingerprint { mf_u32 a, b; struct mf_u64 bytes; unsigned long count; int complete; };
struct mf_as {
    struct mf_as_config config;
    struct mf_storage storage;
    size_t requested, section_count, symbol_count, fixup_count, current;
    struct mf_section *sections;
    struct section_state *section_state;
    struct mf_symbol *symbols;
    struct symbol_state *symbol_state;
    struct base_state bases[16];
    struct mf_statement statement;
    const struct mf_object_writer *writer;
    const struct mf_diagnostics *diagnostics;
    int pass, ended, used;
    struct mf_entry entry;
};
struct parser { struct mf_as *as; struct mf_span text; size_t at, depth; };

static mf_octet upper(mf_octet c)
{ return c >= 0x61 && c <= 0x7a ? (mf_octet)(c - 0x20) : c; }
static struct mf_span subspan(struct mf_span s, size_t first, size_t length)
{ struct mf_span r; r.data = first ? s.data + first : s.data; r.length = length; return r; }
static struct mf_span trim(struct mf_span s)
{
    while (s.length && s.data[0] == 0x20) { ++s.data; --s.length; }
    while (s.length && s.data[s.length - 1] == 0x20) --s.length;
    return s;
}
static int same_name(struct mf_span a, struct mf_span b)
{
    size_t i;
    if (a.length != b.length) return 0;
    for (i = 0; i < a.length; ++i)
        if (upper(a.data[i]) != upper(b.data[i])) return 0;
    return 1;
}
static int word(struct mf_span s, const mf_octet *p, size_t n)
{ struct mf_span t; t.data = p; t.length = n; return same_name(s, t); }
static int name_start(mf_octet c)
{
    c = upper(c);
    return (c >= 0x41 && c <= 0x5a) || c == 0x40 || c == 0x23 ||
        c == 0x24 || c == 0x5f;
}
static int name_char(mf_octet c)
{ return name_start(c) || (c >= 0x30 && c <= 0x39); }
static int valid_name(struct mf_span s)
{
    size_t i;
    if (!s.length || !name_start(s.data[0])) return 0;
    for (i = 1; i < s.length; ++i) if (!name_char(s.data[i])) return 0;
    return 1;
}
static void *acquire(struct mf_as *as, size_t n)
{
    if (n > (size_t)-1 - as->requested) return NULL;
    as->requested += n;
    return as->storage.acquire(as->storage.cookie, n);
}
static int allocation_size(size_t count, size_t item, size_t *n)
{
    if (item && count > (size_t)-1 / item) return 0;
    *n = count * item;
    return 1;
}
static enum mf_status copy_name(struct mf_as *as, struct mf_span name,
    struct mf_span *out)
{
    mf_octet *p; size_t i;
    if (!valid_name(name)) return MF_SOURCE;
    p = (mf_octet *)acquire(as, name.length);
    if (!p) return MF_LIMIT;
    for (i = 0; i < name.length; ++i) p[i] = upper(name.data[i]);
    out->data = p; out->length = name.length;
    return MF_OK;
}
static struct value zero_value(void)
{
    struct value v;
    memset(&v, 0, sizeof v);
    return v;
}
static int zero(struct mf_u64 a) { return a.hi == 0 && a.lo == 0; }
static int compare(struct mf_u64 a, struct mf_u64 b)
{
    if (a.hi != b.hi) return a.hi < b.hi ? -1 : 1;
    if (a.lo != b.lo) return a.lo < b.lo ? -1 : 1;
    return 0;
}
static struct mf_u64 difference(struct mf_u64 a, struct mf_u64 b)
{
    struct mf_u64 r;
    r.hi = (a.hi - b.hi - (a.lo < b.lo ? 1UL : 0UL)) & U32MAX;
    r.lo = (a.lo - b.lo) & U32MAX;
    return r;
}
static enum mf_status combine(struct value a, struct value b, int subtract,
    struct value *out)
{
    struct value r; enum mf_status status; int cmp;
    r = a;
    if (subtract) { b.negative = !b.negative; b.coefficient = -b.coefficient; }
    if (a.negative == b.negative) {
        status = mf_u64_add(a.magnitude, b.magnitude, &r.magnitude);
        if (status != MF_OK) return status;
    } else {
        cmp = compare(a.magnitude, b.magnitude);
        if (cmp >= 0) r.magnitude = difference(a.magnitude, b.magnitude);
        else { r.magnitude = difference(b.magnitude, a.magnitude); r.negative = b.negative; }
    }
    if (zero(r.magnitude)) r.negative = 0;
    if (!a.coefficient) { r.coefficient = b.coefficient; r.kind = b.kind; r.target = b.target; }
    else if (b.coefficient) {
        if (a.kind != b.kind || a.target != b.target) return MF_UNSUPPORTED;
        r.coefficient = a.coefficient + b.coefficient;
        if (r.coefficient < -1 || r.coefficient > 1) return MF_UNSUPPORTED;
    }
    *out = r;
    return MF_OK;
}
static int value_equal(struct value a, struct value b)
{
    return compare(a.magnitude, b.magnitude) == 0 && a.negative == b.negative &&
        a.coefficient == b.coefficient && (!a.coefficient ||
        (a.kind == b.kind && a.target == b.target));
}
static enum mf_status small_value(struct value v, mf_u32 maximum, mf_u32 *out)
{
    if (v.coefficient) return MF_SOURCE;
    if (v.negative || v.magnitude.hi || v.magnitude.lo > maximum) return MF_RANGE;
    *out = v.magnitude.lo;
    return MF_OK;
}
static size_t find_symbol(struct mf_as *as, struct mf_span name)
{
    size_t i;
    for (i = 0; i < as->symbol_count; ++i)
        if (same_name(as->symbols[i].name, name)) return i;
    return NIL;
}
static enum mf_status add_symbol(struct mf_as *as, struct mf_span name, size_t *index)
{
    enum mf_status status; size_t i;
    if (as->symbol_count == as->config.max_symbols) return MF_LIMIT;
    i = as->symbol_count;
    status = copy_name(as, name, &as->symbols[i].name);
    if (status != MF_OK) return status;
    as->symbols[i].id = (unsigned)(i + 1);
    as->symbols[i].kind = MF_LOCAL;
    as->symbols[i].section = 0; as->symbols[i].offset = 0;
    as->symbol_state[i].defined = 0;
    as->symbol_state[i].value = zero_value();
    ++as->symbol_count; *index = i;
    return MF_OK;
}
static struct value current_value(struct mf_as *as)
{
    struct value v;
    v = zero_value();
    if (as->current != NIL) {
        v.magnitude.lo = as->section_state[as->current].position;
        v.coefficient = 1; v.kind = MF_REF_SECTION;
        v.target = as->sections[as->current].id;
    }
    return v;
}
static enum mf_status define(struct mf_as *as, struct mf_span name, struct value v)
{
    size_t i; enum mf_status status;
    if (!name.length) return MF_OK;
    if (!valid_name(name)) return MF_SOURCE;
    i = find_symbol(as, name);
    if (as->pass == 2) {
        if (i == NIL || !as->symbol_state[i].defined ||
            !value_equal(as->symbol_state[i].value, v)) return MF_REPLAY;
        return MF_OK;
    }
    if (i == NIL) { status = add_symbol(as, name, &i); if (status != MF_OK) return status; }
    if (as->symbol_state[i].defined || as->symbols[i].kind == MF_EXTERNAL) return MF_DUPLICATE;
    if (v.coefficient && (v.kind != MF_REF_SECTION || v.coefficient != 1 ||
        v.negative || v.magnitude.hi)) return MF_UNSUPPORTED;
    if (as->symbols[i].kind == MF_EXPORT && !v.coefficient) return MF_SOURCE;
    as->symbol_state[i].value = v;
    as->symbol_state[i].defined = 1;
    if (v.coefficient) { as->symbols[i].section = v.target; as->symbols[i].offset = v.magnitude.lo; }
    return MF_OK;
}
static void skip(struct parser *p)
{ while (p->at < p->text.length && p->text.data[p->at] == 0x20) ++p->at; }
static enum mf_status expression(struct parser *, struct value *);
static enum mf_status atom(struct parser *p, struct value *out)
{
    mf_octet c; size_t start, i; struct mf_span s; enum mf_status status;
    struct value v; int negate;
    skip(p);
    if (p->at == p->text.length) return MF_SOURCE;
    if (++p->depth > p->as->config.max_expression_depth) { --p->depth; return MF_LIMIT; }
    c = p->text.data[p->at++]; v = zero_value(); status = MF_OK;
    if (c == 0x2b || c == 0x2d) {
        negate = c == 0x2d;
        status = atom(p, &v);
        if (negate) { if (!zero(v.magnitude)) v.negative = !v.negative; v.coefficient = -v.coefficient; }
    } else if (c == 0x28) {
        status = expression(p, &v); skip(p);
        if (status == MF_OK && (p->at == p->text.length || p->text.data[p->at++] != 0x29)) status = MF_SOURCE;
    } else if (c == 0x2a) {
        if (p->as->current == NIL) status = MF_SOURCE;
        else v = current_value(p->as);
    } else if (c >= 0x30 && c <= 0x39) {
        start = p->at - 1;
        while (p->at < p->text.length && p->text.data[p->at] >= 0x30 && p->text.data[p->at] <= 0x39) ++p->at;
        s = subspan(p->text, start, p->at - start);
        status = mf_u64_parse(s, &v.magnitude, &v.negative);
    } else if (upper(c) == 0x58 && p->at < p->text.length && p->text.data[p->at] == 0x27) {
        ++p->at; start = p->at;
        while (p->at < p->text.length && p->text.data[p->at] != 0x27) {
            c = upper(p->text.data[p->at++]);
            if (c >= 0x30 && c <= 0x39) i = (size_t)(c - 0x30);
            else if (c >= 0x41 && c <= 0x46) i = (size_t)(c - 0x41 + 10);
            else { status = MF_SOURCE; break; }
            if (v.magnitude.hi > 0x0fffffffUL) { status = MF_RANGE; break; }
            v.magnitude.hi = ((v.magnitude.hi << 4) | (v.magnitude.lo >> 28)) & U32MAX;
            v.magnitude.lo = ((v.magnitude.lo << 4) | (mf_u32)i) & U32MAX;
        }
        if (status == MF_OK && (p->at == start || p->at == p->text.length)) status = MF_SOURCE;
        if (status == MF_OK) ++p->at;
    } else if (name_start(c)) {
        start = p->at - 1;
        while (p->at < p->text.length && name_char(p->text.data[p->at])) ++p->at;
        s = subspan(p->text, start, p->at - start); i = find_symbol(p->as, s);
        if (i == NIL || !p->as->symbol_state[i].defined) status = MF_UNDEFINED;
        else v = p->as->symbol_state[i].value;
    } else status = c == 0x3d || c == 0x26 ? MF_UNSUPPORTED : MF_SOURCE;
    --p->depth;
    if (status == MF_OK) *out = v;
    return status;
}
static enum mf_status expression(struct parser *p, struct value *out)
{
    struct value a, b; enum mf_status status; mf_octet c;
    status = atom(p, &a);
    while (status == MF_OK) {
        skip(p);
        if (p->at == p->text.length) break;
        c = p->text.data[p->at];
        if (c != 0x2b && c != 0x2d) break;
        ++p->at; status = atom(p, &b);
        if (status == MF_OK) status = combine(a, b, c == 0x2d, &a);
    }
    if (status == MF_OK) *out = a;
    return status;
}
static enum mf_status evaluate(struct mf_as *as, struct mf_span s, struct value *v)
{
    struct parser p; enum mf_status status;
    p.as = as; p.text = s; p.at = 0; p.depth = 0;
    status = expression(&p, v); skip(&p);
    if (status == MF_OK && p.at != p.text.length)
        status = p.text.data[p.at] == 0x2a || p.text.data[p.at] == 0x2f ? MF_UNSUPPORTED : MF_SOURCE;
    return status;
}
/* Split only at top-level commas. Nested parentheses/quotes belong to values. */
static enum mf_status split(struct mf_span s, struct mf_span *parts, size_t capacity, size_t *count)
{
    size_t i, first, n, depth; int quoted;
    first = n = depth = 0; quoted = 0;
    for (i = 0; i <= s.length; ++i) {
        if (i < s.length && s.data[i] == 0x27) {
            if (quoted && i + 1 < s.length && s.data[i + 1] == 0x27) { ++i; continue; }
            quoted = !quoted;
        } else if (!quoted && i < s.length && s.data[i] == 0x28) ++depth;
        else if (!quoted && i < s.length && s.data[i] == 0x29) {
            if (!depth) return MF_SOURCE;
            --depth;
        }
        if (i == s.length || (!quoted && !depth && s.data[i] == 0x2c)) {
            if (n == capacity) return MF_UNSUPPORTED;
            parts[n++] = trim(subspan(s, first, i - first)); first = i + 1;
        }
    }
    if (quoted || depth) return MF_SOURCE;
    *count = n; return MF_OK;
}
static enum mf_status number(struct mf_as *as, struct mf_span s, mf_u32 max, mf_u32 *out)
{
    struct value v; enum mf_status status;
    status = evaluate(as, s, &v);
    return status == MF_OK ? small_value(v, max, out) : status;
}
static enum mf_status ensure_section(struct mf_as *as)
{
    struct mf_span empty;
    if (as->current != NIL) return MF_OK;
    if (as->pass == 2) {
        if (!as->section_count || as->sections[0].name.length) return MF_REPLAY;
        as->current = 0; return MF_OK;
    }
    if (as->section_count == as->config.max_sections) return MF_LIMIT;
    empty.data = NULL; empty.length = 0;
    as->current = as->section_count++;
    as->sections[as->current].id = (unsigned)(as->current + 1);
    as->sections[as->current].name = empty;
    as->sections[as->current].length = 0;
    as->sections[as->current].amode = 24; as->sections[as->current].rmode = 24;
    as->sections[as->current].dummy = 0;
    as->section_state[as->current].position = 0;
    return MF_OK;
}
static enum mf_status select_section(struct mf_as *as, int dummy)
{
    size_t i, symbol; enum mf_status status; int declared;
    if (as->statement.operand.length || !valid_name(as->statement.label)) return MF_SOURCE;
    for (i = 0; i < as->section_count; ++i)
        if (same_name(as->sections[i].name, as->statement.label)) break;
    declared = i != as->section_count;
    if (!declared) {
        if (as->pass == 2) return MF_REPLAY;
        if (i == as->config.max_sections) return MF_LIMIT;
        status = copy_name(as, as->statement.label, &as->sections[i].name);
        if (status != MF_OK) return status;
        as->sections[i].id = (unsigned)(i + 1); as->sections[i].length = 0;
        as->sections[i].amode = 24; as->sections[i].rmode = 24;
        as->sections[i].dummy = dummy;
        as->section_state[i].position = 0; as->section_state[i].mode_mask = 0;
        ++as->section_count;
    } else if (as->sections[i].dummy != dummy) return as->pass == 1 ? MF_DUPLICATE : MF_REPLAY;
    as->current = i;
    if (declared) {
        symbol = find_symbol(as, as->statement.label);
        if (symbol == NIL || !as->symbol_state[symbol].defined ||
            as->symbols[symbol].section != as->sections[i].id ||
            as->symbols[symbol].offset != 0) return MF_REPLAY;
        return MF_OK; /* Declaration identity is independent of position. */
    }
    return define(as, as->statement.label, current_value(as));
}
static enum mf_status advance(struct mf_as *as, mf_u32 n, const mf_octet *bytes)
{
    struct section_state *s; enum mf_status status; struct mf_section *section;
    s = &as->section_state[as->current]; section = &as->sections[as->current];
    if (n > U32MAX - s->position) return MF_RANGE;
    status = MF_OK;
    if (as->pass == 2 && !section->dummy && n) {
        if (bytes) status = as->writer->text(as->writer->cookie, section->id, s->position, bytes, (size_t)n);
        else status = as->writer->gap(as->writer->cookie, section->id, s->position, n);
    }
    if (status != MF_OK) return status;
    s->position += n; return MF_OK;
}
static enum mf_status align_position(struct mf_as *as, unsigned alignment)
{
    mf_u32 remainder;
    remainder = as->section_state[as->current].position % alignment;
    return remainder ? advance(as, alignment - remainder, NULL) : MF_OK;
}
static enum mf_status symbol_list(struct mf_as *as, int external)
{
    struct mf_span s, name; size_t at, start, i; enum mf_status status; struct value v;
    if (as->statement.label.length) return MF_SOURCE;
    s = as->statement.operand; at = 0;
    if (!s.length) return MF_SOURCE;
    while (at < s.length) {
        start = at;
        while (at < s.length && s.data[at] != 0x2c) ++at;
        name = trim(subspan(s, start, at - start));
        if (!valid_name(name)) return MF_SOURCE;
        i = find_symbol(as, name);
        if (as->pass == 2) {
            if (i == NIL || as->symbols[i].kind != (external ? MF_EXTERNAL : MF_EXPORT)) return MF_REPLAY;
        } else {
            if (i == NIL) { status = add_symbol(as, name, &i); if (status != MF_OK) return status; }
            if (external) {
                if (as->symbol_state[i].defined || as->symbols[i].kind != MF_LOCAL) return MF_DUPLICATE;
                as->symbols[i].kind = MF_EXTERNAL; as->symbol_state[i].defined = 1;
                v = zero_value(); v.coefficient = 1; v.kind = MF_REF_EXTERNAL; v.target = as->symbols[i].id;
                as->symbol_state[i].value = v;
            } else {
                if (as->symbols[i].kind == MF_EXTERNAL) return MF_DUPLICATE;
                if (as->symbol_state[i].defined && !as->symbol_state[i].value.coefficient) return MF_SOURCE;
                as->symbols[i].kind = MF_EXPORT;
            }
        }
        if (at < s.length) { ++at; if (at == s.length) return MF_SOURCE; }
    }
    return MF_OK;
}
static enum mf_status mode(struct mf_as *as, int address)
{
    static const mf_octet any[] = { 0x41, 0x4e, 0x59 };
    mf_u32 n; enum mf_status status; unsigned mask; struct section_state *state;
    if (as->current == NIL || as->sections[as->current].dummy) return MF_SOURCE;
    if (as->statement.label.length && !same_name(as->statement.label, as->sections[as->current].name)) return MF_SOURCE;
    if (!address && word(as->statement.operand, any, sizeof any)) n = 31;
    else {
        status = number(as, as->statement.operand, 31, &n);
        if (status != MF_OK) return status;
        if (n != 24 && (address ? n != 31 : 1)) return MF_UNSUPPORTED;
    }
    state = &as->section_state[as->current]; mask = address ? 1U : 2U;
    if (as->pass == 2 || (state->mode_mask & mask)) {
        if ((address ? as->sections[as->current].amode : as->sections[as->current].rmode) != n)
            return as->pass == 2 ? MF_REPLAY : MF_SOURCE;
    } else if (address) as->sections[as->current].amode = (unsigned)n;
    else as->sections[as->current].rmode = (unsigned)n;
    state->mode_mask |= mask; return MF_OK;
}
static enum mf_status using_statement(struct mf_as *as, int drop)
{
    struct mf_span parts[2]; size_t count; struct value v; mf_u32 reg;
    enum mf_status status;
    if (as->statement.label.length) return MF_SOURCE;
    status = split(as->statement.operand, parts, 2, &count);
    if (status != MF_OK) return status;
    if (drop) {
        size_t i;
        for (i = 0; i < count; ++i) {
            status = number(as, parts[i], 15, &reg); if (status != MF_OK) return status;
            as->bases[reg].active = 0;
        }
        return MF_OK;
    }
    if (count != 2) return MF_SOURCE;
    status = evaluate(as, parts[0], &v); if (status != MF_OK) return status;
    if (v.coefficient != 1 || v.kind != MF_REF_SECTION || v.negative || v.magnitude.hi) return MF_SOURCE;
    status = number(as, parts[1], 15, &reg); if (status != MF_OK) return status;
    if (!reg) return MF_RANGE;
    as->bases[reg].active = 1; as->bases[reg].section = v.target;
    as->bases[reg].offset = v.magnitude.lo; return MF_OK;
}
/* Address operand: displacement, optional index/length, and explicit base.
 * A symbolic displacement needs a matching USING even with explicit base. */
static enum mf_status address(struct mf_as *as, struct mf_span s, int indexed,
    int length_field, unsigned max_length, mf_u32 *disp, unsigned *base,
    unsigned *index_or_length)
{
    struct mf_span fields[2], value_text; size_t i, open, count, depth; struct value v;
    enum mf_status status; mf_u32 n, d, best; unsigned r, chosen; int explicit_base, group_comma;
    s = trim(s); open = NIL; explicit_base = 0; *index_or_length = 0;
    depth = 0; group_comma = 0;
    for (i = 0; i < s.length; ++i) {
        if (s.data[i] == 0x28) {
            if (!depth) { open = i; group_comma = 0; }
            ++depth;
        } else if (s.data[i] == 0x29) { if (!depth) return MF_SOURCE; --depth; }
        else if (s.data[i] == 0x2c && depth == 1) group_comma = 1;
    }
    if (depth) return MF_SOURCE;
    /* A suffix follows a complete displacement expression. An initial group,
     * or one following an arithmetic sign, belongs to that expression. */
    if (open != NIL && ((!open && !group_comma) || s.data[s.length - 1] != 0x29 ||
        (open && (s.data[open - 1] == 0x2b || s.data[open - 1] == 0x2d)))) open = NIL;
    if (open != NIL) {
        if (!s.length || s.data[s.length - 1] != 0x29) return MF_SOURCE;
        value_text = trim(subspan(s, 0, open));
        status = split(subspan(s, open + 1, s.length - open - 2), fields, 2, &count);
        if (status != MF_OK) return status;
        if (indexed || length_field) {
            if (count == 2) {
                if (fields[0].length) {
                    status = number(as, fields[0], length_field ? max_length : 15, &n);
                    if (status != MF_OK) return status;
                    if (length_field && !n) return MF_RANGE;
                    *index_or_length = (unsigned)n;
                } else if (length_field) return MF_SOURCE;
                status = number(as, fields[1], 15, &n);
            } else if (count == 1 && !length_field) status = number(as, fields[0], 15, &n);
            else return MF_SOURCE;
        } else {
            if (count != 1) return MF_SOURCE;
            status = number(as, fields[0], 15, &n);
        }
        if (status != MF_OK) return status;
        *base = (unsigned)n; explicit_base = 1;
    } else {
        if (length_field) return MF_SOURCE;
        value_text = s;
    }
    if (!value_text.length) v = zero_value();
    else {
        status = evaluate(as, value_text, &v);
        /* Forward instruction labels do not affect layout. Pass 2 resolves. */
        if (status == MF_UNDEFINED && as->pass == 1) { *disp = 0; if (!explicit_base) *base = 0; return MF_OK; }
        if (status != MF_OK) return status;
    }
    if (!v.coefficient) {
        status = small_value(v, 4095, disp);
        if (status != MF_OK) return status;
        if (!explicit_base) *base = 0;
        return MF_OK;
    }
    if (v.coefficient != 1 || v.kind != MF_REF_SECTION || v.negative || v.magnitude.hi) return MF_SOURCE;
    chosen = 0; best = 4096;
    for (r = 1; r <= 15; ++r) {
        if (explicit_base && r != *base) continue;
        if (!as->bases[r].active || as->bases[r].section != v.target || v.magnitude.lo < as->bases[r].offset) continue;
        d = v.magnitude.lo - as->bases[r].offset;
        if (d <= 4095 && d <= best) { chosen = r; best = d; }
    }
    if (!chosen) return MF_RANGE;
    *base = chosen; *disp = best; return MF_OK;
}
static enum mf_status instruction(struct mf_as *as, const struct mf_instruction *ins)
{
    struct mf_span parts[3]; size_t count; struct mf_operands operands;
    enum mf_status status; mf_u32 n; mf_octet bytes[6]; unsigned length;
    n = 0;
    status = ensure_section(as); if (status != MF_OK) return status;
    if (as->sections[as->current].dummy) return MF_SOURCE;
    status = align_position(as, 2); if (status != MF_OK) return status;
    status = define(as, as->statement.label, current_value(as)); if (status != MF_OK) return status;
    status = split(as->statement.operand, parts, 3, &count); if (status != MF_OK) return status;
    memset(&operands, 0, sizeof operands);
    if (ins->format == MF_RR) {
        if (ins->opcode == 0x0a) {
            if (count != 1) return MF_SOURCE;
            status = number(as, parts[0], 255, &operands.immediate);
        } else {
            if (count != 2) return MF_SOURCE;
            status = number(as, parts[0], 15, &n); operands.r1 = (unsigned)n;
            if (status == MF_OK) { status = number(as, parts[1], 15, &n); operands.r2 = (unsigned)n; }
        }
    } else if (ins->format == MF_RX) {
        if (count != 2) return MF_SOURCE;
        status = number(as, parts[0], 15, &n); operands.r1 = (unsigned)n;
        if (status == MF_OK) status = address(as, parts[1], 1, 0, 0, &operands.d2, &operands.b2, &operands.x2);
    } else if (ins->format == MF_RS) {
        if (ins->opcode >= 0x88 && ins->opcode <= 0x8f) {
            if (count != 2) return MF_SOURCE;
            status = number(as, parts[0], 15, &n); operands.r1 = (unsigned)n;
            if (status == MF_OK) status = address(as, parts[1], 0, 0, 0, &operands.d2, &operands.b2, &operands.x2);
        } else {
            if (count != 3) return MF_SOURCE;
            status = number(as, parts[0], 15, &n); operands.r1 = (unsigned)n;
            if (status == MF_OK) { status = number(as, parts[1], 15, &n); operands.r3 = (unsigned)n; }
            if (status == MF_OK) status = address(as, parts[2], 0, 0, 0, &operands.d2, &operands.b2, &operands.x2);
        }
    } else if (ins->format == MF_SI) {
        if (count != 2) return MF_SOURCE;
        status = address(as, parts[0], 0, 0, 0, &operands.d1, &operands.b1, &operands.x2);
        if (status == MF_OK) status = number(as, parts[1], 255, &operands.immediate);
    } else if (ins->format == MF_SS) {
        if (count != 2) return MF_SOURCE;
        status = address(as, parts[0], 0, 1, ins->opcode >= 0xf0 ? 16U : 256U,
            &operands.d1, &operands.b1, &operands.length1);
        if (status == MF_OK) status = address(as, parts[1], 0, ins->opcode >= 0xf0, 16,
            &operands.d2, &operands.b2, &operands.length2);
    } else return MF_UNSUPPORTED;
    if (status != MF_OK) return status;
    status = mf_encode(as->config.profile, ins, &operands, bytes, sizeof bytes, &length);
    if (status != MF_OK) return status;
    if (length != ins->length || length > sizeof bytes) return MF_OBJECT;
    return advance(as, (mf_u32)length, bytes);
}

struct constant {
    unsigned type, width, alignment;
    mf_u32 repeat;
    struct mf_span value;
    int explicit_length;
};
static int hex_digit(mf_octet c)
{
    c = upper(c);
    if (c >= 0x30 && c <= 0x39) return c - 0x30;
    if (c >= 0x41 && c <= 0x46) return c - 0x41 + 10;
    return -1;
}
static enum mf_status constant_parse(struct mf_as *as, struct mf_span s, int reserve,
    struct constant *c)
{
    size_t at, start, i; mf_u32 n; enum mf_status status; unsigned type; mf_octet byte;
    memset(c, 0, sizeof *c); s = trim(s); at = 0; c->repeat = 1;
    while (at < s.length && s.data[at] >= 0x30 && s.data[at] <= 0x39) ++at;
    if (at) { status = number(as, subspan(s, 0, at), U32MAX, &c->repeat); if (status != MF_OK) return status; }
    if (at == s.length) return MF_SOURCE;
    type = upper(s.data[at++]); c->type = type;
    if (type == 0x41 && at < s.length && upper(s.data[at]) == 0x44) { ++at; c->type = 0x44; }
    if (type == 0x48) c->width = c->alignment = 2;
    else if (type == 0x46 || type == 0x41 || type == 0x56) c->width = c->alignment = c->type == 0x44 ? 8 : 4;
    else if (type == 0x44 && reserve) c->width = c->alignment = 8;
    else if (type == 0x43 || type == 0x58) c->width = c->alignment = 1;
    else return MF_UNSUPPORTED;
    if (at < s.length && upper(s.data[at]) == 0x4c) {
        ++at; start = at;
        while (at < s.length && s.data[at] >= 0x30 && s.data[at] <= 0x39) ++at;
        if (at == start) return MF_SOURCE;
        status = number(as, subspan(s, start, at - start), UINT_MAX, &n);
        if (status != MF_OK) return status;
        if (!n) return MF_RANGE;
        c->explicit_length = 1; c->width = (unsigned)n; c->alignment = 1;
        if (type != 0x43 && type != 0x58) return MF_UNSUPPORTED;
    }
    if (reserve) return at == s.length ? MF_OK : MF_UNSUPPORTED;
    if (!c->repeat) return MF_RANGE;
    if (at == s.length) return MF_SOURCE;
    if (type == 0x41 || type == 0x56) {
        if (s.data[at] != 0x28 || s.data[s.length - 1] != 0x29) return MF_SOURCE;
        c->value = trim(subspan(s, at + 1, s.length - at - 2));
        return c->value.length ? MF_OK : MF_SOURCE;
    }
    if (s.data[at] != 0x27 || s.data[s.length - 1] != 0x27 || s.length - at < 2) return MF_SOURCE;
    c->value = subspan(s, at + 1, s.length - at - 2);
    if (type == 0x58) {
        if (!c->value.length) return MF_SOURCE;
        for (i = 0; i < c->value.length; ++i) if (hex_digit(c->value.data[i]) < 0) return MF_SOURCE;
        if (c->value.length / 2 + c->value.length % 2 > UINT_MAX) return MF_RANGE;
        n = (mf_u32)(c->value.length / 2 + c->value.length % 2);
        if (!c->explicit_length) c->width = (unsigned)n;
        else if (n > c->width) return MF_RANGE;
    } else if (type == 0x43) {
        n = 0;
        for (i = 0; i < c->value.length; ++i) {
            byte = c->value.data[i];
            if (byte == 0x27) {
                if (i + 1 == c->value.length || c->value.data[++i] != 0x27) return MF_SOURCE;
            }
            status = mf_ascii_to_ebcdic(byte, &byte); if (status != MF_OK) return status;
            if (n == UINT_MAX) return MF_RANGE;
            ++n;
        }
        if (!c->explicit_length) c->width = (unsigned)n;
        else if (n > c->width) return MF_RANGE;
        if (!c->width) return MF_RANGE;
    }
    return MF_OK;
}
static enum mf_status numeric_bytes(struct value v, unsigned width, int signed_only,
    mf_octet *bytes)
{
    struct mf_u64 limit, bits;
    limit.hi = 0; limit.lo = U32MAX;
    if (width == 2) limit.lo = v.negative ? 32768UL : 32767UL;
    else if (width == 4 && !v.coefficient)
        limit.lo = v.negative ? 0x80000000UL : (signed_only ? 0x7fffffffUL : U32MAX);
    else if (width == 8) { limit.hi = v.negative ? 0x80000000UL : U32MAX; limit.lo = v.negative ? 0 : U32MAX; }
    else if (width != 4) return MF_UNSUPPORTED;
    if (compare(v.magnitude, limit) > 0) return MF_RANGE;
    bits = v.magnitude;
    if (v.negative) mf_u64_negate(bits, &bits);
    mf_u64_store(bits, bytes, width); return MF_OK;
}
static enum mf_status emit_constant(struct mf_as *as, const struct constant *c)
{
    enum mf_status status; struct value v; struct mf_fixup fixup;
    mf_octet bytes[256]; mf_u32 repetition, offset; size_t i, written, chunk, input;
    unsigned width; int nibble;
    v = zero_value();
    if (c->type == 0x48 || c->type == 0x46 || c->type == 0x41 || c->type == 0x44 || c->type == 0x56) {
        status = evaluate(as, c->value, &v);
        if (status == MF_UNDEFINED && as->pass == 1) return advance(as, c->width * c->repeat, NULL);
        if (status != MF_OK) return status;
        if ((c->type == 0x48 || c->type == 0x46 || c->type == 0x44) && v.coefficient) return MF_UNSUPPORTED;
        if (c->type == 0x56 && (v.coefficient != 1 || v.kind != MF_REF_EXTERNAL ||
            !zero(v.magnitude) || v.negative)) return MF_SOURCE;
        if (v.coefficient && as->sections[as->current].dummy) return MF_SOURCE;
        if (v.coefficient && (as->fixup_count > as->config.max_fixups ||
            c->repeat > as->config.max_fixups - as->fixup_count)) return MF_LIMIT;
        status = numeric_bytes(v, c->width, c->type == 0x48 || c->type == 0x46, bytes);
        if (status != MF_OK) return status;
        if (as->pass == 1) {
            if (v.coefficient) as->fixup_count += (size_t)c->repeat;
            return advance(as, c->width * c->repeat, NULL);
        }
        for (repetition = 0; repetition < c->repeat; ++repetition) {
            offset = as->section_state[as->current].position;
            status = advance(as, c->width, bytes); if (status != MF_OK) return status;
            if (v.coefficient) {
                ++as->fixup_count;
                if (as->pass == 2) {
                    fixup.section = as->sections[as->current].id; fixup.offset = offset;
                    fixup.target_kind = v.kind; fixup.target = v.target;
                    fixup.address_kind = c->type == 0x56 ? MF_ADDRESS_V : MF_ADDRESS_A;
                    fixup.width = c->width; fixup.subtract = v.coefficient < 0;
                    status = as->writer->fixup(as->writer->cookie, &fixup); if (status != MF_OK) return status;
                }
            }
        }
        return MF_OK;
    }
    if (as->pass == 1) return advance(as, c->width * c->repeat, NULL);
    /* Character/hex data stream in small chunks, independent of repeat count. */
    for (repetition = 0; repetition < c->repeat; ++repetition) {
        written = input = 0;
        while (written < c->width) {
            chunk = c->width - written; if (chunk > sizeof bytes) chunk = sizeof bytes;
            for (i = 0; i < chunk; ++i) {
                if (c->type == 0x43) {
                    if (input < c->value.length) {
                        bytes[i] = c->value.data[input++];
                        if (bytes[i] == 0x27) ++input;
                        status = mf_ascii_to_ebcdic(bytes[i], &bytes[i]); if (status != MF_OK) return status;
                    } else bytes[i] = 0x40;
                } else {
                    width = (unsigned)(c->value.length / 2 + c->value.length % 2);
                    if (written + i < c->width - width) bytes[i] = 0;
                    else {
                        nibble = 0;
                        if (input || !(c->value.length % 2)) nibble = hex_digit(c->value.data[input++]);
                        bytes[i] = (mf_octet)((nibble << 4) | hex_digit(c->value.data[input++]));
                    }
                }
            }
            status = advance(as, (mf_u32)chunk, bytes); if (status != MF_OK) return status;
            written += chunk;
        }
    }
    return MF_OK;
}
static enum mf_status data_statement(struct mf_as *as, int reserve)
{
    struct mf_span s, item; struct constant c; enum mf_status status;
    size_t at, first, depth; int quoted, first_item; mf_u32 total;
    status = ensure_section(as); if (status != MF_OK) return status;
    s = as->statement.operand; at = first = depth = 0; quoted = 0; first_item = 1;
    while (at <= s.length) {
        if (at < s.length && s.data[at] == 0x27) {
            if (quoted && at + 1 < s.length && s.data[at + 1] == 0x27) { at += 2; continue; }
            quoted = !quoted;
        } else if (!quoted && at < s.length && s.data[at] == 0x28) ++depth;
        else if (!quoted && at < s.length && s.data[at] == 0x29) { if (!depth) return MF_SOURCE; --depth; }
        if (at == s.length || (!quoted && !depth && s.data[at] == 0x2c)) {
            if (quoted || depth) return MF_SOURCE;
            item = trim(subspan(s, first, at - first));
            status = constant_parse(as, item, reserve, &c); if (status != MF_OK) return status;
            if (c.repeat && c.width > U32MAX / c.repeat) return MF_RANGE;
            total = c.repeat * c.width;
            status = align_position(as, c.alignment); if (status != MF_OK) return status;
            if (first_item) {
                status = define(as, as->statement.label, current_value(as)); if (status != MF_OK) return status;
                first_item = 0;
            }
            if (reserve) status = advance(as, total, NULL);
            else status = emit_constant(as, &c);
            if (status != MF_OK) return status;
            first = at + 1;
        }
        ++at;
    }
    return MF_OK;
}
static enum mf_status end_statement(struct mf_as *as)
{
    struct value v; enum mf_status status;
    if (as->statement.label.length) return MF_SOURCE;
    as->ended = 1;
    if (!as->statement.operand.length) return MF_OK;
    status = evaluate(as, as->statement.operand, &v);
    if (status == MF_UNDEFINED && as->pass == 1) return MF_OK;
    if (status != MF_OK) return status;
    if (v.coefficient != 1 || v.kind != MF_REF_SECTION || v.negative || v.magnitude.hi) return MF_SOURCE;
    if (v.target == 0 || v.target > as->section_count || as->sections[v.target - 1].dummy) return MF_SOURCE;
    if (as->pass == 2) {
        as->entry.present = 1; as->entry.section = v.target; as->entry.offset = v.magnitude.lo;
    }
    return MF_OK;
}
static enum mf_status process(struct mf_as *as)
{
    static const mf_octet csect[] = {0x43,0x53,0x45,0x43,0x54};
    static const mf_octet dsect[] = {0x44,0x53,0x45,0x43,0x54};
    static const mf_octet equ[] = {0x45,0x51,0x55};
    static const mf_octet dc[] = {0x44,0x43}, ds[] = {0x44,0x53};
    static const mf_octet amode[] = {0x41,0x4d,0x4f,0x44,0x45};
    static const mf_octet rmode[] = {0x52,0x4d,0x4f,0x44,0x45};
    static const mf_octet entry[] = {0x45,0x4e,0x54,0x52,0x59};
    static const mf_octet extrn[] = {0x45,0x58,0x54,0x52,0x4e};
    static const mf_octet end[] = {0x45,0x4e,0x44};
    static const mf_octet using[] = {0x55,0x53,0x49,0x4e,0x47};
    static const mf_octet drop[] = {0x44,0x52,0x4f,0x50};
    struct mf_span op; struct value v; const struct mf_instruction *ins; enum mf_status status;
    op = as->statement.operation;
    if (!op.length) return as->statement.label.length || as->statement.operand.length ? MF_SOURCE : MF_OK;
    if (as->ended) return MF_SOURCE;
    if (word(op, csect, sizeof csect)) return select_section(as, 0);
    if (word(op, dsect, sizeof dsect)) return select_section(as, 1);
    if (word(op, equ, sizeof equ)) {
        if (!as->statement.label.length) return MF_SOURCE;
        status = evaluate(as, as->statement.operand, &v);
        return status == MF_OK ? define(as, as->statement.label, v) : status;
    }
    if (word(op, dc, sizeof dc)) return data_statement(as, 0);
    if (word(op, ds, sizeof ds)) return data_statement(as, 1);
    if (word(op, amode, sizeof amode)) return mode(as, 1);
    if (word(op, rmode, sizeof rmode)) return mode(as, 0);
    if (word(op, entry, sizeof entry)) return symbol_list(as, 0);
    if (word(op, extrn, sizeof extrn)) return symbol_list(as, 1);
    if (word(op, using, sizeof using)) return using_statement(as, 0);
    if (word(op, drop, sizeof drop)) return using_statement(as, 1);
    if (word(op, end, sizeof end)) return end_statement(as);
    ins = mf_machine_lookup(as->config.profile, op);
    return ins ? instruction(as, ins) : MF_UNSUPPORTED;
}
static void fingerprint_init(struct fingerprint *f)
{
    f->a = 0x13579bdfUL; f->b = 0x2468ace0UL;
    f->bytes.hi = f->bytes.lo = 0; f->count = 0; f->complete = 0;
}
static void hash_octet(struct fingerprint *f, mf_octet byte)
{
    f->a = ((f->a << 5) | (f->a >> 27)) & U32MAX;
    f->a = (f->a ^ byte ^ 0x9e3779b9UL) & U32MAX;
    f->b = ((f->b << 7) | (f->b >> 25)) & U32MAX;
    f->b = (f->b + (mf_u32)byte + 0x7f4a7c15UL) & U32MAX;
}
static void hash_size(struct fingerprint *f, size_t n)
{
    do { hash_octet(f, (mf_octet)(n & 0xff)); n >>= 8; } while (n);
    hash_octet(f, 0xff);
}
static void hash_ulong(struct fingerprint *f, unsigned long n)
{
    do { hash_octet(f, (mf_octet)(n & 0xff)); n >>= 8; } while (n);
    hash_octet(f, 0xfe);
}
static enum mf_status hash_statement(struct fingerprint *f, const struct mf_statement *s)
{
    struct mf_span fields[3]; size_t i, j; struct mf_u64 n, sum; enum mf_status status;
    if (f->count == ULONG_MAX) return MF_LIMIT;
    ++f->count; fields[0] = s->label; fields[1] = s->operation; fields[2] = s->operand;
    for (i = 0; i < 3; ++i) {
        hash_size(f, fields[i].length);
        n.hi = 0; n.lo = 0;
        /* Count arbitrary size_t lengths without a narrowing conversion. */
        for (j = 0; j < fields[i].length; ++j) {
            if (n.lo == U32MAX) { n.lo = 0; if (n.hi == U32MAX) return MF_LIMIT; ++n.hi; }
            else ++n.lo;
            hash_octet(f, fields[i].data[j]);
        }
        status = mf_u64_add(f->bytes, n, &sum); if (status != MF_OK) return MF_LIMIT;
        f->bytes = sum;
    }
    hash_ulong(f, s->origin.source); hash_ulong(f, s->origin.line); hash_ulong(f, s->origin.column);
    return MF_OK;
}
static enum mf_status run_pass(struct mf_as *as, const struct mf_statements *source,
    struct fingerprint *fingerprint)
{
    enum mf_status status, semantic_error; size_t n, i; struct mf_origin error_origin;
    for (i = 0; i < as->section_count; ++i) as->section_state[i].position = 0;
    memset(as->bases, 0, sizeof as->bases); as->current = NIL; as->ended = 0; as->fixup_count = 0;
    fingerprint_init(fingerprint);
    semantic_error = MF_OK; memset(&error_origin, 0, sizeof error_origin);
    for (;;) {
        memset(&as->statement, 0, sizeof as->statement);
        status = source->next(source->cookie, &as->statement);
        if (status == MF_EOF) break;
        if (status != MF_OK) {
            as->statement.label.data = as->statement.operation.data = as->statement.operand.data = NULL;
            as->statement.label.length = as->statement.operation.length = as->statement.operand.length = 0;
            return status;
        }
        if ((as->statement.label.length && !as->statement.label.data) ||
            (as->statement.operation.length && !as->statement.operation.data) ||
            (as->statement.operand.length && !as->statement.operand.data)) {
            as->statement.label.length = as->statement.operation.length = as->statement.operand.length = 0;
            as->statement.label.data = as->statement.operation.data = as->statement.operand.data = NULL;
            return MF_SOURCE;
        }
        n = as->statement.label.length;
        if (n > as->config.max_statement || as->statement.operation.length > as->config.max_statement - n) return MF_LIMIT;
        n += as->statement.operation.length;
        if (as->statement.operand.length > as->config.max_statement - n) return MF_LIMIT;
        status = hash_statement(fingerprint, &as->statement); if (status != MF_OK) return status;
        if (semantic_error == MF_OK) {
            status = process(as);
            if (status != MF_OK) {
                if (as->pass == 1) return status;
                semantic_error = status; error_origin = as->statement.origin;
                /* Finish fingerprinting replay after an emission error. Do not
                 * emit more events; borrowed spans cannot survive next(). */
            }
        }
    }
    fingerprint->complete = 1;
    if (semantic_error != MF_OK) {
        memset(&as->statement, 0, sizeof as->statement); as->statement.origin = error_origin;
        return semantic_error;
    }
    if (!as->ended || !as->section_count) return MF_SOURCE;
    for (i = 0; i < as->section_count; ++i) {
        if (as->pass == 1) as->sections[i].length = as->section_state[i].position;
        else if (as->sections[i].length != as->section_state[i].position) return MF_REPLAY;
    }
    return MF_OK;
}
enum mf_status mf_as_create(const struct mf_as_config *config,
    const struct mf_storage *storage, struct mf_as **out)
{
    struct mf_as *as; size_t a, b, c, d, total;
    if (out) *out = NULL;
    if (!config || !storage || !storage->acquire || !out) return MF_SOURCE;
    if (config->profile != MF_S360 && config->profile != MF_S370) return MF_UNSUPPORTED;
    if (!config->max_sections || !config->max_symbols || !config->max_statement ||
        !config->max_expression_depth || config->max_sections > UINT_MAX || config->max_symbols > UINT_MAX) return MF_LIMIT;
    if (!allocation_size(config->max_sections, sizeof(struct mf_section), &a) ||
        !allocation_size(config->max_sections, sizeof(struct section_state), &b) ||
        !allocation_size(config->max_symbols, sizeof(struct mf_symbol), &c) ||
        !allocation_size(config->max_symbols, sizeof(struct symbol_state), &d)) return MF_LIMIT;
    total = sizeof *as;
    if (a > (size_t)-1 - total) return MF_LIMIT;
    total += a;
    if (b > (size_t)-1 - total) return MF_LIMIT;
    total += b;
    if (c > (size_t)-1 - total) return MF_LIMIT;
    total += c;
    if (d > (size_t)-1 - total) return MF_LIMIT;
    as = (struct mf_as *)storage->acquire(storage->cookie, sizeof *as);
    if (!as) return MF_LIMIT;
    memset(as, 0, sizeof *as); as->config = *config; as->storage = *storage; as->requested = sizeof *as;
    as->sections = (struct mf_section *)acquire(as, a);
    as->section_state = (struct section_state *)acquire(as, b);
    as->symbols = (struct mf_symbol *)acquire(as, c);
    as->symbol_state = (struct symbol_state *)acquire(as, d);
    if (!as->sections || !as->section_state || !as->symbols || !as->symbol_state) return MF_LIMIT;
    memset(as->section_state, 0, b); as->current = NIL; *out = as;
    return MF_OK;
}
enum mf_status mf_as_assemble(struct mf_as *as, const struct mf_statements *source,
    const struct mf_object_writer *writer, const struct mf_diagnostics *diagnostics,
    struct mf_as_result *result)
{
    enum mf_status status, finish_status; struct fingerprint first, second;
    struct mf_diagnostic diagnostic; size_t i; int begun;
    if (result) { memset(result, 0, sizeof *result); result->status = MF_SOURCE; }
    if (!as || !source || !source->next || !source->replay || !writer ||
        !writer->begin || !writer->text || !writer->gap || !writer->fixup ||
        !writer->entry || !writer->finish || !result) return MF_SOURCE;
    if (as->used) { result->status = MF_SOURCE; return MF_SOURCE; }
    as->used = 1; as->writer = writer; as->diagnostics = diagnostics; begun = 0;
    memset(&as->statement, 0, sizeof as->statement); as->pass = 1;
    fingerprint_init(&first); fingerprint_init(&second);
    status = run_pass(as, source, &first);
    if (status == MF_OK) {
        for (i = 0; i < as->symbol_count; ++i) {
            if (!as->symbol_state[i].defined) { status = MF_UNDEFINED; break; }
            if (as->symbols[i].kind == MF_EXPORT &&
                (!as->symbols[i].section || as->sections[as->symbols[i].section - 1].dummy)) { status = MF_SOURCE; break; }
        }
    }
    if (status == MF_OK) status = source->replay(source->cookie);
    if (status == MF_OK) {
        begun = 1;
        status = writer->begin(writer->cookie, as->sections, as->section_count, as->symbols, as->symbol_count);
    }
    if (status == MF_OK) {
        as->pass = 2; status = run_pass(as, source, &second);
        if (second.complete && (first.a != second.a || first.b != second.b || first.count != second.count ||
            compare(first.bytes, second.bytes))) status = MF_REPLAY;
        if (status == MF_OK) status = writer->entry(writer->cookie, &as->entry);
    }
    if (begun) {
        finish_status = writer->finish(writer->cookie, status == MF_OK);
        if (status == MF_OK) status = finish_status;
    }
    if (status != MF_OK && diagnostics && diagnostics->report) {
        diagnostic.code = status; diagnostic.origin = as->statement.origin;
        diagnostic.detail = as->statement.operation;
        diagnostics->report(diagnostics->cookie, &diagnostic);
    }
    result->status = status; result->statements = first.count;
    result->sections = as->section_count; result->symbols = as->symbol_count;
    result->fixups = as->fixup_count; result->storage_requested = as->requested;
    result->valid_output = status == MF_OK;
    return status;
}
void mf_as_destroy(struct mf_as *as) { (void)as; }
