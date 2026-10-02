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
static void label_attributes(void)
{
    struct fixture f; unsigned pass; unsigned long n; size_t allocated;
    init(&f," MACRO\n&LABEL DESCRIBE\n LCLA &SIZE\n&SIZE SETA K'&LABEL\n"
        " DC AL1(&SIZE),AL1(N'&LABEL)\n AIF (T'&LABEL EQ 'O').EMPTY\n"
        " DC C'&LABEL'\n.EMPTY ANOP\n MEND\nFIVE5 DESCRIBE\n DESCRIBE\n");
    CHECK(create(&f) == MF_OK); allocated = f.calls;
    for (pass = 0; pass < 2; ++pass) {
        expected(&f,"","DC","AL1(5),AL1(1)",10);
        expected(&f,"","DC","C'FIVE5'",10);
        expected(&f,"","DC","AL1(0),AL1(0)",11);
        CHECK(drain(&f,&n) == MF_EOF && f.calls == allocated);
        if (!pass) CHECK(f.statements.replay(f.statements.cookie) == MF_OK);
    }
    clean(&f);
}
static void variable_counts(void)
{
    struct fixture f; unsigned long n;
    init(&f," GBLC &G\n&G SETC 'A''B'\n LCLA &I\n&I SETA -12\n"
        " MACRO\n COUNT\n GBLC &G\n LCLC &C,&EMPTY\n&C SETC '(R10)'\n"
        " DC AL1(K'&C),AL1(K'&G),AL1(K'&EMPTY),AL1(K'&SYSNDX)\n MEND\n COUNT\n DC AL1(K'&I)\n");
    CHECK(create(&f) == MF_OK);
    expected(&f,"","DC","AL1(5),AL1(3),AL1(0),AL1(4)",12);
    expected(&f,"","DC","AL1(3)",13);
    CHECK(drain(&f,&n) == MF_EOF); clean(&f);
    bad_source(" LCLC &C\n DC AL1(N'&C)\n",MF_UNSUPPORTED);
    bad_source(" DC AL1(K'&UNDEFINED)\n",MF_UNDEFINED);
}
static void substrings(void)
{
    struct fixture f; unsigned long n;
    init(&f," LCLC &C\n&C SETC 'ABCDE'(2,3)\n"
        " AIF ('AB''CD'(2,3) EQ 'B''C' AND 'ABCDE'(4,*) EQ 'DE').YES\n"
        " BAD &UNDEF\n.YES DC C'&C'\n&C SETC 'ABC'(4,0)\n DC C'X&C.Y'\n");
    CHECK(create(&f) == MF_OK);
    expected(&f,"","DC","C'BCD'",5);
    expected(&f,"","DC","C'XY'",7);
    CHECK(drain(&f,&n) == MF_EOF); clean(&f);
    bad_source(" LCLC &C\n&C SETC 'ABC'(0,1)\n",MF_RANGE);
    bad_source(" LCLC &C\n&C SETC 'ABC'(2,-1)\n",MF_RANGE);
    bad_source(" LCLC &C\n&C SETC 'ABC'(3,2)\n",MF_RANGE);
    bad_source(" AIF ('ABC'('x',1) EQ 'A').YES\n.YES ANOP\n",MF_SOURCE);
}
static void character_concatenation(void)
{
    struct fixture f; unsigned pass; unsigned long n; size_t allocated;
    init(&f," LCLC &C\n&C SETC 'MAP'.'ZP'\n DC C'&C'\n&C SETC 'A.B'.'XYZ'(2,*)\n DC C'&C'\n&C SETC 'A''B'.''.'C'\n DC AL1(K'&C)\n");
    CHECK(create(&f) == MF_OK); allocated = f.calls;
    for (pass = 0; pass < 2; ++pass) {
        expected(&f,"","DC","C'MAPZP'",3);
        expected(&f,"","DC","C'A.BYZ'",5);
        expected(&f,"","DC","AL1(4)",7);
        CHECK(drain(&f,&n) == MF_EOF && f.calls == allocated);
        if (!pass) CHECK(f.statements.replay(f.statements.cookie) == MF_OK);
    }
    clean(&f);
    bad_source(" LCLC &C\n&C SETC 'A'..'B'\n",MF_SOURCE);
    bad_source(" LCLC &C\n&C SETC 'A'.\n",MF_SOURCE);
    bad_source(" LCLC &C\n&C SETC 'A'.1\n",MF_SOURCE);
    bad_source(" LCLC &C\n&C SETC 'A'.'B\n",MF_SOURCE);
    init(&f," LCLC &C\n&C SETC 'AB'.'CD'\n"); f.config.max_statement_bytes = 3;
    CHECK(create(&f) == MF_OK); CHECK(drain(&f,&n) == MF_LIMIT); clean(&f);
}
static void inactive_source(void)
{
    struct fixture f; unsigned pass; unsigned long n;
    init(&f," AGO .DONE\n BAD C'unclosed\n MACRO\n UNUSED\n.DONE BAD C'unclosed\n MEND\n.DONE LR 1,2\n");
    CHECK(create(&f) == MF_OK);
    for (pass = 0; pass < 2; ++pass) {
        expected(&f,"","LR","1,2",7); CHECK(drain(&f,&n) == MF_EOF);
        if (!pass) CHECK(f.statements.replay(f.statements.cookie) == MF_OK);
    }
    clean(&f);
    init(&f," AGO .DONE\n BAD C'unclosed\n.DONE LR 1,2\n");
    f.second = " AGO .DONE\n BAD C'changed\n.DONE LR 1,2\n";
    CHECK(create(&f) == MF_OK); CHECK(drain(&f,&n) == MF_EOF);
    CHECK(f.statements.replay(f.statements.cookie) == MF_OK); CHECK(drain(&f,&n) == MF_REPLAY); clean(&f);
    bad_source(" AGO .DONE\n.DONE DC C'unclosed\n",MF_SOURCE);
    bad_source(" AGO .DONE\n MACRO\n UNUSED\n.DONE ANOP\n MEND\n",MF_UNDEFINED);
}
static void end_targets(void)
{
    struct fixture f; unsigned pass; unsigned long n;
    init(&f," MACRO\n EARLY &STOP=YES\n AIF ('&STOP' EQ 'YES').EXIT\n LR 3,4\n.EXIT MEND\n EARLY\n EARLY STOP=NO\n LR 1,2\n");
    CHECK(create(&f) == MF_OK);
    for (pass = 0; pass < 2; ++pass) {
        expected(&f,"","LR","3,4",7);
        expected(&f,"","LR","1,2",8);
        CHECK(drain(&f,&n) == MF_EOF);
        if (!pass) CHECK(f.statements.replay(f.statements.cookie) == MF_OK);
    }
    clean(&f);
    init(&f," MACRO\n INNER\n AGO .END\n.END MEND\n MACRO\n OUTER\n INNER\n LR 5,6\n.END MEND\n OUTER\n");
    CHECK(create(&f) == MF_OK);
    expected(&f,"","LR","5,6",10);
    CHECK(drain(&f,&n) == MF_EOF); clean(&f);
    bad_source(" MACRO\n BADEND\n.X ANOP\n.X MEND\n BADEND\n",MF_DUPLICATE);
    bad_source(" MACRO\n BADEND\n AGO .MISSING\n.END MEND\n BADEND\n",MF_UNDEFINED);
}
int main(void)
{
    character_concatenation(); end_targets(); inactive_source(); scoped(); expressions(); argument_attributes(); label_attributes(); variable_counts(); substrings(); printf("conditional macros: %lu checks passed\n", checks); return 0;
}
