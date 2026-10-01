/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Original fixtures, numeric expected fields, bounded provider contracts.
 * No upstream macro definition or generated expansion is imported.
 */
#include "mf_classic_macro.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "macro:%d: %s\n", __LINE__, #x); exit(1); } } while (0)
static unsigned long checks;
struct fixture {
    void *blocks[64]; size_t block_count, calls, used, fail_allocate;
    struct mf_macro_config config; struct mf_macro *macro;
    struct mf_storage storage; struct mf_records records;
    struct mf_statements statements; struct mf_macro_observer observer;
    const char *first, *second, *source, *repeat;
    size_t at; unsigned long line, repetitions, repeated, next_calls;
    int pass, fail_replay, fail_pass, change_origin;
    unsigned long fail_next, observed, errors; size_t last_depth;
    struct mf_origin observed_origin, model_origin, definition_origin, nested_origin;
    mf_octet raw[512];
};
static mf_octet ascii(char c)
{
    static const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    size_t i;
    for (i = 0; i < 26; ++i) if (c == letters[i]) return (mf_octet)(0x41 + i);
    for (i = 26; i < 52; ++i) if (c == letters[i]) return (mf_octet)(0x61 + i - 26);
    for (i = 52; i < 62; ++i) if (c == letters[i]) return (mf_octet)(0x30 + i - 52);
    switch (c) {
    case ' ': return 0x20; case '&': return 0x26; case '\'': return 0x27;
    case '(': return 0x28; case ')': return 0x29; case '*': return 0x2a;
    case '+': return 0x2b; case ',': return 0x2c; case '-': return 0x2d;
    case '.': return 0x2e; case '/': return 0x2f; case '=': return 0x3d;
    case '@': return 0x40; case '_': return 0x5f; case '$': return 0x24;
    case '#': return 0x23; case '\t': return 0x09;
    default: CHECK(0); return 0;
    }
}
static int matches(struct mf_span s, const char *text)
{
    size_t i, n; n = strlen(text);
    if (s.length != n) return 0;
    for (i = 0; i < n; ++i) if (s.data[i] != ascii(text[i])) return 0;
    return 1;
}
static void *acquire(void *cookie, size_t n)
{
    struct fixture *f; void *p;
    f = (struct fixture *)cookie; ++f->calls;
    if (f->calls == f->fail_allocate) return NULL;
    CHECK(n != 0 && f->block_count < 64); p = malloc(n); CHECK(p != NULL);
    f->blocks[f->block_count++] = p; CHECK(n <= (size_t)-1 - f->used); f->used += n; return p;
}
static enum mf_status record_next(void *cookie, struct mf_record *out)
{
    struct fixture *f; size_t n; const char *s; enum mf_status st;
    f = (struct fixture *)cookie; ++f->next_calls;
    if (f->pass == f->fail_pass && f->next_calls == f->fail_next) {
        out->origin.source = 99; out->origin.line = 999; out->origin.column = 17;
        out->bytes.data = NULL; out->bytes.length = (size_t)-1; return MF_IO;
    }
    n = 0; s = f->source;
    if (!s[f->at]) {
        if (f->repeated == f->repetitions) return MF_EOF;
        ++f->repeated; s = f->repeat;
        while (*s) { CHECK(n < sizeof f->raw); f->raw[n++] = ascii(*s++); }
    } else {
        while (s[f->at] && s[f->at] != '\n') {
            CHECK(n < sizeof f->raw); f->raw[n++] = ascii(s[f->at++]);
        }
        if (s[f->at]) ++f->at;
    }
    if (f->records.encoding == MF_CP037) {
        size_t i;
        for (i = 0; i < n; ++i) { st = mf_ascii_to_ebcdic(f->raw[i], f->raw + i); CHECK(st == MF_OK); }
    }
    ++f->line; out->origin.source = 7; out->origin.line = f->line;
    out->origin.column = f->pass == 2 && f->change_origin ? 2 : 1;
    out->bytes.data = f->raw; out->bytes.length = n; return MF_OK;
}
static enum mf_status replay(void *cookie)
{
    struct fixture *f; f = (struct fixture *)cookie;
    if (f->fail_replay) return MF_IO;
    f->source = f->second ? f->second : f->first; f->pass = 2;
    f->at = 0; f->line = f->repeated = f->next_calls = 0; return MF_OK;
}
static void observer(void *cookie, const struct mf_macro_event *event)
{
    struct fixture *f; f = (struct fixture *)cookie;
    CHECK(event->depth <= f->config.max_depth);
    f->last_depth = event->depth; f->observed_origin = event->origin;
    if (event->status != MF_OK) ++f->errors;
    else { ++f->observed; CHECK(event->detail.length != 0); }
    if (event->depth) {
        CHECK(event->frames != NULL && event->frames[0].name.length != 0);
        f->definition_origin = event->frames[0].definition;
        f->model_origin = event->frames[event->depth - 1].model;
        f->nested_origin = event->frames[event->depth - 1].invocation;
        CHECK(event->origin.line == event->frames[0].invocation.line);
    } else CHECK(event->frames == NULL);
}
static void init(struct fixture *f, const char *source)
{
    memset(f, 0, sizeof *f); f->first = f->source = source; f->pass = 1;
    f->config.max_macros = 8; f->config.max_parameters = 8;
    f->config.max_model_statements = 64; f->config.max_definition_bytes = 4096;
    f->config.max_depth = 4; f->config.max_argument_bytes = 256;
    f->config.max_statement_bytes = 256; f->config.max_steps = 1000000UL;
    f->storage.cookie = f; f->storage.acquire = acquire;
    f->records.cookie = f; f->records.encoding = MF_ASCII;
    f->records.next = record_next; f->records.replay = replay;
    f->observer.cookie = f; f->observer.notify = observer;
}
static enum mf_status create(struct fixture *f)
{
    return mf_macro_create(&f->config, &f->storage, &f->records,
        &f->observer, &f->macro, &f->statements);
}
static void clean(struct fixture *f)
{
    size_t i; mf_macro_destroy(f->macro);
    for (i = 0; i < f->block_count; ++i) free(f->blocks[i]);
}
static enum mf_status drain(struct fixture *f, unsigned long *n)
{
    struct mf_statement s; enum mf_status st; *n = 0;
    while ((st = f->statements.next(f->statements.cookie, &s)) == MF_OK) ++*n;
    CHECK(s.label.length == 0 && s.operation.length == 0 && s.operand.length == 0);
    return st;
}
static void expected(struct fixture *f, const char *label, const char *operation,
    const char *operand, unsigned long line)
{
    struct mf_statement s; enum mf_status st;
    st = f->statements.next(f->statements.cookie, &s);
    if (st != MF_OK) fprintf(stderr, "expected [%s] [%s] [%s] at %lu, got status %d origin %lu\n",
        label, operation, operand, line, (int)st, s.origin.line);
    CHECK(st == MF_OK);
    if (!matches(s.label, label) || !matches(s.operation, operation) || !matches(s.operand, operand))
        fprintf(stderr, "expected [%s] [%s] [%s], got [%.*s] [%.*s] [%.*s]\n",
            label, operation, operand, (int)s.label.length, s.label.data,
            (int)s.operation.length, s.operation.data, (int)s.operand.length, s.operand.data);
    CHECK(matches(s.label, label) && matches(s.operation, operation) && matches(s.operand, operand));
    CHECK(s.origin.source == 7 && s.origin.line == line && s.origin.column == 1);
}
static const char simple[] =
    " MACRO\n&N XLOAD &R,&A\n&N L &R,&A\n L &R,0(,&R)\n.MEND MEND ,\n"
    " MACRO\n&N XSTORE &R,&A,&S=14\n&N L &S,&A\n ST &R,0(,&S)\n MEND\n"
    "HERE XLOAD 4,16(1)\n XSTORE 6,24(1)\n XSTORE 6,28(1),S=7\n";
static void templates(void)
{
    struct fixture f; unsigned pass; unsigned long n; size_t calls, used;
    init(&f, simple); CHECK(create(&f) == MF_OK); calls = f.calls; used = f.used;
    CHECK(f.statements.replay(f.statements.cookie) == MF_SOURCE);
    for (pass = 0; pass < 2; ++pass) {
        expected(&f, "HERE", "L", "4,16(1)", 11);
        CHECK(f.last_depth == 1 && f.definition_origin.line == 2 && f.model_origin.line == 3);
        expected(&f, "", "L", "4,0(,4)", 11);
        expected(&f, "", "L", "14,24(1)", 12);
        expected(&f, "", "ST", "6,0(,14)", 12);
        expected(&f, "", "L", "7,28(1)", 13);
        expected(&f, "", "ST", "6,0(,7)", 13);
        CHECK(drain(&f, &n) == MF_EOF && n == 0);
        CHECK(f.calls == calls && f.used == used);
        if (!pass) CHECK(f.statements.replay(f.statements.cookie) == MF_OK);
    }
    CHECK(f.observed == 12 && !f.errors); clean(&f);
}
static void binding(void)
{
    struct fixture f; unsigned long n;
    init(&f, " MACRO\n&N INNER &A,&B=C'yes'\n&N DC &A,&B\n MEND\n MACRO\n&N OUTER &P,&Q=C'no'\n&N INNER &P,B=&Q\n MEND\nLBL OUTER (A,(B,C)),Q=C'a b,('\n OUTER C'q''q',Q=\n");
    CHECK(create(&f) == MF_OK);
    expected(&f, "LBL", "DC", "(A,(B,C)),C'a b,('", 9);
    CHECK(f.last_depth == 2 && f.model_origin.line == 3 && f.nested_origin.line == 7);
    expected(&f, "", "DC", "C'q''q',", 10);
    CHECK(drain(&f, &n) == MF_EOF); clean(&f);
    init(&f, " MACRO\n&N MAKE &P=ABC,&V=F'7'\n&N&P.SUF DC &V\n DC C'&P..'\n MEND\nQ MAKE P=Z\nR MAKE P=,V=\n");
    CHECK(create(&f) == MF_OK);
    expected(&f, "QZSUF", "DC", "F'7'", 6); expected(&f, "", "DC", "C'Z.'", 6);
    expected(&f, "RSUF", "DC", "", 7); expected(&f, "", "DC", "C'.'", 7);
    CHECK(drain(&f, &n) == MF_EOF); clean(&f);
    init(&f, " MACRO\n NULL &A,&B\n DC &A,&B\n MEND\n NULL\n NULL ,\n");
    CHECK(create(&f) == MF_OK); expected(&f, "", "DC", ",", 5);
    expected(&f, "", "DC", ",", 6); CHECK(drain(&f, &n) == MF_EOF); clean(&f);
    init(&f, " MACRO\n&N MIX &OP,&R=1\n&N &OP &R,2\n MEND\nx mix lr,r=3\n");
    CHECK(create(&f) == MF_OK); expected(&f, "x", "lr", "3,2", 5);
    CHECK(drain(&f, &n) == MF_EOF); clean(&f);
    init(&f, " MACRO\n TEST &A,&B=F'0'\n DC &A,&B\n MEND\n TEST C'x=y,z',B=(Q=R, (S,T))\n");
    CHECK(create(&f) == MF_OK);
    expected(&f, "", "DC", "C'x=y,z',(Q=R, (S,T))", 5);
    CHECK(drain(&f, &n) == MF_EOF); clean(&f);
    init(&f, "FIELD DS D'0'\n"); CHECK(create(&f) == MF_OK);
    expected(&f, "FIELD", "DS", "D'0'", 1); CHECK(drain(&f, &n) == MF_EOF); clean(&f);
    init(&f, " MACRO\n Q &V\n DC &V\n MEND\n Q D'A,B'\n Q D'A B'\n Q D'A)B'\n");
    CHECK(create(&f) == MF_OK);
    expected(&f, "", "DC", "D'A,B'", 5); expected(&f, "", "DC", "D'A B'", 6);
    expected(&f, "", "DC", "D'A)B'", 7); CHECK(drain(&f, &n) == MF_EOF); clean(&f);
}
static void bad_source(const char *source, enum mf_status wanted)
{
    struct fixture f; enum mf_status st; unsigned long n;
    init(&f, source); CHECK(create(&f) == MF_OK); st = drain(&f, &n);
    if (st != wanted) fprintf(stderr, "wanted %d got %d source %s\n", wanted, st, source);
    CHECK(st == wanted && f.errors == 1);
    CHECK(drain(&f, &n) == wanted && f.errors == 1); clean(&f);
}
static void failures(void)
{
    static const char *bad[] = {
        " COPY MEMBER\n",
        " LCLA &A(2)\n", " MEXIT\n", " MNOTE 0,'x'\n",
        " MACRO\n X\n MACRO\n MEND\n", " MACRO\n X\n COPY Y\n MEND\n",
        " MACRO\n X &A\n DC C'&&'\n MEND\n X Z\n"
    };
    size_t i;
    for (i = 0; i < sizeof bad / sizeof bad[0]; ++i) bad_source(bad[i], MF_UNSUPPORTED);
    bad_source(" MACRO\n", MF_SOURCE);
    bad_source(" MACRO\n X\n LR 1,2\n", MF_SOURCE);
    bad_source(" MACRO\n X &A,&A\n MEND\n", MF_DUPLICATE);
    bad_source(" MACRO\n X\n MEND\n MACRO\n X\n MEND\n", MF_DUPLICATE);
    bad_source(" MACRO\n X &A=1\n LR &A,2\n MEND\n X A=3,A=4\n", MF_DUPLICATE);
    bad_source(" MACRO\n X &A=1\n LR &A,2\n MEND\n X B=3\n", MF_SOURCE);
    bad_source(" MACRO\n X &A\n LR &A,2\n MEND\n X 1,2\n", MF_UNSUPPORTED);
    bad_source(" MACRO\n X\n LR &NO,2\n MEND\n X\n", MF_UNDEFINED);
    bad_source(" DC C'unclosed\n", MF_SOURCE);
    bad_source(" LR 1,(2\n", MF_SOURCE);
    bad_source(" LR 1,2)\n", MF_SOURCE);
    bad_source(" \tLR 1,2\n", MF_SOURCE);
    bad_source(" MEND\n", MF_UNSUPPORTED);
    bad_source(" MACRO\n X ,\n MEND\n", MF_SOURCE);
    bad_source(" MACRO\n X\n MEND\n X ,\n", MF_UNSUPPORTED);
    bad_source(" MACRO\n X &A\n LR &A,2\n MEND\n X ,\n", MF_UNSUPPORTED);
    bad_source(" DC AL1(L'LABEL)\n", MF_UNSUPPORTED);
    bad_source(" DC N'NAME\n", MF_UNSUPPORTED);
    bad_source(" DC T'NAME\n", MF_UNSUPPORTED);
    bad_source(" DC D'NAME\n", MF_UNSUPPORTED);
    bad_source(" DC S'NAME\n", MF_UNSUPPORTED);
    bad_source(" DC I'NAME\n", MF_UNSUPPORTED);
    bad_source(" DC O'NAME\n", MF_UNSUPPORTED);
}
static void limits(void)
{
    struct fixture f; unsigned long n; size_t i, calls;
    for (i = 1; i <= 16; ++i) {
        init(&f, simple); f.fail_allocate = i; CHECK(create(&f) == MF_LIMIT);
        CHECK(f.macro == NULL && f.statements.next == NULL); clean(&f);
    }
    init(&f, simple); f.config.max_macros = 1; CHECK(create(&f) == MF_OK);
    CHECK(drain(&f, &n) == MF_LIMIT); clean(&f);
    init(&f, simple); f.config.max_parameters = 1; CHECK(create(&f) == MF_OK);
    CHECK(drain(&f, &n) == MF_LIMIT); clean(&f);
    init(&f, simple); f.config.max_model_statements = 1; CHECK(create(&f) == MF_OK);
    CHECK(drain(&f, &n) == MF_LIMIT); clean(&f);
    init(&f, simple); f.config.max_definition_bytes = 4; CHECK(create(&f) == MF_OK);
    CHECK(drain(&f, &n) == MF_LIMIT); clean(&f);
    init(&f, simple); f.config.max_argument_bytes = 2; CHECK(create(&f) == MF_OK);
    CHECK(drain(&f, &n) == MF_LIMIT); clean(&f);
    init(&f, simple); f.config.max_statement_bytes = 2; CHECK(create(&f) == MF_OK);
    CHECK(drain(&f, &n) == MF_LIMIT); clean(&f);
    init(&f, simple); f.config.max_steps = 4; CHECK(create(&f) == MF_OK);
    CHECK(drain(&f, &n) == MF_LIMIT); clean(&f);
    init(&f, " MACRO\n SELF\n SELF\n MEND\n SELF\n");
    CHECK(create(&f) == MF_OK); calls = f.calls;
    CHECK(drain(&f, &n) == MF_LIMIT && f.calls == calls);
    CHECK(f.last_depth == 4 && f.observed_origin.line == 5); clean(&f);
    init(&f, ""); f.config.max_depth = (size_t)-1; CHECK(create(&f) == MF_LIMIT); clean(&f);
    init(&f, ""); f.config.max_macros = (size_t)-1; CHECK(create(&f) == MF_LIMIT); clean(&f);
    init(&f, ""); f.config.max_parameters = (size_t)-1; CHECK(create(&f) == MF_SOURCE); clean(&f);
    init(&f, ""); f.config.max_steps = 0; CHECK(create(&f) == MF_SOURCE); clean(&f);
    init(&f, " MACRO\n Z\n MEND\n Z\n LR 1,2\n"); f.config.max_parameters = 0;
    CHECK(create(&f) == MF_OK); expected(&f, "", "LR", "1,2", 5);
    CHECK(drain(&f, &n) == MF_EOF); clean(&f);
}
static void replay_and_io(void)
{
    struct fixture f; unsigned long n; struct mf_statement s; char card[84], changed[84];
    init(&f, " MACRO\n UNUSED\n LR 1,2\n MEND\n LR 3,4\n");
    f.second = " MACRO\n UNUSED\n LR 1,3\n MEND\n LR 3,4\n";
    CHECK(create(&f) == MF_OK); CHECK(drain(&f, &n) == MF_EOF && n == 1);
    CHECK(f.statements.replay(f.statements.cookie) == MF_OK);
    CHECK(drain(&f, &n) == MF_REPLAY && n == 1); clean(&f);
    init(&f, "*first comment\n LR 1,2\n"); f.second = "*other comment\n LR 1,2\n";
    CHECK(create(&f) == MF_OK); CHECK(drain(&f, &n) == MF_EOF);
    CHECK(f.statements.replay(f.statements.cookie) == MF_OK); CHECK(drain(&f, &n) == MF_REPLAY); clean(&f);
    init(&f, " LR 1,2\n"); f.change_origin = 1;
    CHECK(create(&f) == MF_OK); CHECK(drain(&f, &n) == MF_EOF);
    CHECK(f.statements.replay(f.statements.cookie) == MF_OK); CHECK(drain(&f, &n) == MF_REPLAY); clean(&f);
    init(&f, " LR 1,2\n"); f.second = " LR 1,(2\n";
    CHECK(create(&f) == MF_OK); CHECK(drain(&f, &n) == MF_EOF);
    CHECK(f.statements.replay(f.statements.cookie) == MF_OK); CHECK(drain(&f, &n) == MF_REPLAY); clean(&f);
    init(&f, " LR 1,2\n"); f.fail_pass = 1; f.fail_next = 1;
    CHECK(create(&f) == MF_OK); CHECK(f.statements.next(f.statements.cookie, &s) == MF_IO);
    CHECK(s.origin.line == 999 && s.origin.column == 17 && s.label.data == NULL && s.operation.length == 0); clean(&f);
    init(&f, " LR 1,2\n"); f.fail_replay = 1;
    CHECK(create(&f) == MF_OK); CHECK(drain(&f, &n) == MF_EOF);
    CHECK(f.statements.replay(f.statements.cookie) == MF_IO); clean(&f);
    memset(card, ' ', sizeof card); memcpy(card, " LR 1,2", 7); card[71] = 'X'; card[72] = '\n'; card[73] = 0;
    bad_source(card, MF_UNSUPPORTED);
    card[71] = ' '; card[73] = ' '; card[80] = '\n'; card[81] = 0; card[72] = 'A';
    init(&f, card); f.records.encoding = MF_CP037; CHECK(create(&f) == MF_OK);
    expected(&f, "", "LR", "1,2", 1); CHECK(drain(&f, &n) == MF_EOF); clean(&f);
    memcpy(changed, card, sizeof changed); changed[72] = 'B';
    init(&f, card); f.second = changed; CHECK(create(&f) == MF_OK);
    expected(&f, "", "LR", "1,2", 1); CHECK(drain(&f, &n) == MF_EOF);
    CHECK(f.statements.replay(f.statements.cookie) == MF_OK);
    expected(&f, "", "LR", "1,2", 1); CHECK(drain(&f, &n) == MF_REPLAY); clean(&f);
    card[80] = ' '; card[81] = '\n'; card[82] = 0; bad_source(card, MF_LIMIT);
}
static void constant_storage(void)
{
    struct fixture f; unsigned long n; size_t short_calls, short_used, calls;
    init(&f, " MACRO\n RUN &A,&B\n LR &A,&B\n MEND\n");
    f.repeat = " RUN 1,2"; f.repetitions = 1; CHECK(create(&f) == MF_OK);
    CHECK(drain(&f, &n) == MF_EOF && n == 1); short_calls = f.calls; short_used = f.used; clean(&f);
    init(&f, " MACRO\n RUN &A,&B\n LR &A,&B\n MEND\n");
    f.repeat = " RUN 1,2"; f.repetitions = 30000; CHECK(create(&f) == MF_OK);
    CHECK(drain(&f, &n) == MF_EOF && n == 30000);
    CHECK(f.calls == short_calls && f.used == short_used); calls = f.calls;
    CHECK(f.statements.replay(f.statements.cookie) == MF_OK);
    CHECK(drain(&f, &n) == MF_EOF && n == 30000 && f.calls == calls); clean(&f);
    init(&f, ""); f.repeat = " LR 1,2"; f.repetitions = 50000;
    CHECK(create(&f) == MF_OK); calls = f.calls;
    CHECK(drain(&f, &n) == MF_EOF && n == 50000 && f.calls == calls); clean(&f);
}
int main(void)
{
    templates(); binding(); failures(); limits(); replay_and_io(); constant_storage();
    printf("macro provider: %lu checks passed\n", checks); return 0;
}
