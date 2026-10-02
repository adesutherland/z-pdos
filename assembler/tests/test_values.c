/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Original numeric expectations; no native execution-charset assumption.
 */
#include "mf_classic.h"
#define CHECK(x) do { if (!(x)) return __LINE__; } while (0)
static struct mf_span span(const mf_octet *p, size_t n)
{
    struct mf_span s;
    s.data = p; s.length = n; return s;
}
int main(void)
{
    static const mf_octet maximum[] = {
        0x31,0x38,0x34,0x34,0x36,0x37,0x34,0x34,0x30,0x37,
        0x33,0x37,0x30,0x39,0x35,0x35,0x31,0x36,0x31,0x35 };
    static const mf_octet excess[] = {
        0x31,0x38,0x34,0x34,0x36,0x37,0x34,0x34,0x30,0x37,
        0x33,0x37,0x30,0x39,0x35,0x35,0x31,0x36,0x31,0x36 };
    static const mf_octet negative[] = {0x2d,0x34,0x32};
    static const mf_octet malformed[] = {0x31,0x78};
    static const mf_octet plus[] = {0x2b};
    static const mf_octet signedmin[] = {
        0x2d,0x39,0x32,0x32,0x33,0x33,0x37,0x32,0x30,0x33,
        0x36,0x38,0x35,0x34,0x37,0x37,0x35,0x38,0x30,0x38};
    static const mf_octet bigendian[] = {0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef};
    struct mf_u64 value, a, b, result;
    mf_octet output[8], encoded, decoded;
    unsigned i, j;
    int sign;
    CHECK(mf_u64_parse(span(maximum,sizeof(maximum)), &value,&sign) == MF_OK);
    CHECK(value.hi == 0xffffffffUL && value.lo == 0xffffffffUL && !sign);
    result.hi = 7; result.lo = 9;
    CHECK(mf_u64_parse(span(excess,sizeof(excess)), &result,&sign) == MF_RANGE);
    CHECK(result.hi == 7 && result.lo == 9);
    CHECK(mf_u64_parse(span(negative,sizeof(negative)), &value,&sign) == MF_OK);
    CHECK(value.hi == 0 && value.lo == 42 && sign);
    mf_u64_negate(value,&value);
    CHECK(value.hi == 0xffffffffUL && value.lo == 0xffffffd6UL);
    mf_u64_negate(value,&value);
    CHECK(value.hi == 0 && value.lo == 42);
    CHECK(mf_u64_parse(span(malformed,sizeof(malformed)), &value,&sign) == MF_SOURCE);
    CHECK(mf_u64_parse(span(plus,sizeof(plus)), &value,&sign) == MF_SOURCE);
    CHECK(mf_u64_parse(span(maximum,0), &value,&sign) == MF_SOURCE);
    CHECK(mf_u64_parse(span(signedmin,sizeof(signedmin)), &value,&sign) == MF_OK);
    CHECK(value.hi == 0x80000000UL && value.lo == 0 && sign);
    mf_u64_negate(value,&result);
    CHECK(result.hi == 0x80000000UL && result.lo == 0);
    a.hi = 5; a.lo = 0xffffffffUL; b.hi = 2; b.lo = 1;
    CHECK(mf_u64_add(a,b,&result) == MF_OK);
    CHECK(result.hi == 8 && result.lo == 0);
    a.hi = a.lo = 0xffffffffUL;
    CHECK(mf_u64_add(a,b,&result) == MF_RANGE);
#if ULONG_MAX > 0xffffffffUL
    a.hi = 0; a.lo = 0xffffffffUL + 1UL;
    CHECK(mf_u64_add(a,b,&result) == MF_RANGE);
#endif
    value.hi = 0x01234567UL; value.lo = 0x89abcdefUL;
    for (i = 1; i <= 8; ++i) {
        for (j = 0; j < 8; ++j) output[j] = 0x55;
        mf_u64_store(value,output,i);
        CHECK(mf_span_equal(span(output,i),span(bigendian + 8-i,i)));
        if (i < 8) CHECK(output[i] == 0x55);
    }
    for (i = 0; i < 128; ++i) {
        CHECK(mf_ascii_to_ebcdic((mf_octet)i,&encoded) == MF_OK);
        CHECK(mf_ebcdic_to_ascii(encoded,&decoded) == MF_OK && decoded == i);
    }
    CHECK(mf_ascii_to_ebcdic(0x41,&encoded) == MF_OK && encoded == 0xc1);
    CHECK(mf_ascii_to_ebcdic(0x30,&encoded) == MF_OK && encoded == 0xf0);
    CHECK(mf_ascii_to_ebcdic(0x27,&encoded) == MF_OK && encoded == 0x7d);
    CHECK(mf_ascii_to_ebcdic(0x5b,&encoded) == MF_OK && encoded == 0xba);
    CHECK(mf_ascii_to_ebcdic(0x80,&encoded) == MF_UNSUPPORTED);
    CHECK(mf_ebcdic_to_ascii(0xff,&decoded) == MF_UNSUPPORTED);
    CHECK(!mf_span_equal(span(maximum,20),span(excess,20)));
    return 0;
}
