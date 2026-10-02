/* SPDX-License-Identifier: MIT
 * Original in-memory library adapter; no filesystem dependency in the core.
 */
#define main macro_existing_tests
#include "test_macro.c"
#undef main
struct mock_member { const char *name, *text, *second; enum mf_encoding encoding; };
struct handle { struct fixture records; unsigned identity; int active; };
struct library_fixture {
    struct fixture root;
    struct mock_member members[4]; struct handle handles[8];
    unsigned opens, closes; int io_open, io_close;
};
static enum mf_status member_next(void *cookie, struct mf_record *r)
{
    struct handle *h; enum mf_status st; h = (struct handle *)cookie;
    st = record_next(&h->records, r); r->origin.source = h->identity; return st;
}
static enum mf_status member_replay(void *cookie)
{ return replay(&((struct handle *)cookie)->records); }
static enum mf_status open_member(void *cookie, struct mf_span name,
    struct mf_records *r, unsigned *identity)
{
    struct library_fixture *l; struct handle *h; size_t i, j;
    l = (struct library_fixture *)cookie;
    if (l->io_open) return MF_IO;
    for (i = 0; i < 4; ++i) if (l->members[i].name && matches(name, l->members[i].name)) break;
    if (i == 4) return MF_UNDEFINED;
    for (j = 0; j < 8; ++j) if (!l->handles[j].active) break;
    CHECK(j < 8); h = l->handles + j;
    init(&h->records, l->root.pass == 2 && l->members[i].second ?
        l->members[i].second : l->members[i].text);
    h->records.records.encoding = l->members[i].encoding;
    h->identity = (unsigned)i + 20; h->active = 1;
    r->cookie = h; r->encoding = l->members[i].encoding;
    r->next = member_next; r->replay = member_replay;
    *identity = h->identity; ++l->opens; return MF_OK;
}
static enum mf_status close_member(void *cookie, struct mf_records *r)
{
    struct library_fixture *l; struct handle *h;
    l = (struct library_fixture *)cookie; h = (struct handle *)r->cookie;
    CHECK(h->active); h->active = 0; ++l->closes;
    return l->io_close ? MF_IO : MF_OK;
}
static void library_init(struct library_fixture *l, const char *text)
{
    memset(l, 0, sizeof *l); init(&l->root, text);
}
static void library_create(struct library_fixture *l)
{
    struct mf_macro_library adapter;
    CHECK(create(&l->root) == MF_OK);
    adapter.cookie = l; adapter.open = open_member; adapter.close = close_member;
    CHECK(mf_macro_set_library(l->root.macro, &adapter) == MF_OK);
}
static void library_clean(struct library_fixture *l)
{ clean(&l->root); CHECK(l->opens == l->closes); }
static void positive(void)
{
    struct library_fixture l; struct mf_statement s; enum mf_status st;
    unsigned pass; unsigned long n;
    library_init(&l, " COPY OUTER\nHERE LOAD 4,16(1)\n END HERE\n");
    l.members[0].name = "OUTER"; l.members[0].text = "* unused comment\n COPY INNER\n";
    l.members[1].name = "INNER"; l.members[1].text = "DEMO CSECT\n";
    l.members[1].encoding = MF_CP037;
    l.members[2].name = "LOAD";
    l.members[2].text = " MACRO\n&N LOAD &R,&A\n&N L &R,&A\n MEND\n* ignored\n";
    library_create(&l);
    for (pass = 0; pass < 2; ++pass) {
        st = l.root.statements.next(l.root.statements.cookie, &s);
        CHECK(st == MF_OK && matches(s.label, "DEMO") && matches(s.operation, "CSECT"));
        CHECK(s.origin.source == 21 && s.origin.line == 1);
        expected(&l.root, "HERE", "L", "4,16(1)", 2);
        CHECK(l.root.definition_origin.source == 22 && l.root.definition_origin.line == 2);
        expected(&l.root, "", "END", "HERE", 3);
        CHECK(drain(&l.root, &n) == MF_EOF && n == 0);
        if (!pass) CHECK(l.root.statements.replay(l.root.statements.cookie) == MF_OK);
    }
    CHECK(l.opens == 6); library_clean(&l);
}
static void library_failures(void)
{
    struct library_fixture l; unsigned long n;
    library_init(&l, " COPY MISSING\n"); library_create(&l);
    CHECK(drain(&l.root, &n) == MF_UNDEFINED); library_clean(&l);
    library_init(&l, " COPY ../BAD\n"); library_create(&l);
    CHECK(drain(&l.root, &n) == MF_SOURCE); library_clean(&l);
    library_init(&l, " COPY SELF\n");
    l.members[0].name = "SELF"; l.members[0].text = " COPY SELF\n";
    library_create(&l); CHECK(drain(&l.root, &n) == MF_SOURCE); library_clean(&l);
    library_init(&l, " COPY A\n"); l.root.config.max_depth = 1;
    l.members[0].name = "A"; l.members[0].text = " COPY B\n";
    l.members[1].name = "B"; l.members[1].text = "";
    library_create(&l); CHECK(drain(&l.root, &n) == MF_LIMIT); library_clean(&l);
    library_init(&l, " COPY A\n");
    l.members[0].name = "A"; l.members[0].text = "* unused first\n";
    l.members[0].second = "* unused changed\n";
    library_create(&l); CHECK(drain(&l.root, &n) == MF_EOF);
    CHECK(l.root.statements.replay(l.root.statements.cookie) == MF_OK);
    CHECK(drain(&l.root, &n) == MF_REPLAY); library_clean(&l);
    library_init(&l, " COPY A\n"); l.io_open = 1; library_create(&l);
    CHECK(drain(&l.root, &n) == MF_IO); library_clean(&l);
    library_init(&l, " COPY A\n"); l.io_close = 1;
    l.members[0].name = "A"; l.members[0].text = "";
    library_create(&l); CHECK(drain(&l.root, &n) == MF_IO); library_clean(&l);
    library_init(&l, " LOAD\n"); l.members[0].name = "LOAD";
    l.members[0].text = " MACRO\n OTHER\n MEND\n";
    library_create(&l); CHECK(drain(&l.root, &n) == MF_SOURCE); library_clean(&l);
}
int main(void)
{
    positive(); library_failures();
    printf("macro library: %lu checks passed\n", checks); return 0;
}
