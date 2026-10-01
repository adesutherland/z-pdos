/* SPDX-License-Identifier: MIT
 * Original checked scalar conditional-expression evaluator, numeric ASCII.
 */
#include "macro_internal.h"
#include <string.h>
#define MAXNUM 2147483647L
#define MINNUM (-2147483647L-1L)
struct expression { struct mf_span text; size_t at, depth, limit; };
static void spaces(struct expression *p)
{ while (p->at < p->text.length && p->text.data[p->at] == 0x20) ++p->at; }
static int token(struct expression *p, const mf_octet *word, size_t n)
{
    size_t i; spaces(p);
    if (n > p->text.length - p->at) return 0;
    for (i = 0; i < n; ++i) if (mf_macro_upper(p->text.data[p->at + i]) != word[i]) return 0;
    if (p->at + n < p->text.length && mf_macro_name_rest(p->text.data[p->at + n])) return 0;
    p->at += n; return 1;
}
static enum mf_status logical(struct expression *, struct mf_macro_value *);
static enum mf_status primary(struct expression *p, struct mf_macro_value *v)
{
    enum mf_status st; size_t start; mf_octet c; unsigned long n, maximum; int negative;
    spaces(p); memset(v, 0, sizeof *v);
    if (p->at == p->text.length) return MF_SOURCE;
    c = p->text.data[p->at++];
    if (c == 0x28) {
        if (p->depth == p->limit) return MF_LIMIT;
        ++p->depth; st = logical(p, v); --p->depth;
        spaces(p); if (st != MF_OK) return st;
        if (p->at == p->text.length || p->text.data[p->at++] != 0x29) return MF_SOURCE;
        return MF_OK;
    }
    if (c == 0x27) {
        start = p->at;
        while (p->at < p->text.length) {
            if (p->text.data[p->at] == 0x27) {
                if (p->at + 1 < p->text.length && p->text.data[p->at + 1] == 0x27) { p->at += 2; continue; }
                v->character = 1; v->text.data = p->text.data + start;
                v->text.length = p->at++ - start; return MF_OK;
            }
            ++p->at;
        }
        return MF_SOURCE;
    }
    negative = c == 0x2d;
    if (negative || c == 0x2b) {
        spaces(p);
        if (p->at == p->text.length) return MF_SOURCE;
        c = p->text.data[p->at++];
    }
    if (c < 0x30 || c > 0x39) return MF_UNSUPPORTED;
    maximum = negative ? 2147483648UL : 2147483647UL;
    n = (unsigned long)(c - 0x30);
    while (p->at < p->text.length && p->text.data[p->at] >= 0x30 && p->text.data[p->at] <= 0x39) {
        c = p->text.data[p->at++];
        if (n > (maximum - (unsigned long)(c - 0x30)) / 10UL) return MF_RANGE;
        n = n * 10UL + (unsigned long)(c - 0x30);
    }
    v->number = negative ? (n == 2147483648UL ? MINNUM : -(long)n) : (long)n; return MF_OK;
}
static enum mf_status product(struct expression *p, struct mf_macro_value *v)
{
    struct mf_macro_value r; enum mf_status st; mf_octet op; long a, b;
    st = primary(p, v); if (st != MF_OK) return st;
    for (;;) {
        spaces(p); if (p->at == p->text.length) return MF_OK;
        op = p->text.data[p->at]; if (op != 0x2a && op != 0x2f) return MF_OK;
        ++p->at; st = primary(p, &r); if (st != MF_OK) return st;
        if (v->character || r.character) return MF_SOURCE;
        a = v->number; b = r.number;
        if (op == 0x2f) {
            if (!b || (a == MINNUM && b == -1)) return MF_RANGE;
            v->number = a / b;
        } else {
            if (a && ((a > 0 && ((b > 0 && a > MAXNUM / b) || (b < 0 && b < MINNUM / a))) ||
                (a < 0 && ((b > 0 && a < MINNUM / b) || (b < 0 && a < MAXNUM / b))))) return MF_RANGE;
            v->number = a * b;
        }
    }
}
static enum mf_status sum(struct expression *p, struct mf_macro_value *v)
{
    struct mf_macro_value r; enum mf_status st; mf_octet op; long b;
    st = product(p, v); if (st != MF_OK) return st;
    for (;;) {
        spaces(p); if (p->at == p->text.length) return MF_OK;
        op = p->text.data[p->at]; if (op != 0x2b && op != 0x2d) return MF_OK;
        ++p->at; st = product(p, &r); if (st != MF_OK) return st;
        if (v->character || r.character) return MF_SOURCE;
        b = r.number;
        if (op == 0x2d) { if (b == MINNUM) return MF_RANGE; b = -b; }
        if ((b > 0 && v->number > MAXNUM - b) || (b < 0 && v->number < MINNUM - b)) return MF_RANGE;
        v->number += b;
    }
}
static enum mf_status character_compare(struct mf_span a, struct mf_span b, int *result)
{
    size_t i, j; mf_octet x, y; enum mf_status st;
    i = j = 0; *result = 0;
    while (i < a.length || j < b.length) {
        x = i < a.length ? a.data[i++] : 0x20;
        y = j < b.length ? b.data[j++] : 0x20;
        if (x == 0x27 && i < a.length && a.data[i] == 0x27) ++i;
        if (y == 0x27 && j < b.length && b.data[j] == 0x27) ++j;
        st = mf_ascii_to_ebcdic(x, &x); if (st != MF_OK) return st;
        st = mf_ascii_to_ebcdic(y, &y); if (st != MF_OK) return st;
        if (!*result && x != y) *result = x < y ? -1 : 1;
    }
    return MF_OK;
}
static enum mf_status relation(struct expression *p, struct mf_macro_value *v)
{
    static const mf_octet ops[6][2] = {{0x45,0x51},{0x4e,0x45},{0x4c,0x54},{0x4c,0x45},{0x47,0x54},{0x47,0x45}};
    struct mf_macro_value r; enum mf_status st; size_t i; int cmp;
    st = sum(p, v); if (st != MF_OK) return st;
    for (i = 0; i < 6; ++i) if (token(p, ops[i], 2)) break;
    if (i == 6) return MF_OK;
    st = sum(p, &r); if (st != MF_OK) return st;
    if (v->character != r.character) return MF_SOURCE;
    if (v->character) { st = character_compare(v->text, r.text, &cmp); if (st != MF_OK) return st; }
    else cmp = v->number < r.number ? -1 : v->number > r.number ? 1 : 0;
    v->character = 0;
    v->number = i == 0 ? cmp == 0 : i == 1 ? cmp != 0 : i == 2 ? cmp < 0 :
        i == 3 ? cmp <= 0 : i == 4 ? cmp > 0 : cmp >= 0;
    return MF_OK;
}
static enum mf_status boolean_not(struct expression *p, struct mf_macro_value *v)
{
    static const mf_octet op[] = {0x4e,0x4f,0x54}; enum mf_status st;
    if (token(p, op, 3)) {
        if (p->depth == p->limit) return MF_LIMIT;
        ++p->depth; st = boolean_not(p, v); --p->depth;
        if (st != MF_OK) return st;
        if (v->character || (v->number != 0 && v->number != 1)) return MF_SOURCE;
        v->number = !v->number; return MF_OK;
    }
    return relation(p, v);
}
static enum mf_status boolean_and(struct expression *p, struct mf_macro_value *v)
{
    static const mf_octet op[] = {0x41,0x4e,0x44};
    struct mf_macro_value r; enum mf_status st;
    st = boolean_not(p, v); if (st != MF_OK) return st;
    while (token(p, op, 3)) {
        st = boolean_not(p, &r); if (st != MF_OK) return st;
        if (v->character || r.character || (v->number != 0 && v->number != 1) ||
            (r.number != 0 && r.number != 1)) return MF_SOURCE;
        v->number = v->number && r.number;
    }
    return MF_OK;
}
static enum mf_status logical(struct expression *p, struct mf_macro_value *v)
{
    static const mf_octet op[] = {0x4f,0x52};
    struct mf_macro_value r; enum mf_status st;
    st = boolean_and(p, v); if (st != MF_OK) return st;
    while (token(p, op, 2)) {
        st = boolean_and(p, &r); if (st != MF_OK) return st;
        if (v->character || r.character || (v->number != 0 && v->number != 1) ||
            (r.number != 0 && r.number != 1)) return MF_SOURCE;
        v->number = v->number || r.number;
    }
    return MF_OK;
}
enum mf_status mf_macro_eval(struct mf_span text, size_t depth, struct mf_macro_value *v)
{
    struct expression p; enum mf_status st;
    p.text = text; p.at = p.depth = 0; p.limit = depth;
    st = logical(&p, v); spaces(&p);
    return st != MF_OK ? st : p.at == text.length ? MF_OK : MF_UNSUPPORTED;
}
