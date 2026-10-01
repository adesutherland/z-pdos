/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Original independent bootstrap engine expectations and failure contracts.
 */
#include "mf_classic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "assemble:%d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define U32MAX 0xffffffffUL
static unsigned long checks;
struct fixture {
    void *allocations[256]; size_t allocation_count, allocation_calls, used;
    size_t allocation_fail, budget;
    struct mf_as *as; struct mf_obj *object; struct mf_as_config config;
    struct mf_as_result result; struct mf_storage storage;
    struct mf_records records; struct mf_reader reader;
    struct mf_statements statements, wrapped;
    struct mf_object_writer writer; struct mf_diagnostics diagnostics;
    const char *first, *second, *source; size_t at; unsigned long line;
    mf_octet raw[512], buffer[512]; int pass, replay_fail;
    unsigned long next_calls, fail_next, repeat_count, repeat_at;
    int fail_pass, malformed_span;
    enum mf_status provider_status;
    struct mf_section sections[8]; struct mf_symbol symbols[64];
    size_t section_count, symbol_count; struct mf_fixup fixups[64];
    size_t fixup_count; struct mf_entry entry;
    mf_octet data[8][1024], present[8][1024];
    unsigned long texts, gaps, emitted; unsigned begins, finishes, invalids;
    int valid, discard, fail_begin, fail_text, fail_gap, fail_fixup, fail_entry, fail_finish;
    unsigned reports; struct mf_diagnostic diagnostic; mf_octet detail[256];
    mf_octet deck[8192]; size_t deck_length; unsigned sink_begins, sink_writes, sink_finishes;
    int sink_valid, sink_fail_begin, sink_fail_finish; unsigned sink_fail_write;
};
/* Test source is written in native C text, then explicitly converted. This
 * helper does not confuse the execution character set with canonical ASCII. */
static mf_octet ascii(char c)
{
    static const char native[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    static const mf_octet numeric[] = {
        0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,
        0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,
        0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,
        0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,
        0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39};
    size_t i;
    for (i = 0; i < sizeof numeric; ++i) if (c == native[i]) return numeric[i];
    switch (c) {
    case ' ': return 0x20; case '\'': return 0x27; case '(': return 0x28;
    case ')': return 0x29; case '*': return 0x2a; case '+': return 0x2b;
    case ',': return 0x2c; case '-': return 0x2d; case '/': return 0x2f;
    case '=': return 0x3d; case '&': return 0x26; case '_': return 0x5f;
    case '$': return 0x24; case '#': return 0x23; case '@': return 0x40;
    default: CHECK(0); return 0;
    }
}
static void *acquire(void *cookie, size_t n)
{
    struct fixture *f; void *p;
    f = (struct fixture *)cookie; ++f->allocation_calls;
    if (f->allocation_calls == f->allocation_fail || n > f->budget - f->used) return NULL;
    p = malloc(n ? n : 1); CHECK(p != NULL);
    CHECK(f->allocation_count < sizeof f->allocations / sizeof f->allocations[0]);
    f->allocations[f->allocation_count++] = p; f->used += n; return p;
}
static enum mf_status record_next(void *cookie, struct mf_record *record)
{
    struct fixture *f; size_t n; const char *s;
    f = (struct fixture *)cookie; s = f->source; n = 0;
    if (f->repeat_count) {
        if (f->repeat_at == f->repeat_count + 1) return MF_EOF;
        s = f->repeat_at++ == f->repeat_count ? " END" : " LR 1,2";
        while (*s) f->raw[n++] = ascii(*s++);
    } else {
        if (!s[f->at]) return MF_EOF;
        while (s[f->at] && s[f->at] != '\n') {
            if (n == sizeof f->raw) return MF_LIMIT;
            f->raw[n++] = ascii(s[f->at++]);
        }
        if (s[f->at]) ++f->at;
    }
    ++f->line; record->origin.source = 7; record->origin.line = f->line; record->origin.column = 1;
    record->bytes.data = f->raw; record->bytes.length = n; return MF_OK;
}
static enum mf_status replay(void *cookie)
{
    struct fixture *f; f = (struct fixture *)cookie;
    if (f->replay_fail) return MF_IO;
    f->pass = 2; f->at = 0; f->line = 0; f->next_calls = 0; f->repeat_at = 0;
    f->source = f->second ? f->second : f->first; return MF_OK;
}
static enum mf_status statement_next(void *cookie, struct mf_statement *statement)
{
    struct fixture *f; enum mf_status status;
    f = (struct fixture *)cookie; ++f->next_calls;
    if (f->pass == f->fail_pass && f->next_calls == f->fail_next) {
        /* Error spans deliberately do not describe readable memory. */
        statement->label.data = statement->operation.data = statement->operand.data = NULL;
        statement->label.length = statement->operation.length = statement->operand.length = (size_t)-1;
        statement->origin.source = 93; statement->origin.line = 321; statement->origin.column = 17;
        return f->provider_status;
    }
    status = f->statements.next(f->statements.cookie, statement);
    if (status == MF_OK && f->malformed_span) {
        statement->operation.data = NULL;
        statement->operation.length = f->malformed_span == 2 ? (size_t)-1 : 1;
    }
    return status;
}
static enum mf_status statement_replay(void *cookie)
{
    struct fixture *f; f = (struct fixture *)cookie;
    return f->statements.replay(f->statements.cookie);
}
static enum mf_status begin(void *cookie, const struct mf_section *s, size_t ns,
    const struct mf_symbol *y, size_t ny)
{
    struct fixture *f; f = (struct fixture *)cookie; ++f->begins;
    CHECK(ns <= 8 && ny <= 64);
    memcpy(f->sections, s, ns * sizeof *s); memcpy(f->symbols, y, ny * sizeof *y);
    f->section_count = ns; f->symbol_count = ny;
    return f->fail_begin ? MF_IO : MF_OK;
}
static enum mf_status text(void *cookie, unsigned id, mf_u32 offset,
    const mf_octet *p, size_t n)
{
    struct fixture *f; f = (struct fixture *)cookie; ++f->texts;
    if (f->fail_text) return MF_IO;
    f->emitted += (unsigned long)n;
    if (!f->discard) {
        CHECK(id >= 1 && id <= 8); CHECK(offset < 1024 && n <= 1024 - offset);
        memcpy(f->data[id - 1] + (size_t)offset, p, n);
        memset(f->present[id - 1] + (size_t)offset, 1, n);
    }
    return MF_OK;
}
static enum mf_status gap(void *cookie, unsigned id, mf_u32 offset, mf_u32 n)
{
    struct fixture *f; f = (struct fixture *)cookie; ++f->gaps;
    CHECK(id >= 1 && id <= 8); (void)offset; (void)n;
    return f->fail_gap ? MF_IO : MF_OK;
}
static enum mf_status fixup(void *cookie, const struct mf_fixup *fix)
{
    struct fixture *f; f = (struct fixture *)cookie;
    if (f->fail_fixup) return MF_IO;
    CHECK(f->fixup_count < 64); f->fixups[f->fixup_count++] = *fix; return MF_OK;
}
static enum mf_status entry(void *cookie, const struct mf_entry *e)
{
    struct fixture *f; f = (struct fixture *)cookie;
    f->entry = *e; return f->fail_entry ? MF_IO : MF_OK;
}
static enum mf_status finish(void *cookie, int valid)
{
    struct fixture *f; f = (struct fixture *)cookie; ++f->finishes;
    if (!valid) { ++f->invalids; f->valid = 0; return MF_OK; }
    if (f->fail_finish) return MF_IO;
    f->valid = 1; return MF_OK;
}
static void report(void *cookie, const struct mf_diagnostic *d)
{
    struct fixture *f; f = (struct fixture *)cookie; ++f->reports;
    f->diagnostic = *d; CHECK(d->detail.length <= sizeof f->detail);
    if (d->detail.length) { CHECK(d->detail.data != NULL); memcpy(f->detail, d->detail.data, d->detail.length); }
    f->diagnostic.detail.data = f->detail;
}
static enum mf_status sink_begin(void *cookie)
{
    struct fixture *f; f = (struct fixture *)cookie; ++f->sink_begins;
    return f->sink_fail_begin ? MF_IO : MF_OK;
}
static enum mf_status sink_write(void *cookie, const mf_octet *p, size_t n)
{
    struct fixture *f; f = (struct fixture *)cookie; ++f->sink_writes;
    CHECK(n == 80); CHECK(f->deck_length + n <= sizeof f->deck);
    if (f->sink_writes == f->sink_fail_write) {
        memcpy(f->deck + f->deck_length, p, 7); f->deck_length += 7; return MF_IO;
    }
    memcpy(f->deck + f->deck_length, p, n); f->deck_length += n; return MF_OK;
}
static enum mf_status sink_finish(void *cookie, int valid)
{
    struct fixture *f; f = (struct fixture *)cookie; ++f->sink_finishes;
    f->sink_valid = valid && !f->sink_fail_finish;
    return f->sink_fail_finish && valid ? MF_IO : MF_OK;
}
static void init(struct fixture *f, const char *source)
{
    memset(f, 0, sizeof *f); f->first = f->source = source; f->pass = 1;
    f->budget = (size_t)-1; f->storage.cookie = f; f->storage.acquire = acquire;
    f->config.profile = MF_S360; f->config.max_sections = 8; f->config.max_symbols = 64;
    f->config.max_literals = 0;
    f->config.max_fixups = 64; f->config.max_statement = 256; f->config.max_expression_depth = 32;
    f->records.cookie = f; f->records.encoding = MF_ASCII; f->records.next = record_next; f->records.replay = replay;
    CHECK(mf_reader_init(&f->reader, &f->records, f->buffer, sizeof f->buffer, &f->statements) == MF_OK);
    f->wrapped.cookie = f; f->wrapped.next = statement_next; f->wrapped.replay = statement_replay;
    f->writer.cookie = f; f->writer.begin = begin; f->writer.text = text; f->writer.gap = gap;
    f->writer.fixup = fixup; f->writer.entry = entry; f->writer.finish = finish;
    f->diagnostics.cookie = f; f->diagnostics.report = report;
}
static enum mf_status run(struct fixture *f)
{
    enum mf_status status;
    status = mf_as_create(&f->config, &f->storage, &f->as);
    if (status != MF_OK) return status;
    return mf_as_assemble(f->as, &f->wrapped, &f->writer, &f->diagnostics, &f->result);
}
static void clean(struct fixture *f)
{
    size_t i; mf_as_destroy(f->as); mf_obj_destroy(f->object);
    for (i = 0; i < f->allocation_count; ++i) free(f->allocations[i]);
}
static void successful(struct fixture *f)
{
    CHECK(run(f) == MF_OK); CHECK(f->result.status == MF_OK);
    CHECK(f->result.valid_output && f->valid); CHECK(f->reports == 0);
    CHECK(f->begins == 1 && f->finishes == 1 && f->invalids == 0);
    CHECK(f->result.storage_requested == f->used);
}
static void failed(struct fixture *f, enum mf_status expected)
{
    enum mf_status status; status = run(f);
    if (status != expected) fprintf(stderr, "expected %d, got %d for %s\n", (int)expected, (int)status, f->first);
    CHECK(status == expected); CHECK(!f->result.valid_output && !f->valid);
    if (f->as) {
        CHECK(f->result.status == expected); CHECK(f->reports == 1 && f->diagnostic.code == expected);
        CHECK(f->begins ? f->finishes == 1 : f->finishes == 0);
    }
}
static size_t symbol_index(struct fixture *f, const char *name)
{
    size_t i, j, n; n = strlen(name);
    for (i = 0; i < f->symbol_count; ++i) {
        if (f->symbols[i].name.length != n) continue;
        for (j = 0; j < n && f->symbols[i].name.data[j] == ascii(name[j]); ++j) { }
        if (j == n) return i;
    }
    CHECK(0); return 0;
}
static void constants(void)
{
    static const mf_octet expected[] = {
        0x7f,0,0x80,0,0x7f,0xff,0,0,
        0x80,0,0,0,0x7f,0xff,0xff,0xff,
        0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xc1,0x7d,0xc2,0x40,
        0x0a,0xbc,0,0,0xab,0,0xab,0,0xab};
    struct fixture f; size_t i;
    init(&f, "CONSTS CSECT\n DC X'7F'\n DS 0H\nHMIN DC H'-32768'\nHMAX DC H'32767'\n DS 0F\nFMIN DC F'-2147483648'\nFMAX DC F'2147483647'\n DS 0D\nADPOS DC AD(X'0123456789ABCDEF')\nADNEG DC AD(-1)\nAFOUR DC A(4294967295)\nCHARS DC CL4'A''B'\n DC X'ABC',XL3'AB',2X'00AB'\n END CONSTS\n");
    successful(&f); CHECK(f.section_count == 1 && f.sections[0].length == 49);
    CHECK(!memcmp(f.data[0], expected, 49));
    CHECK(f.gaps == 2 && !f.present[0][1] && !f.present[0][6] && !f.present[0][7]);
    CHECK(f.entry.present && f.entry.section == 1 && f.entry.offset == 0);
    for (i = 0; i < 49; ++i) if (i != 1 && i != 6 && i != 7) CHECK(f.present[0][i]);
    CHECK(f.symbols[symbol_index(&f, "HMIN")].offset == 2);
    CHECK(f.symbols[symbol_index(&f, "FMIN")].offset == 8);
    CHECK(f.symbols[symbol_index(&f, "ADPOS")].offset == 16);
    CHECK(f.fixup_count == 0); clean(&f);
}
static void forms(void)
{
    static const mf_octet expected[] = {
        0x18,0x12,0x1a,0x34,0x1b,0x56,0x07,0xfe,
        0x58,0x12,0x3f,0xff,0x50,0x40,0x50,0,
        0x89,0x60,0,0x1f,0xba,0x12,0x30,0x40,
        0x95,0xff,0x80,0,0x91,0x80,0x9f,0xff,
        0xd2,0,0xa0,0,0xb0,0,0xd5,0xff,0xcf,0xff,0xdf,0xff,
        0xf8,0x21,0x10,0x20,0x20,0x30,0x0a,0xff};
    struct fixture f;
    init(&f, "FORMS CSECT\n LR 1,2\n AR 3,4\n SR 5,6\n BCR 15,14\n L 1,4095(2,3)\n ST 4,0(,5)\n SLL 6,31\n CS 1,2,64(3)\n CLI 0(8),X'FF'\n TM 4095(9),X'80'\n MVC 0(1,10),0(11)\n CLC 4095(256,12),4095(13)\n ZAP 32(3,1),48(2,2)\n SVC 255\n END FORMS\n");
    f.config.profile = MF_S370; successful(&f);
    CHECK(f.sections[0].length == sizeof expected); CHECK(!memcmp(f.data[0], expected, sizeof expected));
    CHECK(f.fixup_count == 0 && f.gaps == 0); clean(&f);
}
static void expressions_and_bases(void)
{
    static const mf_octet expected[] = {
        0x58,0x10,0xc0,0x04,0x50,0x20,0xa0,0,
        0x41,0x32,0xc0,0x04,0x7f,0xff,0xff,0xff,
        0,0,0,4,0,0,0,10};
    struct fixture f;
    init(&f, "BASE CSECT\nNUM EQU -(1-(2+3))\nBASE AMODE 31\nBASE RMODE ANY\n USING *,12\n L 1,TARGET\nTARGET EQU *\n USING TARGET,10\n ST 2,TARGET\n DROP 10\n LA 3,TARGET(2,12)\n DC F'2147483647',A(TARGET-BASE),A(NUM+X'A'-4)\n END TARGET\n");
    successful(&f); CHECK(f.sections[0].length == sizeof expected);
    CHECK(!memcmp(f.data[0], expected, sizeof expected));
    CHECK(f.sections[0].amode == 31 && f.sections[0].rmode == 31);
    CHECK(f.entry.present && f.entry.section == 1 && f.entry.offset == 4);
    CHECK(f.fixup_count == 0); clean(&f);
}
static void narrow_addresses(void)
{
    static const mf_octet expected[] = {255,0x12,0x34,0xab,0xcd,0xef};
    struct fixture f;
    init(&f, "S CSECT\n DC AL1(255),AL2(X'1234'),AL3(X'ABCDEF')\n END S\n");
    successful(&f); CHECK(f.sections[0].length == sizeof expected);
    CHECK(!memcmp(f.data[0], expected, sizeof expected) && !f.fixup_count); clean(&f);
}
static void deferred_modes(void)
{
    struct fixture f;
    init(&f, " AMODE ANY\n RMODE ANY\nLATER AMODE 31\nLATER RMODE 24\n CSECT\n LR 1,2\nLATER CSECT\n LR 3,4\n END\n");
    successful(&f); CHECK(f.result.sections == 2);
    CHECK(f.sections[0].amode == 0 && f.sections[0].rmode == 31);
    CHECK(f.sections[1].amode == 31 && f.sections[1].rmode == 24);
    CHECK(f.sections[0].length == 2 && f.sections[1].length == 2); clean(&f);
}
static void relocations_and_sections(void)
{
    static const mf_octet first[] = {0,0,0,1,0,0,0,0,0xff,0xff,0xff,0xff,0,0,0,0};
    static const mf_octet second[] = {0,0,0,8,0,0,0,3};
    struct fixture f; size_t i;
    init(&f, "LONG_INTERNAL_DUMMY_SECTION DSECT\nLONG_INTERNAL_FIELD DS F\nNEXT_FIELD DS H\nCODE CSECT\n EXTRN EXT\n ENTRY START\n DS 0F\nSTART DC A(DATA+1),V(EXT),A(-EXT-1),A(EXT)\nLONG_INTERNAL_LABEL EQU START+4\nDATA CSECT\nITEM DC A(START+8),A(3)\nCODE CSECT\n END START\n");
    successful(&f); CHECK(f.section_count == 3 && f.sections[0].dummy && f.sections[0].length == 6);
    CHECK(f.sections[1].length == 16 && f.sections[2].length == 8);
    CHECK(!memcmp(f.data[1], first, sizeof first)); CHECK(!memcmp(f.data[2], second, sizeof second));
    CHECK(f.texts == 6 && f.gaps == 0 && f.fixup_count == 5 && f.result.fixups == 5);
    CHECK(f.symbols[symbol_index(&f, "START")].kind == MF_EXPORT);
    CHECK(f.symbols[symbol_index(&f, "LONG_INTERNAL_LABEL")].offset == 4);
    CHECK(f.entry.present && f.entry.section == 2 && f.entry.offset == 0);
    for (i = 0; i < 5; ++i) CHECK(f.fixups[i].width == 4);
    CHECK(f.fixups[0].section == 2 && f.fixups[0].offset == 0 && f.fixups[0].target_kind == MF_REF_SECTION && f.fixups[0].target == 3);
    CHECK(f.fixups[1].offset == 4 && f.fixups[1].target_kind == MF_REF_EXTERNAL && f.fixups[1].address_kind == MF_ADDRESS_V);
    CHECK(f.fixups[2].offset == 8 && f.fixups[2].subtract && f.fixups[2].address_kind == MF_ADDRESS_A);
    CHECK(f.fixups[3].offset == 12 && !f.fixups[3].subtract && f.fixups[3].address_kind == MF_ADDRESS_A);
    CHECK(f.fixups[4].section == 3 && f.fixups[4].offset == 0 && f.fixups[4].target == 2);
    clean(&f);
}
struct bad_case { const char *source; enum mf_status status; };
static void bad_sources(void)
{
    static const struct bad_case cases[] = {
        {" DC F'2147483648'\n END\n", MF_RANGE},
        {" DC F'-2147483649'\n END\n", MF_RANGE},
        {" DC H'32768'\n END\n", MF_RANGE},
        {" DC H'-32769'\n END\n", MF_RANGE},
        {" DC A(4294967296)\n END\n", MF_RANGE},
        {" DC A(-2147483649)\n END\n", MF_RANGE},
        {" DC AD(-9223372036854775809)\n END\n", MF_RANGE},
        {" DC AD(18446744073709551616)\n END\n", MF_RANGE},
        {"S CSECT\n DC AD(S)\n END\n", MF_UNSUPPORTED},
        {"S CSECT\n DC H'S'\n END\n", MF_UNSUPPORTED},
        {"S CSECT\n DC F'S'\n END\n", MF_UNSUPPORTED},
        {" DC V(1)\n END\n", MF_SOURCE},
        {" EXTRN EXT\n DC V(EXT+1)\n END\n", MF_SOURCE},
        {" EXTRN EXT\n DC V(-EXT)\n END\n", MF_SOURCE},
        {"A EQU B\nB EQU 1\n DC A(A)\n END\n", MF_UNDEFINED},
        {" DC A(MISSING)\n END\n", MF_UNDEFINED},
        {" L 1,MISSING\n END\n", MF_UNDEFINED},
        {" ENTRY MISSING\n LR 1,2\n END\n", MF_UNDEFINED},
        {"A EQU 1\nA EQU 2\n LR 1,2\n END\n", MF_DUPLICATE},
        {" EXTRN EXT,EXT\n LR 1,2\n END\n", MF_DUPLICATE},
        {" EXTRN EXT\nEXT EQU 1\n LR 1,2\n END\n", MF_DUPLICATE},
        {"D DSECT\n L 1,0\n END\n", MF_SOURCE},
        {"D DSECT\n ENTRY FIELD\nFIELD DS F\nC CSECT\n LR 1,2\n END C\n", MF_SOURCE},
        {"D DSECT\nFIELD DC A(D)\nC CSECT\n LR 1,2\n END C\n", MF_SOURCE},
        {"S CSECT\n SVC 256\n END\n", MF_RANGE},
        {" LR 16,0\n END\n", MF_RANGE},
        {" LR -1,0\n END\n", MF_RANGE},
        {" LR 1\n END\n", MF_SOURCE},
        {" L 1,4096\n END\n", MF_RANGE},
        {" L 1,0(16,1)\n END\n", MF_RANGE},
        {" SLL 1,0(16)\n END\n", MF_RANGE},
        {" MVI 0,256\n END\n", MF_RANGE},
        {" MVC 0(-1,1),0(2)\n END\n", MF_RANGE},
        {" MVC 0(257,1),0(2)\n END\n", MF_RANGE},
        {" ZAP 0(17,1),0(2,2)\n END\n", MF_RANGE},
        {" MP 0(2,1),0(2,2)\n END\n", MF_RANGE},
        {" MR 1,2\n END\n", MF_RANGE},
        {" BASR 1,2\n END\n", MF_UNSUPPORTED},
        {"S CSECT\n USING S+1,0\n LR 1,2\n END\n", MF_RANGE},
        {"S CSECT\n USING S,12\n DROP 12\n L 1,S\n END\n", MF_RANGE},
        {"S CSECT\n USING S,12\n L 1,S(,11)\n END\n", MF_RANGE},
        {"S CSECT\n USING S,12\n DS 4096C\n L 1,*\n END\n", MF_RANGE},
        {"S CSECT\n USING S,12\nT CSECT\n L 1,T\n END\n", MF_RANGE},
        {"S CSECT\n AMODE 64\n LR 1,2\n END\n", MF_RANGE},
        {"S CSECT\n RMODE 31\n LR 1,2\n END\n", MF_UNDEFINED},
        {" MACRO\n END\n", MF_UNSUPPORTED},
        {" COPY MEMBER\n END\n", MF_UNSUPPORTED},
        {" ORG 0\n END\n", MF_UNSUPPORTED},
        {" L 1,=F'1'\n END\n", MF_UNSUPPORTED},
        {" DC A(2*3)\n END\n", MF_UNSUPPORTED},
        {" DC A(2/3)\n END\n", MF_UNSUPPORTED},
        {"S CSECT\nT CSECT\n DC A(S+T)\n END\n", MF_UNSUPPORTED},
        {"S CSECT\n DC A(S+S)\n END\n", MF_UNSUPPORTED},
        {" DC 4294967296F'1'\n END\n", MF_RANGE},
        {" DC CL1'AB'\n END\n", MF_RANGE},
        {" DC XL1'ABC'\n END\n", MF_RANGE},
        {" DC X'G0'\n END\n", MF_SOURCE},
        {" DC FL8'1'\n END\n", MF_UNSUPPORTED},
        {" DS 4294967295F\n END\n", MF_RANGE},
        {" DS 4294967295C\n DS 1C\n END\n", MF_RANGE},
        {" LR 1,2\n", MF_SOURCE},
        {" END\n", MF_SOURCE},
        {" LR 1,2\n END\n LR 1,2\n", MF_SOURCE},
        {" DC F'1\n END\n", MF_SOURCE},
        {" EXTRN EXT,\n LR 1,2\n END\n", MF_SOURCE},
        {" EQU 1\n END\n", MF_SOURCE},
        {"D DSECT\n DS F\n END D\n", MF_SOURCE}
    };
    struct fixture f; size_t i;
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        init(&f, cases[i].source); failed(&f, cases[i].status); clean(&f);
    }
}
static void limits(void)
{
    struct fixture f; size_t i; struct mf_as *as;
    init(&f, "A CSECT\nB CSECT\n END A\n"); f.config.max_sections = 1; failed(&f, MF_LIMIT); clean(&f);
    init(&f, "A EQU 1\nB EQU 2\n LR 1,2\n END\n"); f.config.max_symbols = 1; failed(&f, MF_LIMIT); clean(&f);
    init(&f, "S CSECT\n DC 2A(S)\n END\n"); f.config.max_fixups = 1; failed(&f, MF_LIMIT); clean(&f);
    init(&f, "S CSECT\n DC 2A(FORWARD)\nFORWARD DC F'0'\n END\n"); f.config.max_fixups = 1; failed(&f, MF_LIMIT); CHECK(f.invalids == 1); clean(&f);
    init(&f, "A EQU (((1)))\n LR 1,2\n END\n"); f.config.max_expression_depth = 3; failed(&f, MF_LIMIT); clean(&f);
    init(&f, " LR 1,2\n END\n"); f.config.max_statement = 4; failed(&f, MF_LIMIT); clean(&f);
    for (i = 1; i <= 7; ++i) {
        init(&f, "S CSECT\n LR 1,2\n END\n"); f.allocation_fail = i;
        failed(&f, MF_LIMIT); CHECK(f.as == NULL || f.begins == 0); clean(&f);
    }
    init(&f, " LR 1,2\n END\n"); f.config.max_symbols = (size_t)-1;
    as = (struct mf_as *)1; CHECK(mf_as_create(&f.config, &f.storage, &as) == MF_LIMIT);
    CHECK(as == NULL && f.allocation_calls == 0); clean(&f);
    init(&f, " LR 1,2\n END\n"); f.config.max_sections = (size_t)-1;
    CHECK(mf_as_create(&f.config, &f.storage, &as) == MF_LIMIT); CHECK(f.allocation_calls == 0); clean(&f);
    init(&f, " LR 1,2\n END\n"); f.config.profile = (enum mf_profile)99;
    CHECK(mf_as_create(&f.config, &f.storage, &as) == MF_UNSUPPORTED); CHECK(f.allocation_calls == 0); clean(&f);
    init(&f, " LR 1,2\n END\n"); f.config.max_expression_depth = 0;
    CHECK(mf_as_create(&f.config, &f.storage, &as) == MF_LIMIT); clean(&f);
    init(&f, " LR 1,2\n END\n"); f.budget = 1; failed(&f, MF_LIMIT); clean(&f);
}
static void replay_and_providers(void)
{
    static const char *changed[] = {
        "S CSECT\n LR 1,3\n END S\n",
        "S CSECT\n DC F'1'\n END S\n",
        "S CSECT\n LR 1,2\n END S\n END S\n",
        "S CSECT\n LR 1,2\n",
        "T CSECT\n LR 1,2\n END T\n",
        "\nS CSECT\n LR 1,2\n END S\n",
        "S CSECT\n XX 1,2\n END S\n"};
    struct fixture f; size_t i; int pass;
    for (i = 0; i < sizeof changed / sizeof changed[0]; ++i) {
        init(&f, "S CSECT\n LR 1,2\n END S\n"); f.second = changed[i];
        failed(&f, MF_REPLAY); CHECK(f.invalids == 1); clean(&f);
    }
    init(&f, " LR 1,2\n END\n"); f.replay_fail = 1;
    failed(&f, MF_IO); CHECK(f.begins == 0 && f.diagnostic.detail.length == 0); clean(&f);
    for (pass = 1; pass <= 2; ++pass) {
        init(&f, " LR 1,2\n END\n"); f.fail_pass = pass; f.fail_next = 2; f.provider_status = MF_IO;
        failed(&f, MF_IO); CHECK(f.diagnostic.origin.source == 93 && f.diagnostic.origin.line == 321 && f.diagnostic.origin.column == 17);
        CHECK(f.diagnostic.detail.length == 0); CHECK(pass == 1 ? f.begins == 0 : f.invalids == 1); clean(&f);
    }
    init(&f, " LR 1,2\n END\n"); f.malformed_span = 1;
    failed(&f, MF_SOURCE); CHECK(f.begins == 0); clean(&f);
    init(&f, " LR 1,2\n END\n"); f.malformed_span = 2;
    failed(&f, MF_SOURCE); CHECK(f.begins == 0 && f.diagnostic.detail.length == 0); clean(&f);
}
static void writer_failures(void)
{
    struct fixture f; int i;
    for (i = 0; i < 6; ++i) {
        init(&f, "S CSECT\n LR 1,2\n DS 0F\n DC A(S)\n END S\n");
        if (i == 0) f.fail_begin = 1; if (i == 1) f.fail_text = 1;
        if (i == 2) f.fail_gap = 1; if (i == 3) f.fail_fixup = 1;
        if (i == 4) f.fail_entry = 1; if (i == 5) f.fail_finish = 1;
        failed(&f, MF_IO); CHECK(f.begins == 1 && f.finishes == 1);
        CHECK(i == 5 ? f.invalids == 0 : f.invalids == 1); clean(&f);
    }
}
static void real_writer(struct fixture *f)
{
    struct mf_sink sink;
    sink.cookie = f; sink.begin = sink_begin; sink.write = sink_write; sink.finish = sink_finish;
    CHECK(mf_obj_create(&f->storage, &sink, 8, 64, &f->object, &f->writer) == MF_OK);
}
static void integrated_sink_failures(void)
{
    struct fixture f; int i; enum mf_status status;
    for (i = 0; i < 6; ++i) {
        init(&f, "S CSECT\n DC A(S)\n END S\n"); real_writer(&f);
        if (i == 0) f.sink_fail_begin = 1;
        if (i >= 1 && i <= 4) f.sink_fail_write = (unsigned)i;
        if (i == 5) f.sink_fail_finish = 1;
        CHECK(run(&f) == MF_IO); CHECK(!f.result.valid_output && !f.sink_valid);
        CHECK(f.sink_finishes >= 1 && f.reports == 1); clean(&f);
    }
    init(&f, "S CSECT\n DC A(S)\n END S\n"); real_writer(&f);
    status = run(&f); CHECK(status == MF_OK && f.result.valid_output && f.sink_valid);
    CHECK(f.deck_length == 320); clean(&f);
    init(&f, "LONG_INTERNAL_DUMMY_SECTION DSECT\nFIELD DS F\nS CSECT\nLONG_INTERNAL_SYMBOL DC F'1'\n END S\n"); real_writer(&f);
    CHECK(run(&f) == MF_OK && f.sink_valid); clean(&f);
    init(&f, "LONG_CSECT_NAME CSECT\n LR 1,2\n END\n"); real_writer(&f);
    CHECK(run(&f) == MF_LIMIT && !f.result.valid_output && !f.sink_valid); clean(&f);
    init(&f, "S CSECT\n ENTRY LONG_EXPORT_NAME\nLONG_EXPORT_NAME LR 1,2\n END\n"); real_writer(&f);
    CHECK(run(&f) == MF_LIMIT && !f.result.valid_output && !f.sink_valid); clean(&f);
    init(&f, "S CSECT\n EXTRN LONG_EXTERNAL_NAME\n LR 1,2\n END\n"); real_writer(&f);
    CHECK(run(&f) == MF_LIMIT && !f.result.valid_output && !f.sink_valid); clean(&f);
    init(&f, " DS 16777216C\n END\n"); real_writer(&f);
    CHECK(run(&f) == MF_RANGE && !f.result.valid_output && !f.sink_valid); clean(&f);
}
static void constant_storage(void)
{
    struct fixture a, b;
    init(&a, ""); a.repeat_count = 1; a.discard = 1; successful(&a);
    init(&b, ""); b.repeat_count = 10000; b.discard = 1; successful(&b);
    CHECK(a.used == b.used && a.allocation_count == b.allocation_count);
    CHECK(a.result.storage_requested == b.result.storage_requested);
    CHECK(b.result.statements == 10001 && b.texts == 10000 && b.emitted == 20000);
    CHECK(b.sections[0].length == 20000); clean(&a); clean(&b);
}
static void boundary_and_regression_cases(void)
{
    static const mf_octet wide[] = {
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    static const mf_octet grouped[] = {
        0x58,0x10,0,5,0x58,0x20,0,12,0x58,0x30,0,3,
        0x58,0x42,0xc0,5,0x58,0x50,0xc0,0};
    struct fixture f; struct mf_statements bad; struct mf_object_writer writer;
    struct mf_as_result result; enum mf_status status; size_t i;
    init(&f, "S CSECT\n DC AD(18446744073709551615),AD(-9223372036854775808),AD(0)\n END S\n");
    successful(&f); CHECK(f.sections[0].length == sizeof wide);
    CHECK(!memcmp(f.data[0], wide, sizeof wide)); clean(&f);
    init(&f, "S CSECT\n USING S,12\n L 1,(4+1)\n L 2,(12)\n L 3,4-(1)\n L 4,(S+5)(2,12)\n L 5,(,12)\n END S\n");
    successful(&f); CHECK(!memcmp(f.data[0], grouped, sizeof grouped)); clean(&f);
    init(&f, "A CSECT\nB CSECT\nA CSECT\n DS 0F\nB CSECT\nA CSECT\n END\n");
    successful(&f); CHECK(f.section_count == 2 && f.sections[0].length == 0 && f.sections[1].length == 0); clean(&f);
    init(&f, " DC X'01'\nHERE LR 1,2\n END HERE\n");
    successful(&f); CHECK(f.sections[0].name.length == 0 && f.sections[0].length == 4);
    CHECK(f.data[0][0] == 1 && !f.present[0][1] && f.data[0][2] == 0x18 && f.data[0][3] == 0x12);
    CHECK(f.gaps == 1 && f.entry.offset == 2); clean(&f);
    init(&f, " DC A(1)\n END\n"); f.config.max_fixups = 0; successful(&f); clean(&f);
    init(&f, "S CSECT\n DC A(S)\n END\n"); f.config.max_fixups = 0; failed(&f, MF_LIMIT); clean(&f);
    /* Layout counts these enormous but representable declarations once. The
     * writer refuses immediately; no billions-of-elements emission follows. */
    init(&f, " DC 4294967295X'00'\n END\n"); f.fail_begin = 1;
    failed(&f, MF_IO); CHECK(f.sections[0].length == U32MAX && f.texts == 0); clean(&f);
    init(&f, " DC 1073741823F'1'\n END\n"); f.fail_begin = 1;
    failed(&f, MF_IO); CHECK(f.sections[0].length == 0xfffffffcUL && f.texts == 0); clean(&f);
    init(&f, " LR 1,2\n END\n");
    CHECK(mf_as_create(&f.config, &f.storage, &f.as) == MF_OK);
    bad = f.wrapped; writer = f.writer;
    CHECK(mf_as_assemble(NULL, &bad, &writer, NULL, &result) == MF_SOURCE);
    CHECK(result.status == MF_SOURCE && !result.valid_output);
    for (i = 0; i < 8; ++i) {
        bad = f.wrapped; writer = f.writer;
        if (i == 0) bad.next = NULL;
        if (i == 1) bad.replay = NULL;
        if (i == 2) writer.begin = NULL;
        if (i == 3) writer.text = NULL;
        if (i == 4) writer.gap = NULL;
        if (i == 5) writer.fixup = NULL;
        if (i == 6) writer.entry = NULL;
        if (i == 7) writer.finish = NULL;
        CHECK(mf_as_assemble(f.as, &bad, &writer, NULL, &result) == MF_SOURCE);
        CHECK(result.status == MF_SOURCE && !result.valid_output);
    }
    CHECK(mf_as_assemble(f.as, NULL, &f.writer, NULL, &result) == MF_SOURCE && result.status == MF_SOURCE);
    CHECK(mf_as_assemble(f.as, &f.wrapped, NULL, NULL, &result) == MF_SOURCE && result.status == MF_SOURCE);
    CHECK(mf_as_assemble(f.as, &f.wrapped, &f.writer, NULL, NULL) == MF_SOURCE);
    status = mf_as_assemble(f.as, &f.wrapped, &f.writer, NULL, &result);
    CHECK(status == MF_OK && result.valid_output);
    CHECK(mf_as_assemble(f.as, &f.wrapped, &f.writer, NULL, &result) == MF_SOURCE);
    CHECK(result.status == MF_SOURCE && !result.valid_output); clean(&f);
}
static void address_state(void)
{
    static const mf_octet expected[] = {0x58,0x10,0,4,0x58,0x20,0xa0,0,0x58,0x30,0xa0,0,0x58,0x40,0xc0,8};
    struct fixture f; char source[512]; unsigned i;
    init(&f, "S CSECT\n USING D,0\n L 1,FIELD\n USING S,12\n PUSH USING\n USING S+8,10\n L 2,S+8\n DROP ,\n USING S+8,10\n L 3,S+8\n POP USING\n L 4,S+8\nD DSECT\n DS F\nFIELD DS F\n END S\n");
    successful(&f); CHECK(f.sections[0].length == sizeof expected);
    CHECK(!memcmp(f.data[0],expected,sizeof expected)); clean(&f);
    init(&f, "S CSECT\n USING 4096,3\n LA 1,4100\n USING 0,4\n LA 2,8\n END S\n");
    successful(&f); CHECK(f.data[0][2] == 0x30 && f.data[0][3] == 4 && f.data[0][6] == 0x40 && f.data[0][7] == 8); clean(&f);
    init(&f, "S CSECT\n USING S,12,11\n L 1,S+4098\n END S\n");
    successful(&f); CHECK(f.data[0][2] == 0xb0 && f.data[0][3] == 2); clean(&f);
    init(&f, "S CSECT\n POP USING\n END S\n"); CHECK(run(&f) == MF_SOURCE); clean(&f);
    strcpy(source,"S CSECT\n"); for (i = 0; i < 17; ++i) strcat(source," PUSH USING\n"); strcat(source," END S\n");
    init(&f,source); CHECK(run(&f) == MF_LIMIT); clean(&f);
    init(&f, "S CSECT\n USING MISSING,0\n L 1,0\n END S\n"); CHECK(run(&f) == MF_UNDEFINED); clean(&f);
}
static void declaration_expressions(void)
{
    static const mf_octet expected[] = {0xaa,0xaa,0xaa,0xaa,0x58,0x1f,0,0,0,0,0,0};
    struct fixture f;
    init(&f,"S CSECT\nSIZE EQU 4\n DC (SIZE)X'AA'\n L 1,0(B'1111',0)\n DS (SIZE-2)H\n END S\n");
    successful(&f); CHECK(f.sections[0].length == sizeof expected);
    CHECK(!memcmp(f.data[0],expected,sizeof expected)); CHECK(!f.present[0][8]); clean(&f);
    init(&f,"S CSECT\nTOP EQU 8\n DC (TOP-*+S)X'00'\n END S\n");
    successful(&f); CHECK(f.sections[0].length == 8); clean(&f);
    init(&f," DC (-1)X'00'\n END\n"); CHECK(run(&f) == MF_RANGE); clean(&f);
    init(&f," DC (MISSING)X'00'\n END\n"); CHECK(run(&f) == MF_UNDEFINED); clean(&f);
    init(&f," L 1,0(B'12',0)\n END\n"); CHECK(run(&f) == MF_SOURCE); clean(&f);
    init(&f," L 1,0(B'',0)\n END\n"); CHECK(run(&f) == MF_SOURCE); clean(&f);
    init(&f," DC A(B'11111111111111111111111111111111')\n END\n");
    successful(&f); CHECK(f.data[0][0] == 255 && f.data[0][3] == 255); clean(&f);
    init(&f," DC A(B'100000000000000000000000000000000')\n END\n"); CHECK(run(&f) == MF_RANGE); clean(&f);
    init(&f,"S CSECT\nBYTES DC 4AL1(*-BYTES)\n END S\n");
    successful(&f); CHECK(f.data[0][0] == 0 && f.data[0][1] == 1 && f.data[0][2] == 2 && f.data[0][3] == 3); clean(&f);
}
static void source_metadata(void)
{
    static const mf_octet id[8] = {0xc4,0xc5,0xc3,0xd2,0x40,0x40,0x40,0x40};
    struct fixture f; size_t i;
    init(&f, "DECK TITLE 'first title'\n PRINT GEN,,ON\nDECK EQU 1\nS CSECT\n TITLE 'second title'\n PRINT NOGEN,OFF,DATA,NODATA\n LR 1,2\n END S\n");
    real_writer(&f); CHECK(run(&f) == MF_OK && f.sink_valid);
    CHECK(f.deck_length == 240 && f.deck[96] == 0x18 && f.deck[97] == 0x12);
    for (i = 0; i < f.deck_length; i += 80) CHECK(!memcmp(f.deck+i+72,id,8));
    clean(&f);
    init(&f, " TITLE ''\n LR 1,2\n END\n"); CHECK(run(&f) == MF_SOURCE); clean(&f);
    init(&f, "X TITLE 'hello'\nX TITLE 'again'\n LR 1,2\n END\n"); CHECK(run(&f) == MF_DUPLICATE); clean(&f);
    init(&f, "LONGTITLE TITLE 'hello'\n LR 1,2\n END\n"); CHECK(run(&f) == MF_LIMIT); clean(&f);
    init(&f, "X TITLE 'hello'\n LR 1,2\n END\n"); CHECK(run(&f) == MF_UNSUPPORTED); clean(&f);
    init(&f, " PRINT UNKNOWN\n LR 1,2\n END\n"); CHECK(run(&f) == MF_UNSUPPORTED); clean(&f);
    init(&f, "X PRINT GEN\n LR 1,2\n END\n"); CHECK(run(&f) == MF_SOURCE); clean(&f);
}
int main(void)
{
    declaration_expressions(); address_state(); source_metadata(); constants(); forms(); narrow_addresses(); deferred_modes(); expressions_and_bases(); relocations_and_sections();
    bad_sources(); limits(); replay_and_providers(); writer_failures();
    integrated_sink_failures(); constant_storage(); boundary_and_regression_cases();
    printf("assemble: %lu checks passed\n", checks); return 0;
}
