/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Original source-level contracts; no stable binary ABI is promised.
 */
#ifndef MF_CLASSIC_H
#define MF_CLASSIC_H
#include <stddef.h>
#include <limits.h>
#if CHAR_BIT != 8 || ULONG_MAX < 0xffffffffUL
#error mf_classic requires 8-bit bytes and an unsigned long of at least 32 bits
#endif

typedef unsigned char mf_octet;
typedef unsigned long mf_u32; /* values constrained to 32 bits */
struct mf_u64 { mf_u32 hi; mf_u32 lo; };
struct mf_span { const mf_octet *data; size_t length; };
struct mf_origin { unsigned source; unsigned long line; unsigned column; };
enum mf_status {
    MF_OK = 0, MF_EOF, MF_SOURCE, MF_UNSUPPORTED, MF_LIMIT, MF_IO,
    MF_RANGE, MF_DUPLICATE, MF_UNDEFINED, MF_REPLAY, MF_OBJECT
};
struct mf_diagnostic {
    enum mf_status code;
    struct mf_origin origin;
    struct mf_span detail; /* borrowed through report return */
};
struct mf_diagnostics {
    void *cookie;
    void (*report)(void *, const struct mf_diagnostic *);
};
struct mf_storage {
    void *cookie;
    void *(*acquire)(void *, size_t); /* suitably aligned; session lifetime */
};

/* Canonical text is ASCII/UTF-8 numeric octets, never native C text. */
int mf_span_equal(struct mf_span, struct mf_span);
enum mf_status mf_ascii_to_ebcdic(mf_octet, mf_octet *);
enum mf_status mf_ebcdic_to_ascii(mf_octet, mf_octet *);
enum mf_status mf_u64_parse(struct mf_span, struct mf_u64 *, int *negative);
enum mf_status mf_u64_add(struct mf_u64, struct mf_u64, struct mf_u64 *);
void mf_u64_negate(struct mf_u64, struct mf_u64 *);
/* Store the low width octets big endian (width<=8); the caller checks the
 * target signed/range interpretation before calling this binary helper.
 */
void mf_u64_store(struct mf_u64, mf_octet *, unsigned width);

enum mf_encoding { MF_ASCII, MF_CP037 };
struct mf_record { struct mf_span bytes; struct mf_origin origin; };
struct mf_records {
    void *cookie;
    enum mf_encoding encoding;
    enum mf_status (*next)(void *, struct mf_record *);
    enum mf_status (*replay)(void *);
};
struct mf_statement {
    struct mf_span label, operation, operand;
    struct mf_origin origin;
};
struct mf_statements {
    void *cookie;
    enum mf_status (*next)(void *, struct mf_statement *);
    enum mf_status (*replay)(void *);
};
/* On a provider error only statement.origin is meaningful, when available. */
/* Reader storage belongs to caller and remains alive through provider use.
 * Returned spans last until next/replay. Fixed card: columns 1..71 syntax,
 * column 72 continuation (unsupported initially), columns 73..80 sequence.
 */
struct mf_reader {
    struct mf_records records;
    mf_octet *buffer;
    size_t capacity;
    struct mf_origin last_origin;
};
enum mf_status mf_reader_init(struct mf_reader *, const struct mf_records *,
    mf_octet *, size_t, struct mf_statements *);

enum mf_profile { MF_S360, MF_S370, MF_ESA390, MF_Z900 };
enum mf_format { MF_RR, MF_RX, MF_RS, MF_SI, MF_SS, MF_S, MF_E, MF_RSY, MF_RIL };
struct mf_instruction {
    struct mf_span mnemonic;
    enum mf_profile minimum_profile;
    enum mf_format format;
    unsigned opcode, length;
};
struct mf_operands {
    unsigned r1, r2, r3, x2, b1, b2;
    mf_u32 d1, d2, immediate;
    unsigned length1, length2;
};
const struct mf_instruction *mf_machine_lookup(enum mf_profile, struct mf_span);
/* Lookup identifies known mnemonics even when forbidden by the profile;
 * mf_encode checks membership as well as operand ranges.
 */
enum mf_status mf_encode(enum mf_profile, const struct mf_instruction *,
    const struct mf_operands *, mf_octet *, size_t, unsigned *);

enum mf_symbol_kind { MF_LOCAL, MF_EXPORT, MF_EXTERNAL };
struct mf_section {
    unsigned id;
    struct mf_span name;
    mf_u32 length;
    unsigned amode; /* 0 means ANY, otherwise 24 or 31 */
    unsigned rmode; /* 24 or 31 (ANY encoded as 31) */
    int dummy;
};
struct mf_symbol {
    unsigned id;
    struct mf_span name;
    enum mf_symbol_kind kind;
    unsigned section;
    mf_u32 offset;
};
enum mf_reference_kind { MF_REF_SECTION, MF_REF_EXTERNAL };
enum mf_address_kind { MF_ADDRESS_A, MF_ADDRESS_V };
struct mf_fixup {
    unsigned section;
    mf_u32 offset;
    enum mf_reference_kind target_kind;
    unsigned target; /* session section id or external symbol id */
    enum mf_address_kind address_kind; /* A and V are not target identities */
    unsigned width;
    int subtract;
};
struct mf_entry { unsigned section; mf_u32 offset; int present; };
struct mf_sink {
    void *cookie;
    enum mf_status (*begin)(void *);
    enum mf_status (*write)(void *, const mf_octet *, size_t);
    enum mf_status (*finish)(void *, int valid);
};
struct mf_object_writer {
    void *cookie;
    enum mf_status (*begin)(void *, const struct mf_section *, size_t,
        const struct mf_symbol *, size_t);
    enum mf_status (*text)(void *, unsigned, mf_u32, const mf_octet *, size_t);
    enum mf_status (*gap)(void *, unsigned, mf_u32, mf_u32);
    enum mf_status (*fixup)(void *, const struct mf_fixup *);
    enum mf_status (*entry)(void *, const struct mf_entry *);
    enum mf_status (*finish)(void *, int valid);
    /* Optional, before begin. Initialize to NULL when unsupported. A named
     * TITLE requires this callback; the span is borrowed only for the call. */
    enum mf_status (*deck_id)(void *, struct mf_span);
    /* Optional ORG event. Resets the section cursor, not its contents.
     * Backward positions below prior relocated fields are unsupported. */
    enum mf_status (*origin)(void *, unsigned, mf_u32);
};
/* Writer owns bounded caller-supplied session storage, never host devices.
 * Events are ordered within each section unless an origin event resets it.
 * Selected overlays are limited to a suffix without earlier relocations.
 * A fixup follows its field's text before a gap/noncontiguous text in that
 * section; the writer validates the most recent contiguous text range.
 * Local symbols and dummy-section names are internal, not serialized.
 * END is emitted only for a valid finish request after successful events.
 * A deck is usable only when writer and sink finish(valid=1) succeed.
 */
struct mf_obj;
enum mf_status mf_obj_create(const struct mf_storage *, const struct mf_sink *,
    size_t max_sections, size_t max_symbols, struct mf_obj **,
    struct mf_object_writer *);
void mf_obj_destroy(struct mf_obj *);

struct mf_as_config {
    enum mf_profile profile;
    size_t max_sections, max_symbols, max_fixups;
    size_t max_literals; /* total pooled identities; zero disables literals */
    size_t max_statement, max_expression_depth;
};
struct mf_as_result {
    enum mf_status status;
    unsigned long statements;
    size_t sections, symbols, fixups, storage_requested;
    int valid_output;
};
struct mf_as;
enum mf_status mf_as_create(const struct mf_as_config *,
    const struct mf_storage *, struct mf_as **);
enum mf_status mf_as_assemble(struct mf_as *, const struct mf_statements *,
    const struct mf_object_writer *, const struct mf_diagnostics *,
    struct mf_as_result *);
void mf_as_destroy(struct mf_as *);
#endif
