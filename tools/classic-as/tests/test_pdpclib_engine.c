/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Original bounded PDPCLIB-language vectors; no macro implementations.
 * Explicit SS0/1: IBM OS Assembler GC28-6514-9, Jan1974, printed p30.
 * Zero DC: IBM HLASM V1R6, Subfield1: Duplication Factor.
 */
#define main mf_bootstrap_engine_test_entry
#include "test_assemble.c"
#undef main
static void pdp_successful(struct fixture *f)
{
    enum mf_status status; status = run(f);
    if (status != MF_OK) fprintf(stderr, "PDPCLIB expected success, got %d for %s\n", (int)status, f->first);
    CHECK(status == MF_OK && f->result.status == MF_OK);
    CHECK(f->result.valid_output && f->valid && f->reports == 0);
    CHECK(f->begins == 1 && f->finishes == 1 && f->invalids == 0);
    CHECK(f->result.storage_requested == f->used);
}
static void ss_lengths(void)
{
    static const mf_octet expected[] = {
        0xd2,0,0x10,0,0x20,0,0xd2,0,0x10,0,0x20,0,
        0xd2,1,0x10,0,0x20,0,0xd5,0xff,0x3f,0xff,0x4f,0xff,
        0xf8,0,0x10,0,0x20,0,0xf8,0,0x10,0,0x20,0,
        0xf8,0xff,0x10,0,0x20,0,
        0xfc,0x70,0x10,0,0x20,0,0xfc,0x70,0x10,0,0x20,0,
        0xfd,0x70,0x10,0,0x20,0,0xfd,0x70,0x10,0,0x20,0};
    struct fixture f;
    init(&f, "S CSECT\n MVC 0(0,1),0(2)\n MVC 0(1,1),0(2)\n MVC 0(2,1),0(2)\n CLC 4095(256,3),4095(4)\n ZAP 0(0,1),0(0,2)\n ZAP 0(1,1),0(1,2)\n ZAP 0(16,1),0(16,2)\n MP 0(8,1),0(0,2)\n MP 0(8,1),0(1,2)\n DP 0(8,1),0(0,2)\n DP 0(8,1),0(1,2)\n END S\n");
    pdp_successful(&f); CHECK(f.sections[0].length == sizeof expected);
    CHECK(!memcmp(f.data[0], expected, sizeof expected)); clean(&f);
    init(&f, "S CSECT\n USING S,12\n XC AREA(0),AREA\n MVC AREA(1),AREA\nAREA DS F\n END S\n");
    pdp_successful(&f); CHECK(f.data[0][1] == 0 && f.data[0][7] == 0);
    CHECK(f.data[0][2] == 0xc0 && f.data[0][3] == 12 && f.data[0][9] == 12); clean(&f);
}
static void pure_encoder_and_zero_storage(void)
{
    static const mf_octet mvc[] = {0x4d,0x56,0x43};
    struct mf_span name; struct mf_operands op; const struct mf_instruction *ins;
    mf_octet bytes[6]; unsigned length; struct fixture a, b; char source[4096]; size_t i;
    name.data = mvc; name.length = sizeof mvc; ins = mf_machine_lookup(MF_S360, name);
    CHECK(ins != NULL); memset(&op, 0, sizeof op);
    CHECK(mf_encode(MF_S360, ins, &op, bytes, sizeof bytes, &length) == MF_RANGE);
    op.length1 = 1; CHECK(mf_encode(MF_S360, ins, &op, bytes, sizeof bytes, &length) == MF_OK);
    CHECK(length == 6 && bytes[0] == 0xd2 && bytes[1] == 0);
    init(&a, " CSECT\n DC 0V(NOTDECLARED)\n END\n"); pdp_successful(&a);
    strcpy(source, " CSECT\n");
    for (i = 0; i < 200; ++i) strcat(source, " DC 0V(NOTDECLARED)\n");
    strcat(source, " END\n"); init(&b, source); pdp_successful(&b);
    CHECK(a.used == b.used && a.allocation_count == b.allocation_count);
    CHECK(a.result.storage_requested == b.result.storage_requested && b.symbol_count == 0 && b.fixup_count == 0);
    CHECK(b.sections[0].length == 0 && b.texts == 0 && b.result.statements == 202); clean(&a); clean(&b);
    init(&a, "S CSECT\n USING S,12\n L 1,=0F'1'\n END S\n"); a.config.max_literals = 4;
    failed(&a, MF_UNSUPPORTED); clean(&a);
}
static void unnamed_sections(void)
{
    struct fixture f; size_t entry_symbol;
    init(&f, " CSECT\n AMODE 31\n RMODE ANY\n ENTRY START,AFTER\nSTART LR 1,2\nD DSECT\nFIELD DS 3F\n CSECT\nAFTER LR 3,4\n END START\n");
    pdp_successful(&f); CHECK(f.section_count == 2 && !f.sections[0].name.length && !f.sections[0].dummy);
    CHECK(f.sections[0].length == 4 && f.sections[0].amode == 31 && f.sections[0].rmode == 31);
    CHECK(f.sections[1].dummy && f.sections[1].length == 12);
    CHECK(f.data[0][0] == 0x18 && f.data[0][1] == 0x12 && f.data[0][2] == 0x18 && f.data[0][3] == 0x34);
    entry_symbol = symbol_index(&f, "AFTER"); CHECK(f.symbols[entry_symbol].kind == MF_EXPORT);
    CHECK(f.symbols[entry_symbol].section == 1 && f.symbols[entry_symbol].offset == 2);
    CHECK(f.entry.present && f.entry.section == 1 && f.entry.offset == 0); clean(&f);
    /* Restore an implicit PC identity without creating a second unnamed ESD. */
    init(&f, "START LR 1,2\nD DSECT\nFIELD DS F\n CSECT\n LR 3,4\nN CSECT\n LR 5,6\n CSECT\n LR 7,8\n END START\n");
    pdp_successful(&f); CHECK(f.section_count == 3 && f.sections[0].length == 6 && f.sections[2].length == 2);
    CHECK(f.data[0][4] == 0x18 && f.data[0][5] == 0x78 && f.entry.section == 1); clean(&f);
    /* The first unnamed section can be introduced after an existing DSECT. */
    init(&f, "D DSECT\nFIELD DS H\n CSECT\nSTART DC F'1'\nD DSECT\n DS F\n CSECT\n DC H'2'\n END START\n");
    pdp_successful(&f); CHECK(f.section_count == 2 && f.sections[0].dummy && f.sections[0].length == 8);
    CHECK(!f.sections[1].name.length && f.sections[1].length == 6 && f.entry.section == 2); clean(&f);
    init(&f, " CSECT\nD DSECT\n CSECT\n CSECT\n END\n");
    pdp_successful(&f); CHECK(f.section_count == 2 && f.sections[0].length == 0 && f.sections[1].length == 0); clean(&f);
}
static void zero_constants(void)
{
    struct fixture f;
    init(&f, "S CSECT\n DC X'11'\nH DC 0H\n DC X'22'\nF DC 0F'FORWARD'\nA DC 0A(MISSING+1)\nV DC 0V(UNDECLARED)\nAD DC 0AD\nC DC 0CL4\nX DC 0XL2\nH2 DS 0H\nF2 DS 0F\nD2 DS 0D\nFORWARD DC F'7'\n END S\n");
    pdp_successful(&f); CHECK(f.sections[0].length == 12 && f.fixup_count == 0 && f.result.fixups == 0);
    CHECK(f.data[0][0] == 0x11 && f.data[0][2] == 0x22 && f.data[0][11] == 7);
    CHECK(f.texts == 3 && f.gaps == 3 && f.emitted == 6);
    CHECK(f.symbols[symbol_index(&f, "H")].offset == 2 && f.symbols[symbol_index(&f, "F")].offset == 4);
    CHECK(f.symbols[symbol_index(&f, "A")].offset == 4 && f.symbols[symbol_index(&f, "V")].offset == 4);
    CHECK(f.symbols[symbol_index(&f, "AD")].offset == 8 && f.symbols[symbol_index(&f, "C")].offset == 8);
    CHECK(f.symbols[symbol_index(&f, "X")].offset == 8 && f.symbols[symbol_index(&f, "H2")].offset == 8);
    CHECK(f.symbols[symbol_index(&f, "F2")].offset == 8 && f.symbols[symbol_index(&f, "D2")].offset == 8);
    CHECK(f.symbol_count == 12); /* Nominal-only names never enter the symbol table. */
    clean(&f);
    init(&f, "S CSECT\nZ DC 0H'-32769',0F'2147483648'\n DC 0AD(-9223372036854775809)\n DC 0A(S+S),0V(EXT+1)\n DC F'1'\n END S\n");
    pdp_successful(&f); CHECK(f.sections[0].length == 4 && f.fixup_count == 0 && f.symbol_count == 2);
    CHECK(f.symbols[symbol_index(&f, "Z")].offset == 0); clean(&f);
    /* Validate nominal grammar without computing unused arithmetic results. */
    init(&f, " CSECT\nZ DC 0A(18446744073709551615+1),0F'-18446744073709551615-1'\n DC 0AD((18446744073709551615+1)-(18446744073709551615+1))\n DC F'7'\n END Z\n");
    pdp_successful(&f); CHECK(f.sections[0].length == 4 && f.fixup_count == 0 && f.symbol_count == 1);
    CHECK(f.symbols[symbol_index(&f, "Z")].offset == 0 && f.data[0][3] == 7); clean(&f);
    init(&f, "S CSECT\n DC 0F,0A,0V,0AD,0H,0C'AB',0X'ABC',0CL4'AB',0XL4'ABC'\n DC X'7F'\n END S\n");
    pdp_successful(&f); CHECK(f.sections[0].length == 1 && f.fixup_count == 0 && f.texts == 1 && f.gaps == 0); clean(&f);
    /* No implicit external side effect: an ordinary later label is allowed. */
    init(&f, "S CSECT\n DC 0V(LATER)\nLATER DC F'9'\n END LATER\n");
    pdp_successful(&f); CHECK(f.symbol_count == 2 && f.symbols[symbol_index(&f, "LATER")].kind == MF_LOCAL && f.fixup_count == 0); clean(&f);
    init(&f, "D DSECT\n DC X'00'\nFIELD DC 0AD(MISSING)\n CSECT\n LR 1,2\n END\n");
    pdp_successful(&f); CHECK(f.sections[0].length == 8 && f.symbols[symbol_index(&f, "FIELD")].offset == 8);
    CHECK(f.sections[1].length == 2 && f.fixup_count == 0); clean(&f);
}
static void pdp_failures(void)
{
    static const struct bad_case cases[] = {
        {" MVC 0(-1,1),0(2)\n END\n",MF_RANGE},
        {" MVC 0(257,1),0(2)\n END\n",MF_RANGE},
        {" ZAP 0(17,1),0(1,2)\n END\n",MF_RANGE},
        {" ZAP 0(0,1),0(-1,2)\n END\n",MF_RANGE},
        {" MVC 0(,1),0(2)\n END\n",MF_SOURCE},
        {" DSECT\n END\n",MF_SOURCE},
        {" CSECT BAD\n END\n",MF_SOURCE},
        {"S EQU 1\nS CSECT\n END\n",MF_DUPLICATE},
        {" CSECT\nS DC F'1'\nS CSECT\n END\n",MF_DUPLICATE},
        {"S CSECT\nS DSECT\n END\n",MF_DUPLICATE},
        {" DC 0F''\n END\n",MF_SOURCE},
        {" DC 0F'+'\n END\n",MF_SOURCE},
        {" DC 0F'1+'\n END\n",MF_SOURCE},
        {" DC 0A()\n END\n",MF_SOURCE},
        {" DC 0V()\n END\n",MF_SOURCE},
        {" DC 0A(MISSING+)\n END\n",MF_SOURCE},
        {" DC 0A(MISSING*2)\n END\n",MF_UNSUPPORTED},
        {" DC 0F'1/2'\n END\n",MF_UNSUPPORTED},
        {" DC 0X'GG'\n END\n",MF_SOURCE},
        {" DC 0X''\n END\n",MF_SOURCE},
        {" DC 0CL1'AB'\n END\n",MF_RANGE},
        {" DC 0XL1'ABC'\n END\n",MF_RANGE},
        {" DC 0C\n END\n",MF_UNSUPPORTED},
        {" DC 0X\n END\n",MF_UNSUPPORTED},
        {" DC 0D\n END\n",MF_UNSUPPORTED},
        {" DC 0FL4\n END\n",MF_UNSUPPORTED},
        {" DC 0F'1',\n END\n",MF_SOURCE},
        {" DC 4294967296F'1'\n END\n",MF_RANGE},
        {" DC 0A(18446744073709551616)\n END\n",MF_RANGE},
        {" DC A(18446744073709551615+1)\n END\n",MF_RANGE},
        {" DC F'-18446744073709551615-1'\n END\n",MF_RANGE}
    };
    struct fixture f; size_t i;
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        init(&f, cases[i].source); failed(&f, cases[i].status); clean(&f);
    }
    init(&f, "D DSECT\n CSECT\n END\n"); f.config.max_sections = 1;
    failed(&f, MF_LIMIT); clean(&f);
    init(&f, " CSECT\nSTART LR 1,2\n END START\n"); f.config.max_symbols = 1;
    pdp_successful(&f); CHECK(f.symbol_count == 1); clean(&f);
    init(&f, " CSECT\nSTART DC 0V(EXT)\n DC F'1'\n END START\n"); f.config.max_symbols = 1; f.config.max_fixups = 0;
    pdp_successful(&f); CHECK(f.symbol_count == 1 && f.fixup_count == 0); clean(&f);
    init(&f, " DC 0A((((MISSING))))\n DC F'1'\n END\n"); f.config.max_expression_depth = 3;
    failed(&f, MF_LIMIT); clean(&f);
    init(&f, "FIELD DC 0F'1'\n DC F'1'\n END\n"); f.config.max_statement = 4;
    failed(&f, MF_LIMIT); clean(&f);
    init(&f, " CSECT\nSTART DC 0F\n DC F'1'\n END START\n"); f.allocation_fail = 6;
    failed(&f, MF_LIMIT); CHECK(f.begins == 0); clean(&f);
}
static void replay_and_real_object(void)
{
    struct fixture f;
    init(&f, " CSECT\nSTART LR 1,2\nD DSECT\n DS F\n CSECT\n LR 3,4\n END START\n");
    f.second = " CSECT\nSTART LR 1,2\nD DSECT\n DS F\nX CSECT\n LR 3,4\n END START\n";
    failed(&f, MF_REPLAY); CHECK(f.invalids == 1); clean(&f);
    init(&f, "S CSECT\nZ DC 0V(EXT)\n DC F'1'\n END Z\n");
    f.second = "S CSECT\nZ DC 0V(OTHER)\n DC F'1'\n END Z\n";
    failed(&f, MF_REPLAY); CHECK(f.invalids == 1); clean(&f);
    init(&f, " CSECT\n ENTRY START\nSTART DC X'01'\nALIGN DC 0F\n DC F'7'\nD DSECT\nFIELD DS H\n CSECT\n LR 1,2\n END START\n");
    real_writer(&f); CHECK(run(&f) == MF_OK && f.result.valid_output && f.sink_valid);
    CHECK(f.result.sections == 2 && f.result.fixups == 0);
    CHECK(f.deck[24] == 4); /* Unnamed PC ESD, rather than a truncated name. */
    CHECK(f.deck_length == 400); clean(&f);
    init(&f, " CSECT\n DC X'01'\nALIGN DC 0AD\n DC F'7'\n END\n"); f.fail_gap = 1;
    failed(&f, MF_IO); CHECK(f.invalids == 1); clean(&f);
}
int main(void)
{
    ss_lengths(); pure_encoder_and_zero_storage(); unnamed_sections(); zero_constants(); pdp_failures(); replay_and_real_object();
    printf("PDPCLIB engine: %lu checks passed\n", checks); return 0;
}
