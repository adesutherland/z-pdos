/* SPDX-License-Identifier: MIT
 * Original scalar/branch consumers, including deliberately inactive errors.
 */
#define main existing_macro_tests
#include "test_macro.c"
#undef main
static void scoped(void)
{
    struct fixture f; unsigned pass; unsigned long n; size_t allocated;
    init(&f, "");
    f.first = f.source =
        " GBLA &TOTAL\n GBLC &SELECT\n&SELECT SETC 'ON'\n"
        " MACRO\n&N COUNT &FRAME=0\n GBLB &ONCE\n GBLA &TOTAL\n LCLA &I\n"
        "&I SETA 0\n&ONCE SETB 1\n GBLC &SELECT\n"
        ".LOOP ANOP\n&I SETA &I+1\n&TOTAL SETA &TOTAL+1\n"
        " AIF (&I LT 2).LOOP\n AIF ('&SELECT' EQ 'ON' AND &ONCE).ENABLED\n"
        " BAD &UNDEFINED\n.ENABLED AIF (T'&FRAME NE 'N').SYMBOL\n"
        "&N LA 15,&FRAME.(,15)\n AGO .DONE\n.SYMBOL ANOP\n&N L 15,=A(&FRAME)\n"
        ".DONE ANOP\nTAG&SYSNDX DC F'&TOTAL'\n MEXIT\n BAD &UNDEFINED\n MEND\n"
        "FIRST COUNT FRAME=112\nSECOND COUNT FRAME=AREA\n";
    CHECK(create(&f) == MF_OK); allocated = f.calls;
    for (pass = 0; pass < 2; ++pass) {
        expected(&f, "FIRST", "LA", "15,112(,15)", 28);
        expected(&f, "TAG0001", "DC", "F'2'", 28);
        expected(&f, "SECOND", "L", "15,=A(AREA)", 29);
        expected(&f, "TAG0002", "DC", "F'4'", 29);
        CHECK(drain(&f, &n) == MF_EOF && !n && f.calls == allocated);
        if (!pass) CHECK(f.statements.replay(f.statements.cookie) == MF_OK);
    }
    clean(&f);
}
static void expressions(void)
{
    struct fixture f; unsigned long n;
    init(&f,
        " LCLA &A\n LCLB &B\n LCLC &C\n&A SETA (2+3)*4-6/2\n"
        "&B SETB (NOT(0) AND 1 OR 0)\n&C SETC 'abc'\n"
        " AIF (&A EQ 17 AND &B AND 'A' EQ 'A ').YES\n BAD &UNDEF\n"
        ".YES DC F'&A'\n DC C'&C'\n");
    CHECK(create(&f) == MF_OK);
    expected(&f, "", "DC", "F'17'", 9);
    expected(&f, "", "DC", "C'abc'", 10);
    CHECK(drain(&f, &n) == MF_EOF); clean(&f);
    bad_source(" AIF (1).ABSENT\n", MF_UNDEFINED);
    bad_source(" LCLA &A\n LCLA &A\n", MF_DUPLICATE);
    bad_source(" MACRO\n CLASH &A\n LCLA &A\n MEND\n CLASH 1\n", MF_DUPLICATE);
    bad_source(" GBLA &A\n LCLA &A\n", MF_DUPLICATE);
    bad_source(" LCLA &A\n&A SETA 2147483647+1\n", MF_RANGE);
    bad_source(" LCLA &A\n&A SETA 40000*60000\n", MF_RANGE);
    bad_source(" LCLA &A\n&A SETA 1/0\n", MF_RANGE);
    bad_source(" LCLB &B\n&B SETB 2\n", MF_SOURCE);
    bad_source(" GBLA &V\n GBLB &V\n", MF_SOURCE);
    bad_source(" MACRO\n LOOP\n.X AGO .X\n MEND\n LOOP\n", MF_LIMIT);
}
static void argument_attributes(void)
{
    struct fixture f; unsigned long n;
    init(&f, " MACRO\n PARTS &R,&ID\n LCLA &N\n&N SETA K'&ID\n"
        " AIF (N'&R NE 2).BAD\n STM &R(1),&R(2),12(13)\n DC AL1(&N)\n"
        " MEXIT\n.BAD BAD &UNDEFINED\n MEND\n PARTS (14,12),HELLO\n");
    CHECK(create(&f) == MF_OK);
    expected(&f, "", "STM", "14,12,12(13)", 11);
    expected(&f, "", "DC", "AL1(5)", 11);
    CHECK(drain(&f, &n) == MF_EOF); clean(&f);
    bad_source(" LCLC &A\n&A SETC '(1,2)'\n DC F'&A(1)'\n", MF_UNSUPPORTED);
    bad_source(" MACRO\n INDEX &A\n DC F'&A(0)'\n MEND\n INDEX (1,2)\n", MF_RANGE);
    bad_source(" MNOTE 8,'unsupported form'\n", MF_SOURCE);
    bad_source(" MNOTE 4,'warning'\n", MF_UNSUPPORTED);
}
int main(void)
{
    scoped(); expressions(); argument_attributes(); printf("conditional macros: %lu checks passed\n", checks); return 0;
}
