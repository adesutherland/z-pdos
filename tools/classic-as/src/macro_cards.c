/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Original macro-capable fixed-card scanner. Numeric ASCII lexical codes.
 */
#include "macro_internal.h"
int mf_macro_name_first(mf_octet c)
{
    c = mf_macro_upper(c);
    return (c >= 0x41 && c <= 0x5a) || c == 0x24 || c == 0x23 ||
        c == 0x40 || c == 0x5f;
}
int mf_macro_name_rest(mf_octet c)
{ return mf_macro_name_first(c) || (c >= 0x30 && c <= 0x39); }
mf_octet mf_macro_upper(mf_octet c)
{ return c >= 0x61 && c <= 0x7a ? (mf_octet)(c - 0x20) : c; }
int mf_macro_same(struct mf_span a, struct mf_span b)
{
    size_t i;
    if (a.length != b.length) return 0;
    for (i = 0; i < a.length; ++i)
        if (mf_macro_upper(a.data[i]) != mf_macro_upper(b.data[i])) return 0;
    return 1;
}
static enum mf_status decode(mf_octet c, enum mf_encoding e, mf_octet *out)
{
    *out = c;
    return e == MF_CP037 ? mf_ebcdic_to_ascii(c, out) : MF_OK;
}
enum mf_status mf_macro_card(const struct mf_record *r, enum mf_encoding e,
    mf_octet *b, size_t cap, struct mf_statement *s, int *skip, int prefix_only, int logical)
{
    size_t n, i, j, start, level;
    int quoted; mf_octet c; enum mf_status status;
    *skip = 0; s->origin = r->origin;
    s->label.data = s->operation.data = s->operand.data = b;
    s->label.length = s->operation.length = s->operand.length = 0;
    if (r->bytes.length && !r->bytes.data) return MF_SOURCE;
    if (!logical && r->bytes.length > 80) return MF_LIMIT;
    if (!r->bytes.length) { *skip = 1; return MF_OK; }
    status = decode(r->bytes.data[0], e, &c); if (status != MF_OK) return status;
    if (c == 0x2a) { *skip = 1; return MF_OK; }
    if (r->bytes.length > 1 && c == 0x2e) {
        status = decode(r->bytes.data[1], e, &c); if (status != MF_OK) return status;
        if (c == 0x2a) { *skip = 1; return MF_OK; }
    }
    n = logical ? r->bytes.length : (r->bytes.length < 71 ? r->bytes.length : 71);
    if (n > cap) return MF_LIMIT;
    for (i = 0; i < n; ++i) {
        status = decode(r->bytes.data[i], e, b + i);
        if (status != MF_OK) { s->origin.column = (unsigned)i + 1; return status; }
        if (b[i] < 0x20 || b[i] > 0x7e) {
            s->origin.column = (unsigned)i + 1; return MF_SOURCE;
        }
    }
    i = 0;
    if (b[0] != 0x20) {
        start = i;
        while (i < n && b[i] != 0x20) ++i;
        s->label.data = b + start; s->label.length = i - start;
    }
    while (i < n && b[i] == 0x20) ++i;
    if (i == n) {
        if (s->label.length) return MF_SOURCE;
        *skip = 1; return MF_OK;
    }
    start = i;
    while (i < n && b[i] != 0x20) ++i;
    s->operation.data = b + start; s->operation.length = i - start;
    if (prefix_only) return MF_OK;
    if (!logical && r->bytes.length >= 72) {
        status = decode(r->bytes.data[71], e, &c); if (status != MF_OK) return status;
        if (c != 0x20) { s->origin.column = 72; return MF_UNSUPPORTED; }
    }
    while (i < n && b[i] == 0x20) ++i;
    start = i; level = 0; quoted = 0;
    while (i < n) {
        c = b[i];
        if (!quoted && i + 1 < n && b[i + 1] == 0x27 &&
            ((mf_macro_upper(c) == 0x4b ||
             mf_macro_upper(c) == 0x4e || mf_macro_upper(c) == 0x54) &&
             (i + 2 == n || b[i + 2] != 0x26))) {
            s->origin.column = (unsigned)i + 1; return MF_UNSUPPORTED;
        }
        /* D/S/I/O can also begin nominal/self-defining syntax. Preserve a
         * paired nominal quote; a bare attribute reference is unsupported. */
        if (!quoted && i + 1 < n && b[i + 1] == 0x27 &&
            (mf_macro_upper(c) == 0x44 || mf_macro_upper(c) == 0x53 ||
             mf_macro_upper(c) == 0x49 || mf_macro_upper(c) == 0x4f)) {
            j = i + 2;
            while (j < n && b[j] != 0x27) ++j;
            if (j == n || b[j] != 0x27) {
                s->origin.column = (unsigned)i + 1; return MF_UNSUPPORTED;
            }
        }
        if (!quoted && (mf_macro_upper(c) == 0x54 || mf_macro_upper(c) == 0x4b || mf_macro_upper(c) == 0x4e) && i + 2 < n &&
            b[i + 1] == 0x27 && b[i + 2] == 0x26) { i += 2; continue; }
        if (c == 0x27 && (quoted || !i || mf_macro_upper(b[i-1]) != 0x4c)) quoted = !quoted;
        else if (!quoted && c == 0x28) ++level;
        else if (!quoted && c == 0x29) { if (!level) return MF_SOURCE; --level; }
        if (!quoted && !level && c == 0x20) break;
        ++i;
    }
    if (quoted || level) return MF_SOURCE;
    s->operand.data = b + start; s->operand.length = i - start;
    return MF_OK;
}
/* The maximal size_t cursor marks exhaustion without length+1 arithmetic. */
enum mf_status mf_macro_arguments(struct mf_span s, size_t *cursor,
    struct mf_span *out)
{
    size_t i, start, level, end; int quoted;
    if (*cursor == (size_t)-1) return MF_EOF;
    start = i = *cursor; level = 0; quoted = 0;
    while (i < s.length) {
        if (s.data[i] == 0x27 && (quoted || !i || mf_macro_upper(s.data[i-1]) != 0x4c)) quoted = !quoted;
        else if (!quoted && s.data[i] == 0x28) ++level;
        else if (!quoted && s.data[i] == 0x29) { if (!level) return MF_SOURCE; --level; }
        else if (!quoted && !level && s.data[i] == 0x2c) break;
        ++i;
    }
    if (quoted || level) return MF_SOURCE;
    end = i;
    while (start < end && s.data[start] == 0x20) ++start;
    while (end > start && s.data[end - 1] == 0x20) --end;
    out->data = s.data ? s.data + start : NULL; out->length = end - start;
    *cursor = i < s.length ? i + 1 : (size_t)-1;
    return MF_OK;
}
