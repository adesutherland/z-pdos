/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Original macro-to-object slice; expected L/ST octets and record meaning
 * are fixed independently, not generated through assembler helpers.
 */
#define main mf_macro_provider_test_entry
#include "test_macro.c"
#undef main
struct integration {
    struct fixture source; struct mf_as *assembler; struct mf_obj *object;
    struct mf_object_writer writer; struct mf_sink sink;
    struct mf_as_result result; struct mf_diagnostics diagnostics;
    mf_octet deck[8192]; size_t length;
    unsigned begins, writes, finishes, fail_write; int valid, fail_finish;
    struct mf_diagnostic diagnostic; unsigned reports;
};
static enum mf_status sink_begin(void *cookie)
{ struct integration *t; t = (struct integration *)cookie; ++t->begins; return MF_OK; }
static enum mf_status sink_write(void *cookie, const mf_octet *bytes, size_t n)
{
    struct integration *t; t = (struct integration *)cookie; ++t->writes;
    CHECK(n == 80 && n <= sizeof t->deck - t->length);
    if (t->writes == t->fail_write) {
        memcpy(t->deck + t->length, bytes, 7); t->length += 7; return MF_IO;
    }
    memcpy(t->deck + t->length, bytes, n); t->length += n; return MF_OK;
}
static enum mf_status sink_finish(void *cookie, int valid)
{
    struct integration *t; t = (struct integration *)cookie; ++t->finishes;
    t->valid = valid && !t->fail_finish;
    return valid && t->fail_finish ? MF_IO : MF_OK;
}
static void diagnostic(void *cookie, const struct mf_diagnostic *d)
{
    struct integration *t; t = (struct integration *)cookie;
    ++t->reports; t->diagnostic = *d;
    /* Only stable scalar diagnostic fields are inspected after the callback. */
    t->diagnostic.detail.data = NULL; t->diagnostic.detail.length = 0;
}
static void integration_init(struct integration *t, const char *text)
{
    memset(t, 0, sizeof *t); init(&t->source, text);
    t->sink.cookie = t; t->sink.begin = sink_begin; t->sink.write = sink_write;
    t->sink.finish = sink_finish; t->diagnostics.cookie = t;
    t->diagnostics.report = diagnostic;
}
static enum mf_status assemble(struct integration *t)
{
    struct mf_as_config c; enum mf_status st;
    CHECK(create(&t->source) == MF_OK);
    memset(&c, 0, sizeof c); c.profile = MF_S360; c.max_sections = 4;
    c.max_symbols = 32; c.max_fixups = 32; c.max_literals = 0;
    c.max_statement = 256; c.max_expression_depth = 16;
    CHECK(mf_as_create(&c, &t->source.storage, &t->assembler) == MF_OK);
    CHECK(mf_obj_create(&t->source.storage, &t->sink, 4, 32,
        &t->object, &t->writer) == MF_OK);
    st = mf_as_assemble(t->assembler, &t->source.statements, &t->writer,
        &t->diagnostics, &t->result);
    CHECK(st == t->result.status); return st;
}
static void integration_clean(struct integration *t)
{
    mf_as_destroy(t->assembler); mf_obj_destroy(t->object); clean(&t->source);
}
static unsigned long be(const mf_octet *p, unsigned n)
{ unsigned long v; v = 0; while (n--) v = v * 256UL + *p++; return v; }
static unsigned end_count(struct integration *t)
{
    size_t i; unsigned n; n = 0;
    for (i = 0; i + 80 <= t->length; i += 80)
        if (t->deck[i + 1] == 0xc5 && t->deck[i + 2] == 0xd5 && t->deck[i + 3] == 0xc4) ++n;
    return n;
}
static void independent_deck(struct integration *t)
{
    static const mf_octet expected[28] = {
        0x58,0x40,0x10,0x10,0x58,0x40,0x40,0,
        0x58,0x50,0x10,0x14,0x58,0xe0,0x10,0x18,
        0x50,0x60,0xe0,0,0x58,0x70,0x10,0x1c,0x50,0x60,0x70,0
    };
    static const mf_octet section_name[8] = {0xd4,0xc1,0xc3,0xe2,0xe3,0xc5,0xd7,0x40};
    static const mf_octet entry_name[8] = {0xc6,0xc9,0xd9,0xe2,0xe3,0x40,0x40,0x40};
    mf_octet bytes[28], seen[28]; size_t i, j, n, at; unsigned sd, ld, end;
    const mf_octet *card;
    memset(bytes, 0, sizeof bytes); memset(seen, 0, sizeof seen);
    sd = ld = end = 0; CHECK(t->length % 80 == 0);
    for (i = 0; i < t->length; i += 80) {
        card = t->deck + i; CHECK(card[0] == 2 && !end); n = (size_t)be(card + 10, 2);
        if (card[1] == 0xc5 && card[2] == 0xe2 && card[3] == 0xc4) {
            CHECK(n > 0 && n <= 48 && n % 16 == 0);
            for (j = 16; j < 16 + n; j += 16) {
                if (card[j + 8] == 0) {
                    CHECK(!sd++ && !memcmp(card + j, section_name, 8));
                    CHECK(be(card + j + 9, 3) == 0 && card[j + 12] == 1 && be(card + j + 13, 3) == 28);
                    CHECK(be(card + 14, 2) == 1);
                } else {
                    CHECK(card[j + 8] == 1 && !ld++ && !memcmp(card + j, entry_name, 8));
                    CHECK(be(card + j + 9, 3) == 0 && be(card + j + 14, 2) == 1);
                }
            }
        } else if (card[1] == 0xe3 && card[2] == 0xe7 && card[3] == 0xe3) {
            at = (size_t)be(card + 5, 3); CHECK(n > 0 && n <= 56 && be(card + 14, 2) == 1);
            CHECK(at <= 28 && n <= 28 - at);
            for (j = 0; j < n; ++j) { CHECK(!seen[at + j]); seen[at + j] = 1; bytes[at + j] = card[16 + j]; }
        } else {
            CHECK(card[1] == 0xc5 && card[2] == 0xd5 && card[3] == 0xc4);
            CHECK(be(card + 5, 3) == 0 && be(card + 14, 2) == 1); ++end;
        }
    }
    CHECK(sd == 1 && ld == 1 && end == 1);
    for (i = 0; i < 28; ++i) CHECK(seen[i] && bytes[i] == expected[i]);
}
static const char source_slice[] =
    " MACRO\n&N XLOAD &R,&A\n&N L &R,&A\n L &R,0(,&R)\n MEND\n"
    " MACRO\n&N XADDR &R,&A\n&N L &R,&A\n MEND\n"
    " MACRO\n&N XSTORE &R,&A,&S=14\n&N L &S,&A\n ST &R,0(,&S)\n MEND\n"
    "MACSTEP CSECT\n ENTRY FIRST\nFIRST XLOAD 4,16(1)\n XADDR 5,20(1)\n"
    " XSTORE 6,24(1)\n XSTORE 6,28(1),S=7\n END FIRST\n";
static void valid_slice(void)
{
    struct integration t;
    integration_init(&t, source_slice); CHECK(assemble(&t) == MF_OK);
    CHECK(t.result.valid_output && t.valid && !t.reports);
    CHECK(t.begins == 1 && t.finishes == 1 && t.result.sections == 1);
    CHECK(t.result.fixups == 0 && t.result.statements == 10);
    independent_deck(&t); integration_clean(&t);
}
static void invalid_slices(void)
{
    struct integration t;
    integration_init(&t, "S CSECT\n LR 1,2\n END S\n MACRO\n UNUSED\n LR 1,2\n MEND\n");
    t.source.second = "S CSECT\n LR 1,2\n END S\n MACRO\n UNUSED\n LR 1,3\n MEND\n";
    CHECK(assemble(&t) == MF_REPLAY);
    CHECK(!t.result.valid_output && !t.valid && t.begins == 1 && t.finishes == 1);
    CHECK(!end_count(&t) && t.reports == 1); integration_clean(&t);
    integration_init(&t, "S CSECT\n LR 1,2\n END S\n");
    t.source.fail_pass = 2; t.source.fail_next = 2;
    CHECK(assemble(&t) == MF_IO);
    CHECK(!t.result.valid_output && !t.valid && t.finishes == 1 && !end_count(&t));
    CHECK(t.diagnostic.origin.line == 999 && t.diagnostic.origin.column == 17); integration_clean(&t);
    integration_init(&t, " MACRO\n&N BAD &R\n&N LR &R,2\n MEND\nS CSECT\nHERE BAD 16\n END S\n");
    CHECK(assemble(&t) == MF_RANGE && !t.result.valid_output && !t.valid);
    CHECK(t.diagnostic.origin.source == 7 && t.diagnostic.origin.line == 6);
    CHECK(!end_count(&t)); integration_clean(&t);
    integration_init(&t, "S CSECT\n MACRO\n SELF\n SELF\n MEND\n SELF\n END S\n");
    t.source.config.max_depth = 1;
    CHECK(assemble(&t) == MF_LIMIT && !t.result.valid_output && !t.valid && !end_count(&t)); integration_clean(&t);
    integration_init(&t, source_slice); t.fail_write = 2;
    CHECK(assemble(&t) == MF_IO);
    CHECK(!t.result.valid_output && !t.valid && t.finishes == 1 && !end_count(&t)); integration_clean(&t);
    integration_init(&t, source_slice); t.fail_finish = 1;
    CHECK(assemble(&t) == MF_IO);
    CHECK(!t.result.valid_output && !t.valid && t.finishes == 2 && end_count(&t) == 1); integration_clean(&t);
    integration_init(&t, "S CSECT\n COPY SECRET\n END S\n");
    CHECK(assemble(&t) == MF_UNSUPPORTED && !t.result.valid_output && !t.valid && !end_count(&t)); integration_clean(&t);
}
int main(void)
{
    valid_slice(); invalid_slices();
    printf("macro assembly: %lu independent checks passed\n", checks); return 0;
}
