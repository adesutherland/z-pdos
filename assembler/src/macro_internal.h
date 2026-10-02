/* SPDX-License-Identifier: MIT */
#ifndef MF_MACRO_INTERNAL_H
#define MF_MACRO_INTERNAL_H
#include "mf_classic_macro.h"
int mf_macro_name_first(mf_octet);
int mf_macro_name_rest(mf_octet);
mf_octet mf_macro_upper(mf_octet);
int mf_macro_same(struct mf_span, struct mf_span);
enum mf_status mf_macro_card(const struct mf_record *, enum mf_encoding,
    mf_octet *, size_t, struct mf_statement *, int *, int prefix_only, int logical);
enum mf_status mf_macro_arguments(struct mf_span, size_t *, struct mf_span *);
struct mf_macro_value { int character; long number; struct mf_span text; };
enum mf_status mf_macro_eval(struct mf_span, size_t, struct mf_macro_value *);
#endif
