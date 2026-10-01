/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Original entry31 language checks. Expected bytes are independently stated.
 * Reuse the bootstrap callback fixture, not production parser/encoder helpers.
 */
#define main mf_bootstrap_engine_test_entry
#include "test_assemble.c"
#undef main
static void consumer_init(struct fixture *f, const char *source)
{
    init(f, source); f->config.max_literals = 64;
}
static void aliases(void)
{
    static const char *names[] = {"B","BO","BH","BP","BL","BM","BNE","BNZ",
        "BE","BZ","BNL","BNM","BNH","BNP","BNO","NOP"};
    static const unsigned masks[] = {15,1,2,2,4,4,7,7,8,8,11,11,13,13,14,0};
    char source[2048], line[64]; mf_octet expected[96]; size_t i, at;
    struct fixture f;
    strcpy(source, "S CSECT\n"); at = 0;
    for (i = 0; i < sizeof masks / sizeof masks[0]; ++i) {
        CHECK(strlen(names[i]) < 16);
        strcpy(line, " "); strcat(line, names[i]); strcat(line, " 0(1,2)\n ");
        strcat(line, names[i]); strcat(line, "R 14\n");
        CHECK(strlen(source) + strlen(line) + 16 < sizeof source); strcat(source, line);
        expected[at++] = 0x47; expected[at++] = (mf_octet)(masks[i] * 16 + 1);
        expected[at++] = 0x20; expected[at++] = 0;
        expected[at++] = 0x07; expected[at++] = (mf_octet)(masks[i] * 16 + 14);
    }
    strcat(source, " END S\n"); consumer_init(&f, source); successful(&f);
    CHECK(f.sections[0].length == sizeof expected); CHECK(!memcmp(f.data[0], expected, sizeof expected)); clean(&f);
    consumer_init(&f, "S CSECT\n USING S,12\n B FORWARD\nHERE BR 14\nFORWARD BZ HERE\n END S\n");
    successful(&f); CHECK(f.sections[0].length == 10);
    CHECK(f.data[0][0] == 0x47 && f.data[0][1] == 0xf0 && f.data[0][2] == 0xc0 && f.data[0][3] == 6);
    CHECK(f.data[0][4] == 0x07 && f.data[0][5] == 0xfe);
    CHECK(f.data[0][6] == 0x47 && f.data[0][7] == 0x80 && f.data[0][8] == 0xc0 && f.data[0][9] == 4); clean(&f);
}
static void literal_bytes(void)
{
    static const mf_octet expected[] = {
        0x58,0x10,0xc0,0x18,0x58,0x20,0xc0,0x1c,
        0x58,0x30,0xc0,0x20,0x58,0x40,0xc0,0x18,
        0x58,0x50,0xc0,0x1c,0,0,0,0,
        0xff,0xff,0xff,0xff,0x7f,0xff,0xff,0xff,
        0,0,0,0};
    struct fixture f; size_t external;
    consumer_init(&f, "S CSECT\n USING S,12\n L 1,=F'-1'\n L 2,=X'7FFFFFFF'\n L 3,=V(EXT)\n L 4,=f'-1'\n L 5,=x'7fffffff'\nPOOL LTORG\n END S\n");
    successful(&f); CHECK(f.sections[0].length == sizeof expected);
    CHECK(!memcmp(f.data[0], expected, sizeof expected)); CHECK(f.gaps == 1);
    CHECK(!f.present[0][20] && !f.present[0][23]);
    CHECK(f.symbols[symbol_index(&f, "POOL")].offset == 24);
    CHECK(f.fixup_count == 1 && f.fixups[0].section == 1 && f.fixups[0].offset == 32);
    external = symbol_index(&f, "EXT"); CHECK(f.symbols[external].kind == MF_EXTERNAL);
    CHECK(f.fixups[0].target_kind == MF_REF_EXTERNAL && f.fixups[0].target == f.symbols[external].id);
    CHECK(f.fixups[0].address_kind == MF_ADDRESS_V && f.fixups[0].width == 4); clean(&f);
}
static void address_literal(void)
{
    static const mf_octet expected[] = {0x58,0x10,0xc0,8,0,0,0,0,0,0,0,8};
    struct fixture f;
    consumer_init(&f, "S CSECT\n USING S,12\n L 1,=A(FORWARD+4)\nFORWARD DC F'0'\n END S\n");
    successful(&f); CHECK(f.sections[0].length == sizeof expected);
    CHECK(!memcmp(f.data[0], expected, sizeof expected));
    CHECK(f.fixup_count == 1 && f.fixups[0].offset == 8 && f.fixups[0].width == 4);
    CHECK(f.fixups[0].target_kind == MF_REF_SECTION && f.fixups[0].target == 1 && f.fixups[0].address_kind == MF_ADDRESS_A);
    clean(&f);
}
static void pool_grouping(void)
{
    struct fixture f;
    consumer_init(&f, "S CSECT\n USING S,12\n L 1,=X'01'\n L 2,=X'0203'\n L 3,=F'4'\n L 4,=X'05060708090A0B0C'\n L 5,=X'101112131415161718191A1B1C1D1E1F'\n L 6,=X'AA'\nP LTORG\n END S\n");
    successful(&f); CHECK(f.sections[0].length == 56);
    /* Six loads end24; pool starts24 (8mod16), not32. X16 precedes X8,
     * then F4, X2 and the two odd-length literals in encounter order. */
    CHECK(f.symbols[symbol_index(&f, "P")].offset == 24);
    CHECK(f.data[0][3] == 54 && f.data[0][7] == 52 && f.data[0][11] == 48);
    CHECK(f.data[0][15] == 40 && f.data[0][19] == 24 && f.data[0][23] == 55);
    CHECK(f.data[0][24] == 0x10 && f.data[0][39] == 0x1f);
    CHECK(f.data[0][40] == 5 && f.data[0][47] == 12);
    CHECK(f.data[0][48] == 0 && f.data[0][51] == 4);
    CHECK(f.data[0][52] == 2 && f.data[0][53] == 3 && f.data[0][54] == 1 && f.data[0][55] == 0xaa);
    clean(&f);
    /* Exact default-boundary check: X16 is permitted to start at8mod16. */
    consumer_init(&f, "S CSECT\n USING S,12\n L 1,=X'000102030405060708090A0B0C0D0E0F'\nP LTORG\n END S\n");
    successful(&f); CHECK(f.sections[0].length == 24);
    CHECK(f.symbols[symbol_index(&f, "P")].offset == 8 && f.data[0][3] == 8);
    CHECK(f.data[0][8] == 0 && f.data[0][23] == 15 && f.gaps == 1); clean(&f);
}
static void multiple_pools_and_sections(void)
{
    struct fixture f;
    consumer_init(&f, "S CSECT\n USING S,12\n L 1,=F'1'\n LTORG\n L 2,=F'1'\n L 3,=V(EXT)\n END S\n");
    successful(&f); CHECK(f.sections[0].length == 32);
    CHECK(f.data[0][3] == 8 && f.data[0][15] == 24 && f.data[0][19] == 28);
    CHECK(f.data[0][11] == 1 && f.data[0][27] == 1 && f.fixups[0].offset == 28); clean(&f);
    /* Collection is global: both references share one literal placed by
     * LTORG in B. USING B lets both sections address that actual owner. */
    consumer_init(&f, "B CSECT\nA CSECT\n USING B,12\n L 1,=F'7'\nB CSECT\n L 2,=F'7'\n LTORG\n END A\n");
    successful(&f); CHECK(f.section_count == 2 && f.sections[0].length == 12 && f.sections[1].length == 4);
    CHECK(f.data[1][3] == 8 && f.data[0][3] == 8 && f.data[0][11] == 7);
    CHECK(f.texts == 3); clean(&f);
    /* END places a pending pool at the end of the first real CSECT, even
     * when the last statement and original reference belong to another. */
    consumer_init(&f, "D DSECT\nFIELD DS F\nA CSECT\n LR 1,2\nB CSECT\n USING A,12\n L 1,=V(EXT)\n END B\n");
    successful(&f); CHECK(f.sections[1].length == 12 && f.sections[2].length == 4);
    CHECK(f.data[2][3] == 8 && f.fixup_count == 1 && f.fixups[0].section == 2 && f.fixups[0].offset == 8);
    CHECK(f.entry.section == 3 && f.entry.offset == 0); clean(&f);
}
static void implicit_externals(void)
{
    struct fixture f; size_t i;
    consumer_init(&f, "S CSECT\n DC V(EXT),V(ext),V(@@NAME)\n EXTRN EXT\n DC V(EXT)\n END S\n");
    successful(&f); CHECK(f.symbol_count == 3 && f.fixup_count == 4);
    i = symbol_index(&f, "EXT"); CHECK(f.symbols[i].kind == MF_EXTERNAL && f.symbols[i].section == 0);
    CHECK(f.fixups[0].target == f.fixups[1].target && f.fixups[0].target == f.fixups[3].target);
    CHECK(f.fixups[2].target != f.fixups[0].target); clean(&f);
    consumer_init(&f, " EXTRN EXT\nS CSECT\n USING S,12\n DC V(EXT)\n L 1,=V(EXT)\n L 2,=v(ext)\n END S\n");
    successful(&f); CHECK(f.symbol_count == 2 && f.fixup_count == 2);
    CHECK(f.fixups[0].target == f.fixups[1].target); clean(&f);
}
static void local_v_names(void)
{
    struct fixture f; size_t i, exports, externals;
    consumer_init(&f, "S CSECT\n DC V(LOCAL),A(LOCAL),V(S)\n ENTRY LOCAL\nLOCAL BR 14\n END S\n");
    successful(&f); CHECK(f.symbol_count == 4 && f.fixup_count == 3);
    CHECK(f.sections[0].length == 14 && f.data[0][7] == 12);
    CHECK(f.data[0][12] == 7 && f.data[0][13] == 0xfe);
    CHECK(f.fixups[0].address_kind == MF_ADDRESS_V && f.fixups[0].target_kind == MF_REF_EXTERNAL);
    CHECK(f.fixups[1].address_kind == MF_ADDRESS_A && f.fixups[1].target_kind == MF_REF_SECTION);
    CHECK(f.fixups[2].address_kind == MF_ADDRESS_V && f.fixups[2].target_kind == MF_REF_EXTERNAL);
    exports = externals = 0;
    for (i = 0; i < f.symbol_count; ++i) {
        if (f.symbols[i].kind == MF_EXPORT) { ++exports; CHECK(f.symbols[i].offset == 12); }
        if (f.symbols[i].kind == MF_EXTERNAL) { ++externals; CHECK(!f.symbols[i].offset && !f.symbols[i].section); }
    }
    CHECK(exports == 1 && externals == 2); clean(&f);
    consumer_init(&f, " DC V(ONLYV)\n DC A(ONLYV)\n END\n");
    CHECK(run(&f) == MF_UNDEFINED); clean(&f);
    consumer_init(&f, " ENTRY LOCAL\nLOCAL DC F'1'\n DC V(LOCAL)\n END LOCAL\n");
    real_writer(&f); CHECK(run(&f) == MF_OK && f.sink_valid);
    /* PC, ER LOCAL, LD LOCAL, TXT, V RLD, END. */
    CHECK(f.deck_length == 480 && f.deck[80+24] == 2 && f.deck[160+24] == 1);
    CHECK(!memcmp(f.deck+80+16, f.deck+160+16, 8));
    CHECK(f.deck[320+20] == 0x1c); clean(&f);
}
static void symbolic_ss(void)
{
    static const mf_octet expected[] = {
        0xd7,0x13,0xc0,0x14,0xc0,0x14,
        0xd2,0x07,0xc0,0x1a,0x30,0,
        0xf8,0x31,0xc0,0x14,0xc0,0x18};
    struct fixture f;
    consumer_init(&f, "S CSECT\n USING S,12\n XC AREA(20),AREA\n MVC AREA+6(8),0(3)\n ZAP AREA(4),AREA+4(2)\nAREA DS 5F\n END S\n");
    successful(&f); CHECK(f.sections[0].length == 40 && !memcmp(f.data[0], expected, sizeof expected)); clean(&f);
}
static void consumer_failures(void)
{
    static const struct bad_case cases[] = {
        {"S CSECT\n USING S,12\n L 1,=H'1'\n END\n",MF_UNSUPPORTED},
        {"S CSECT\n USING S,12\n L 1,=2F'1'\n END\n",MF_UNSUPPORTED},
        {"S CSECT\n USING S,12\n L 1,=XL4'01'\n END\n",MF_UNSUPPORTED},
        {"S CSECT\n USING S,12\n L 1,=F'2147483648'\n END\n",MF_RANGE},
        {"S CSECT\n USING S,12\n L 1,=F'-2147483649'\n END\n",MF_RANGE},
        {"S CSECT\n USING S,12\n L 1,=F'NAME'\n END\n",MF_SOURCE},
        {"S CSECT\n USING S,12\n L 1,=X'GG'\n END\n",MF_SOURCE},
        {"S CSECT\n USING S,12\n L 1,=X''\n END\n",MF_SOURCE},
        {"S CSECT\n USING S,12\n L 1,=V(EXT+1)\n END\n",MF_SOURCE},
        {"S CSECT\n USING S,12\n L 1,=V(1)\n END\n",MF_SOURCE},
        {" DC V(EXT)\n EXTRN EXT,EXT\n END\n",MF_DUPLICATE},
        {" L 1,=F'1'\n END\n",MF_RANGE},
        {"S CSECT\n USING S,12\n L 1,=F'1'\n DROP 12\n END\n",MF_OK},
        {"S CSECT\n USING S,12\n L 1,=F'1'\n DS 4096C\n END\n",MF_RANGE},
        {"S CSECT\n USING S,12\n L 1,=F'1'\n",MF_SOURCE},
        {"A CSECT\n USING A,12\n L 1,=F'1'\nB CSECT\n LTORG\n END A\n",MF_RANGE},
        {"S CSECT\n LTORG ARG\n END\n",MF_SOURCE},
        {"D DSECT\n LTORG\n END\n",MF_SOURCE},
        {"S CSECT\n USING S,12\n B\n END\n",MF_SOURCE},
        {" B 1,2\n END\n",MF_UNSUPPORTED},
        {" BR 16\n END\n",MF_RANGE},
        {" BR EXT\n END\n",MF_UNDEFINED},
        {"S CSECT\n B S\n END\n",MF_RANGE},
        {"S CSECT\n XC S(-1),S\n END\n",MF_RANGE}
    };
    struct fixture f; size_t i;
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        consumer_init(&f, cases[i].source);
        if (cases[i].status == MF_OK) successful(&f); else failed(&f, cases[i].status);
        clean(&f);
    }
    consumer_init(&f, "S CSECT\n USING S,12\n L 1,=F'1'\n L 2,=F'2'\n END S\n"); f.config.max_literals = 1;
    failed(&f, MF_LIMIT); CHECK(f.begins == 0); clean(&f);
    consumer_init(&f, "S CSECT\n USING S,12\n L 1,=F'1'\n LTORG\n L 2,=F'1'\n END S\n"); f.config.max_literals = 1;
    failed(&f, MF_LIMIT); clean(&f);
    consumer_init(&f, "S CSECT\n USING S,12\n L 1,=F'1'\n END S\n"); f.config.max_literals = 0;
    failed(&f, MF_UNSUPPORTED); clean(&f);
    consumer_init(&f, "S CSECT\n USING S,12\n L 1,=V(EXT)\n END S\n"); f.config.max_symbols = 1;
    failed(&f, MF_LIMIT); clean(&f);
    consumer_init(&f, "S CSECT\n USING S,12\n L 1,=V(EXT)\n END S\n"); f.config.max_fixups = 0;
    failed(&f, MF_LIMIT); clean(&f);
    consumer_init(&f, "S CSECT\n USING S,12\n L 1,=F'1'\n END S\n"); f.allocation_fail = 6;
    failed(&f, MF_LIMIT); CHECK(f.as == NULL); clean(&f);
    consumer_init(&f, "S CSECT\n USING S,12\n L 1,=F'1'\n END S\n"); f.allocation_fail = 9;
    failed(&f, MF_LIMIT); CHECK(f.begins == 0); clean(&f);
    consumer_init(&f, "S CSECT\n END S\n"); f.config.max_literals = (size_t)-1;
    failed(&f, MF_LIMIT); CHECK(f.allocation_calls == 0); clean(&f);    consumer_init(&f, "S CSECT\n USING S,12\n L 1,=X'0123456789ABCDEF0123456789ABCDEF'\n END S\n");
    f.config.max_statement = 16; failed(&f, MF_LIMIT); CHECK(f.begins == 0); clean(&f);
}

static void literal_replay_and_writer_errors(void)
{
    static const char *changed[] = {
        "S CSECT\n USING S,12\n L 1,=F'2'\n LTORG\n END S\n",
        "S CSECT\n USING S,12\n L 1,=F'1'\n END S\n",
        "S CSECT\n USING S,12\n L 1,=F'1'\n DC F'0'\n LTORG\n END S\n",
        "S CSECT\n USING S,12\n L 1,=F'1'\n LTORG\n L 2,=F'1'\n END S\n"};
    struct fixture f; size_t i;
    for (i = 0; i < sizeof changed / sizeof changed[0]; ++i) {
        consumer_init(&f, "S CSECT\n USING S,12\n L 1,=F'1'\n LTORG\n END S\n"); f.second = changed[i];
        failed(&f, MF_REPLAY); CHECK(f.invalids == 1); clean(&f);
    }
    consumer_init(&f, "S CSECT\n USING S,12\n L 1,=V(EXT)\n LTORG\n END S\n"); f.fail_fixup = 1;
    failed(&f, MF_IO); CHECK(f.invalids == 1); clean(&f);
    consumer_init(&f, "S CSECT\n USING S,12\n L 1,=X'01'\n LTORG\n END S\n"); f.fail_gap = 1;
    failed(&f, MF_IO); CHECK(f.invalids == 1); clean(&f);
}
static void literal_sink_contract(void)
{
    struct fixture f; int failure;
    for (failure = 0; failure < 8; ++failure) {
        consumer_init(&f, "S CSECT\n USING S,12\n L 1,=V(EXT)\n END S\n"); real_writer(&f);
        if (failure == 0) f.sink_fail_begin = 1;
        if (failure > 0 && failure < 7) f.sink_fail_write = (unsigned)failure;
        if (failure == 7) f.sink_fail_finish = 1;
        CHECK(run(&f) == MF_IO); CHECK(!f.result.valid_output && !f.sink_valid);
        CHECK(f.reports == 1 && f.sink_finishes >= 1); clean(&f);
    }
    consumer_init(&f, "S CSECT\n USING S,12\n L 1,=V(EXT)\n END S\n"); real_writer(&f);
    CHECK(run(&f) == MF_OK && f.result.valid_output && f.sink_valid);
    CHECK(f.deck_length == 480); clean(&f);
    /* Decimal spelling differences intentionally remain separate identities. */
    consumer_init(&f, "S CSECT\n USING S,12\n L 1,=F'1'\n L 2,=F'01'\n END S\n");
    successful(&f); CHECK(f.sections[0].length == 16 && f.texts == 4);
    CHECK(f.data[0][3] == 8 && f.data[0][7] == 12); clean(&f);
}
static void repeated_literal_storage(void)
{
    struct fixture a, b; char source[4096]; size_t i;
    consumer_init(&a, "S CSECT\n USING S,12\n L 1,=F'1'\n END S\n"); successful(&a);
    strcpy(source, "S CSECT\n USING S,12\n");
    for (i = 0; i < 200; ++i) strcat(source, " L 1,=F'1'\n");
    strcat(source, " END S\n"); consumer_init(&b, source); successful(&b);
    CHECK(a.used == b.used && a.allocation_count == b.allocation_count);
    CHECK(a.result.storage_requested == b.result.storage_requested);
    CHECK(b.sections[0].length == 804 && b.texts == 201); clean(&a); clean(&b);
}
int main(void)
{
    aliases(); address_literal(); literal_bytes(); pool_grouping(); multiple_pools_and_sections();
    implicit_externals(); local_v_names(); symbolic_ss(); consumer_failures();
    literal_replay_and_writer_errors(); literal_sink_contract(); repeated_literal_storage();
    printf("consumer engine: %lu checks passed\n", checks); return 0;
}
