/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Optional original Gate 1 traditional statement provider, portable C89.
 */
#ifndef MF_CLASSIC_MACRO_H
#define MF_CLASSIC_MACRO_H
#include "mf_classic.h"

struct mf_macro_config {
    size_t max_macros, max_parameters, max_model_statements;
    size_t max_definition_bytes, max_depth, max_argument_bytes;
    size_t max_statement_bytes;
    unsigned long max_steps; /* raw records + executed models per pass */
};
struct mf_macro_frame_info {
    struct mf_span name;
    struct mf_origin invocation, definition, model;
};
struct mf_macro_event {
    enum mf_status status; /* MF_OK: yielded statement; other: failure */
    struct mf_origin origin;
    struct mf_span detail;
    const struct mf_macro_frame_info *frames; /* outermost first */
    size_t depth;
};
struct mf_macro_observer {
    void *cookie;
    void (*notify)(void *, const struct mf_macro_event *);
};
struct mf_macro;
enum mf_status mf_macro_create(const struct mf_macro_config *,
    const struct mf_storage *, const struct mf_records *,
    const struct mf_macro_observer *, struct mf_macro **,
    struct mf_statements *);
void mf_macro_destroy(struct mf_macro *);

/* All nonzero limits are required; max_parameters may be zero. Tables and
 * arenas are allocated at create; next/replay allocate nothing. The allocator
 * owns session lifetime, including partial allocations after create failure.
 * Caller owns records and storage; destroy neither closes nor frees either.
 *
 * Supports inline MACRO/prototype/MEND, label/positional/keyword parameters,
 * literal-text keyword defaults, explicit empty overrides, substitution and
 * period delimiters, nested invocations. Missing positional actuals are null.
 * Only declared positional slots are accepted; extra slots are unsupported.
 * A single comma is accepted as the empty operand on MACRO/MEND markers only;
 * comma-separated empty actuals retain their positional slots.
 * COPY, continuation, conditional controls/variables, attributes, SYSNDX,
 * parameter indexing, escaped ampersands and nested definitions are unsupported.
 * Definitions cannot be redefined. Names compare case-insensitively; character
 * argument values retain their original octets. ASCII and CP037 records use
 * fixed cards (syntax1..71; continuation72 rejected; sequence73..80 ignored).
 *
 * Spans survive until next/replay. On error only origin is meaningful. A
 * generated statement's primary origin is the outermost source invocation.
 * Observer spans/frames are borrowed only through notify return, including
 * current model/prototype and nested-call coordinates. They are not a retained
 * history: delayed assembler diagnostics keep only the primary invocation.
 *
 * Replay requires completed EOF, resets all definition/expansion state and
 * reuses storage. EOF compares two 32-bit fingerprints, record/byte counts and
 * all original coordinates, including comments/unused definitions/sequence
 * fields. Differences return MF_REPLAY. This is a consistency check, not a
 * cryptographic guarantee; records owns stable replay. Provider failure must
 * invalidate assembly output through the ordinary assembler/writer contract.
 */
#endif
