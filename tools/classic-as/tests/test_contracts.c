/* SPDX-License-Identifier: MIT
 * Original independent engine contract controls, host-only C89 QA.
 */
#include "mf_classic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct line { const char *label, *operation, *operand; };
struct arena { void *blocks[128]; size_t count; };
struct source {
    const struct line *first, *second;
    size_t count, second_count, index;
    int pass, fail_pass;
    size_t fail_index;
    enum mf_status replay_status;
};
struct observer {
    int begin, text, gap, fixup, entry, finish, valid, reports;
    enum mf_status failure;
    int fail_event;
    unsigned source, column;
    unsigned long line;
    size_t detail_length, sections, symbols;
    mf_u32 lengths[8];
    int dummy[8];
    mf_octet image[256];
    size_t size;
    struct mf_fixup last_fixup;
};
static unsigned checks, failures;
static void check(int condition, const char *name)
{
    ++checks;
    if (!condition) { ++failures; fprintf(stderr, "FAIL: %s\n", name); }
}
static struct mf_span span(const char *s)
{
    struct mf_span p;
    p.data = (const mf_octet *)s; p.length = strlen(s); return p;
}
static void *allocate(void *cookie, size_t n)
{
    struct arena *a; void *p;
    a = (struct arena *)cookie;
    if (a->count == 128) return NULL;
    p = malloc(n ? n : 1);
    if (p) a->blocks[a->count++] = p;
    return p;
}
static void release(struct arena *a)
{ size_t i; for (i = 0; i < a->count; ++i) free(a->blocks[i]); }
static enum mf_status next(void *cookie, struct mf_statement *s)
{
    struct source *p; const struct line *line; size_t count;
    p = (struct source *)cookie;
    s->origin.source = 77; s->origin.line = 100 + (unsigned long)p->index;
    s->origin.column = 9;
    if (p->pass == p->fail_pass && p->index == p->fail_index) {
        /* On provider failure spans need not be readable or initialized. */
        s->label.data = s->operation.data = s->operand.data = (const mf_octet *)1;
        s->label.length = s->operation.length = s->operand.length = 1000;
        return MF_IO;
    }
    count = p->pass == 2 ? p->second_count : p->count;
    if (p->index == count) return MF_EOF;
    line = (p->pass == 2 ? p->second : p->first) + p->index++;
    s->label = span(line->label); s->operation = span(line->operation);
    s->operand = span(line->operand); return MF_OK;
}
static enum mf_status replay(void *cookie)
{
    struct source *s;
    s = (struct source *)cookie; s->pass = 2; s->index = 0;
    return s->replay_status;
}
static enum mf_status begin(void *cookie, const struct mf_section *s, size_t n,
    const struct mf_symbol *symbols, size_t m)
{
    struct observer *o; size_t i;
    o = (struct observer *)cookie; ++o->begin; o->sections = n; o->symbols = m;
    for (i = 0; i < n && i < 8; ++i) { o->lengths[i] = s[i].length; o->dummy[i] = s[i].dummy; }
    (void)symbols;
    return o->fail_event == 1 ? o->failure : MF_OK;
}
static enum mf_status text(void *cookie, unsigned section, mf_u32 offset,
    const mf_octet *bytes, size_t n)
{
    struct observer *o;
    o = (struct observer *)cookie; ++o->text;
    if (offset <= sizeof o->image && n <= sizeof o->image - (size_t)offset) {
        memcpy(o->image + (size_t)offset, bytes, n);
        if ((size_t)offset + n > o->size) o->size = (size_t)offset + n;
    }
    (void)section;
    return o->fail_event == 2 ? o->failure : MF_OK;
}
static enum mf_status gap(void *cookie, unsigned section, mf_u32 offset, mf_u32 n)
{
    struct observer *o; o = (struct observer *)cookie; ++o->gap;
    (void)section; (void)offset; (void)n;
    return o->fail_event == 3 ? o->failure : MF_OK;
}
static enum mf_status fixup(void *cookie, const struct mf_fixup *f)
{
    struct observer *o; o = (struct observer *)cookie; ++o->fixup; o->last_fixup = *f;
    return o->fail_event == 4 ? o->failure : MF_OK;
}
static enum mf_status entry(void *cookie, const struct mf_entry *e)
{
    struct observer *o; o = (struct observer *)cookie; ++o->entry; (void)e;
    return o->fail_event == 5 ? o->failure : MF_OK;
}
static enum mf_status finish(void *cookie, int valid)
{
    struct observer *o; o = (struct observer *)cookie; ++o->finish; o->valid = valid;
    return o->fail_event == 6 ? o->failure : MF_OK;
}
static void diagnostic(void *cookie, const struct mf_diagnostic *d)
{
    struct observer *o; o = (struct observer *)cookie; ++o->reports;
    o->source = d->origin.source; o->line = d->origin.line;
    o->column = d->origin.column; o->detail_length = d->detail.length;
}
static struct mf_as_config configuration(void)
{
    struct mf_as_config c;
    c.profile = MF_S360; c.max_sections = 8; c.max_symbols = 64;
    c.max_literals = 0;
    c.max_fixups = 64; c.max_statement = 256; c.max_expression_depth = 32;
    return c;
}
static struct source input(const struct line *lines, size_t n)
{
    struct source s; memset(&s, 0, sizeof s);
    s.first = s.second = lines; s.count = s.second_count = n;
    s.pass = 1; s.fail_pass = -1; s.replay_status = MF_OK; return s;
}
static enum mf_status run(struct source *s, struct observer *o,
    struct mf_as_config *c, struct mf_as_result *result)
{
    struct arena arena; struct mf_storage storage; struct mf_as *as;
    struct mf_statements source; struct mf_object_writer writer;
    struct mf_diagnostics diagnostics; enum mf_status status;
    memset(&arena, 0, sizeof arena);
    storage.cookie = &arena; storage.acquire = allocate;
    source.cookie = s; source.next = next; source.replay = replay;
    writer.cookie = o; writer.begin = begin; writer.text = text;
    writer.gap = gap; writer.fixup = fixup; writer.entry = entry; writer.finish = finish; writer.deck_id = NULL;
    diagnostics.cookie = o; diagnostics.report = diagnostic;
    status = mf_as_create(c, &storage, &as);
    if (status == MF_OK) status = mf_as_assemble(as, &source, &writer, &diagnostics, result);
    mf_as_destroy(as); release(&arena); return status;
}
#define N(a) (sizeof(a) / sizeof((a)[0]))
static void layouts(void)
{
    static const struct line empty[] = {
        {"A","CSECT",""}, {"B","CSECT",""}, {"A","CSECT",""}, {"","END",""}
    };
    static const struct line zero_ds[] = {
        {"A","CSECT",""}, {"","DS","0F"}, {"B","CSECT",""},
        {"A","CSECT",""}, {"","DC","X'01'"}, {"","END",""}
    };
    static const struct line dummy[] = {
        {"LONG_DUMMY_SECTION_NAME","DSECT",""}, {"LONG_INTERNAL_FIELD_NAME","DS","F"},
        {"TAIL","DS","H"}, {"LEN","EQU","TAIL-LONG_INTERNAL_FIELD_NAME"},
        {"A","CSECT",""}, {"","DC","F'LEN'"}, {"","END",""}
    };
    struct mf_as_config c; struct mf_as_result r; struct source s; struct observer o;
    c = configuration(); s = input(empty, N(empty)); memset(&o, 0, sizeof o);
    check(run(&s, &o, &c, &r) == MF_OK, "return to empty section");
    check(o.begin == 1 && o.finish == 1 && o.valid, "empty section lifecycle");
    s = input(zero_ds, N(zero_ds)); memset(&o, 0, sizeof o);
    check(run(&s, &o, &c, &r) == MF_OK, "return after zero DS");
    check(o.lengths[0] == 1 && o.lengths[1] == 0 && o.gap == 0, "zero DS has no gap");
    s = input(dummy, N(dummy)); memset(&o, 0, sizeof o);
    check(run(&s, &o, &c, &r) == MF_OK, "long dummy and internal labels");
    check(o.dummy[0] && o.lengths[0] == 6 && o.text == 1 && o.size == 4 &&
        o.image[0] == 0 && o.image[1] == 0 && o.image[2] == 0 && o.image[3] == 4,
        "DSECT layout contributes expression without output");
}
static void constants(void)
{
    static const struct line limits[] = {
        {"A","CSECT",""}, {"","DC","F'-2147483648',F'2147483647'"},
        {"","DC","AD(-9223372036854775808),AD(18446744073709551615)"},
        {"","END",""}
    };
    static const mf_octet expected[] = {
        0x80,0,0,0,0x7f,0xff,0xff,0xff,0x80,0,0,0,0,0,0,0,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
    };
    static const struct line forward_bad[] = {
        {"A","CSECT",""}, {"","DC","F'LATER'"},
        {"LATER","EQU","2147483648"}, {"","END",""}
    };
    static const struct line reloc[] = {
        {"A","CSECT",""}, {"B","DS","F"}, {"","DC","A(-B+4)"}, {"","END",""}
    };
    static const struct line expression[] = {
        {"A","CSECT",""}, {"B","DS","F"},
        {"","DC","F'(B-A)+(+3)-(-5)'"}, {"","END",""}
    };
    struct mf_as_config c; struct mf_as_result r; struct source s; struct observer o;
    c = configuration(); s = input(limits, N(limits)); memset(&o, 0, sizeof o);
    check(run(&s, &o, &c, &r) == MF_OK, "signed F and two-part AD bounds");
    check(o.size == N(expected) && !memcmp(o.image, expected, N(expected)), "independent signed F and AD octets");
    s = input(forward_bad, N(forward_bad)); memset(&o, 0, sizeof o);
    check(run(&s, &o, &c, &r) == MF_RANGE, "forward F positive overflow rejected");
    check(o.begin == 1 && o.text == 0 && o.finish == 1 && !o.valid && !r.valid_output,
        "forward width failure invalidates begun writer");
    check(o.source == 77 && o.line == 101 && o.column == 9, "forward width error source coordinate");
    s = input(reloc, N(reloc)); memset(&o, 0, sizeof o);
    check(run(&s, &o, &c, &r) == MF_OK, "negative relocation expression");
    check(o.fixup == 1 && o.last_fixup.subtract && o.last_fixup.target == 1 &&
        o.last_fixup.width == 4 && o.last_fixup.offset == 4 && o.image[7] == 4,
        "negative A relocation carries addend and separate sign");
    s = input(expression, N(expression)); memset(&o, 0, sizeof o);
    check(run(&s, &o, &c, &r) == MF_OK && o.image[7] == 8 && o.fixup == 0,
        "same-section subtraction and unary parenthesized constants");
}
static void capacities(void)
{
    static const struct line sections[] = {
        {"A","CSECT",""}, {"B","CSECT",""}, {"","END",""}
    };
    static const struct line symbols[] = {
        {"A","CSECT",""}, {"B","EQU","1"}, {"","END",""}
    };
    static const struct line statements[] = {
        {"A","CSECT",""}, {"","DC","CL32'X'"}, {"","END",""}
    };
    static const struct line depth[] = {
        {"A","CSECT",""}, {"","DC","F'((((1))))'"}, {"","END",""}
    };
    struct mf_as_config c; struct mf_as_result r; struct source s; struct observer o;
    c = configuration(); c.max_sections = 1; s = input(sections, N(sections)); memset(&o, 0, sizeof o);
    check(run(&s, &o, &c, &r) == MF_LIMIT && !o.begin, "section capacity no writer begin");
    c = configuration(); c.max_symbols = 1; s = input(symbols, N(symbols)); memset(&o, 0, sizeof o);
    check(run(&s, &o, &c, &r) == MF_LIMIT && !o.begin, "symbol capacity no writer begin");
    c = configuration(); c.max_statement = 8; s = input(statements, N(statements)); memset(&o, 0, sizeof o);
    check(run(&s, &o, &c, &r) == MF_LIMIT && !o.begin, "total statement field capacity");
    c = configuration(); c.max_expression_depth = 3; s = input(depth, N(depth)); memset(&o, 0, sizeof o);
    check(run(&s, &o, &c, &r) == MF_LIMIT && !o.begin, "nested expression capacity");
}
static void storage_growth(void)
{
    struct line lines[102]; struct mf_as_config c;
    struct mf_as_result short_result, long_result;
    struct source s; struct observer o; size_t i;
    c = configuration();
    lines[0].label = "A"; lines[0].operation = "CSECT"; lines[0].operand = "";
    lines[1].label = ""; lines[1].operation = "DC"; lines[1].operand = "X'01'";
    lines[2].label = ""; lines[2].operation = "END"; lines[2].operand = "";
    s = input(lines, 3); memset(&o, 0, sizeof o);
    check(run(&s, &o, &c, &short_result) == MF_OK, "short storage control");
    for (i = 2; i < 101; ++i) lines[i] = lines[1];
    lines[101].label = ""; lines[101].operation = "END"; lines[101].operand = "";
    s = input(lines, N(lines)); memset(&o, 0, sizeof o);
    check(run(&s, &o, &c, &long_result) == MF_OK && o.text == 100 &&
        long_result.statements == 102, "repeated unlabeled statements control");
    check(long_result.storage_requested == short_result.storage_requested,
        "session storage independent of unlabeled statement count");
}
static void address_expressions(void)
{
    static const struct line simple[] = {
        {"A","CSECT",""}, {"","L","1,(4+1)"}, {"","END",""}
    };
    static const struct line based[] = {
        {"A","CSECT",""}, {"","USING","*,12"},
        {"","L","1,(TARGET+4)(,12)"}, {"TARGET","DS","F"}, {"","END",""}
    };
    struct mf_as_config c; struct mf_as_result r; struct source s; struct observer o;
    c = configuration(); s = input(simple, N(simple)); memset(&o, 0, sizeof o);
    check(run(&s, &o, &c, &r) == MF_OK && o.size == 4 &&
        o.image[0] == 0x58 && o.image[1] == 0x10 && o.image[2] == 0 && o.image[3] == 5,
        "parenthesized absolute displacement expression");
    s = input(based, N(based)); memset(&o, 0, sizeof o);
    check(run(&s, &o, &c, &r) == MF_OK && o.size == 4 &&
        o.image[0] == 0x58 && o.image[1] == 0x10 && o.image[2] == 0xc0 && o.image[3] == 8,
        "parenthesized forward symbolic displacement with explicit base");
}
static void callbacks(void)
{
    static const struct line good[] = {
        {"A","CSECT",""}, {"","DC","A(A)"}, {"","DS","F"}, {"","END",""}
    };
    static const struct line changed[] = {
        {"A","CSECT",""}, {"","DC","A(A+1)"}, {"","DS","F"}, {"","END",""}
    };
    struct mf_as_config c; struct mf_as_result r; struct source s;
    struct observer o; enum mf_status status; int event, pass;
    c = configuration();
    for (pass = 1; pass <= 2; ++pass) {
        s = input(good, N(good)); s.fail_pass = pass; s.fail_index = 1;
        memset(&o, 0, sizeof o); status = run(&s, &o, &c, &r);
        check(status == MF_IO && r.status == status && !r.valid_output,
            "provider failure status");
        check(o.source == 77 && o.line == 101 && o.column == 9 && o.detail_length == 0,
            "provider failure uses origin only");
        check(o.begin == pass - 1 && o.finish == pass - 1 && !o.valid,
            "provider failure lifecycle by pass");
    }
    s = input(good, N(good)); s.replay_status = MF_IO; memset(&o, 0, sizeof o);
    check(run(&s, &o, &c, &r) == MF_IO && !o.begin && !o.finish, "replay I/O fails before writer begins");
    for (event = 1; event <= 6; ++event) {
        s = input(good, N(good)); memset(&o, 0, sizeof o);
        o.fail_event = event; o.failure = MF_IO;
        status = run(&s, &o, &c, &r);
        check(status == MF_IO && r.status == status && !r.valid_output, "writer failure result");
        check(o.begin == 1 && o.finish == 1 && o.valid == (event == 6), "writer failure completion lifecycle");
    }
    s = input(good, N(good)); s.second = changed; memset(&o, 0, sizeof o);
    check(run(&s, &o, &c, &r) == MF_REPLAY && o.finish == 1 && !o.valid && !r.valid_output,
        "changed replay never valid output");
    s = input(good, N(good)); c.max_fixups = 0; memset(&o, 0, sizeof o);
    check(run(&s, &o, &c, &r) == MF_LIMIT && !o.begin && !o.finish, "fixup capacity before emission");
    memset(&r, 0xff, sizeof r);
    status = mf_as_assemble(NULL, NULL, NULL, NULL, &r);
    check(status == MF_SOURCE && r.status == MF_SOURCE && !r.valid_output,
        "invalid API arguments result agrees with return");
}
int main(void)
{
    layouts(); constants(); capacities(); storage_growth(); address_expressions(); callbacks();
    printf("%u independent contract checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
