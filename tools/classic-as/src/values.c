/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Original portable value operations. CP037 mappings below are character
 * encoding facts, written as numeric octets so the C execution charset does
 * not affect canonical text. Binary spans never undergo this conversion.
 */
#include "mf_classic.h"
#define PART_MASK 0xffffffffUL

int mf_span_equal(struct mf_span a, struct mf_span b)
{
    size_t i;
    if (a.length != b.length) return 0;
    for (i = 0; i < a.length; ++i)
        if (a.data[i] != b.data[i]) return 0;
    return 1;
}

static const mf_octet ascii_cp037[128] = {
    0x00,0x01,0x02,0x03,0x37,0x2d,0x2e,0x2f,
    0x16,0x05,0x25,0x0b,0x0c,0x0d,0x0e,0x0f,
    0x10,0x11,0x12,0x13,0x3c,0x3d,0x32,0x26,
    0x18,0x19,0x3f,0x27,0x1c,0x1d,0x1e,0x1f,
    0x40,0x5a,0x7f,0x7b,0x5b,0x6c,0x50,0x7d,
    0x4d,0x5d,0x5c,0x4e,0x6b,0x60,0x4b,0x61,
    0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,
    0xf8,0xf9,0x7a,0x5e,0x4c,0x7e,0x6e,0x6f,
    0x7c,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,
    0xc8,0xc9,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,
    0xd7,0xd8,0xd9,0xe2,0xe3,0xe4,0xe5,0xe6,
    0xe7,0xe8,0xe9,0xba,0xe0,0xbb,0xb0,0x6d,
    0x79,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
    0x88,0x89,0x91,0x92,0x93,0x94,0x95,0x96,
    0x97,0x98,0x99,0xa2,0xa3,0xa4,0xa5,0xa6,
    0xa7,0xa8,0xa9,0xc0,0x4f,0xd0,0xa1,0x07
};

enum mf_status mf_ascii_to_ebcdic(mf_octet ch, mf_octet *out)
{
    if (!out) return MF_SOURCE;
    if (ch > 127) return MF_UNSUPPORTED;
    *out = ascii_cp037[ch];
    return MF_OK;
}

enum mf_status mf_ebcdic_to_ascii(mf_octet ch, mf_octet *out)
{
    unsigned i;
    if (!out) return MF_SOURCE;
    for (i = 0; i < 128; ++i) {
        if (ascii_cp037[i] == ch) {
            *out = (mf_octet)i;
            return MF_OK;
        }
    }
    return MF_UNSUPPORTED;
}

enum mf_status mf_u64_add(struct mf_u64 a, struct mf_u64 b,
                           struct mf_u64 *out)
{
    struct mf_u64 result;
    mf_u32 carry;
    if (!out) return MF_SOURCE;
    if (a.hi > PART_MASK || a.lo > PART_MASK ||
        b.hi > PART_MASK || b.lo > PART_MASK) return MF_RANGE;
    result.lo = (a.lo + b.lo) & PART_MASK;
    carry = result.lo < a.lo ? 1UL : 0UL;
    if (a.hi > PART_MASK - b.hi) return MF_RANGE;
    result.hi = a.hi + b.hi;
    if (result.hi > PART_MASK - carry) return MF_RANGE;
    result.hi += carry;
    *out = result;
    return MF_OK;
}

enum mf_status mf_u64_parse(struct mf_span text, struct mf_u64 *out,
                             int *negative)
{
    struct mf_u64 value, twice, eight, digit;
    size_t i;
    enum mf_status status;
    int sign;
    if (!out || !negative || !text.data || !text.length) return MF_SOURCE;
    value.hi = value.lo = 0;
    sign = 0;
    i = 0;
    if (text.data[0] == 0x2b || text.data[0] == 0x2d) {
        sign = text.data[0] == 0x2d;
        ++i;
    }
    if (i == text.length) return MF_SOURCE;
    for (; i < text.length; ++i) {
        if (text.data[i] < 0x30 || text.data[i] > 0x39) return MF_SOURCE;
        status = mf_u64_add(value, value, &twice);
        if (status != MF_OK) return status;
        status = mf_u64_add(twice, twice, &eight);
        if (status != MF_OK) return status;
        status = mf_u64_add(eight, eight, &eight);
        if (status != MF_OK) return status;
        status = mf_u64_add(eight, twice, &value);
        if (status != MF_OK) return status;
        digit.hi = 0;
        digit.lo = text.data[i] - 0x30;
        status = mf_u64_add(value, digit, &value);
        if (status != MF_OK) return status;
    }
    *out = value;
    *negative = sign;
    return MF_OK;
}

void mf_u64_negate(struct mf_u64 value, struct mf_u64 *out)
{
    struct mf_u64 result;
    result.lo = ((~value.lo & PART_MASK) + 1UL) & PART_MASK;
    result.hi = ((~value.hi & PART_MASK) +
                 (result.lo == 0 ? 1UL : 0UL)) & PART_MASK;
    if (out) *out = result;
}

void mf_u64_store(struct mf_u64 value, mf_octet *out, unsigned width)
{
    unsigned i;
    if (!out || width > 8) return;
    for (i = width; i > 0; --i) {
        out[i - 1] = (mf_octet)(value.lo & 0xffUL);
        value.lo = ((value.lo >> 8) | ((value.hi & 0xffUL) << 24)) & PART_MASK;
        value.hi = (value.hi >> 8) & PART_MASK;
    }
}
