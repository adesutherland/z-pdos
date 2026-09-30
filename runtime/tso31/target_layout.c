/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Compile only with the selected 31-bit target C compiler before linking.
 * Native host success is neither expected nor a target calling convention test.
 * Big-endian representation and register linkage need producer/output checks.
 */
#include <limits.h>
#include <stddef.h>
#include "abi.h"

typedef char tso31_octet_bits[(CHAR_BIT == 8) ? 1 : -1];
typedef char tso31_int_size[(sizeof(int) == 4) ? 1 : -1];
typedef char tso31_ulong_size[(sizeof(unsigned long) == 4) ? 1 : -1];
typedef char tso31_pointer_size[(sizeof(void *) == 4) ? 1 : -1];
typedef char tso31_function_pointer_size[(sizeof(int (*)(void)) == 4) ? 1 : -1];
typedef char tso31_vector_size[(sizeof(struct lab_tso31_services) == 28) ? 1 : -1];
typedef char tso31_version_offset[
    (offsetof(struct lab_tso31_services, version) == 0) ? 1 : -1];
typedef char tso31_putline_offset[
    (offsetof(struct lab_tso31_services, putline) == 4) ? 1 : -1];
typedef char tso31_allocate_offset[
    (offsetof(struct lab_tso31_services, allocate) == 8) ? 1 : -1];
typedef char tso31_release_offset[
    (offsetof(struct lab_tso31_services, release) == 12) ? 1 : -1];
typedef char tso31_filecall_offset[
    (offsetof(struct lab_tso31_services, filecall) == 16) ? 1 : -1];
typedef char tso31_finish_offset[
    (offsetof(struct lab_tso31_services, finish) == 20) ? 1 : -1];
typedef char tso31_readline_offset[
    (offsetof(struct lab_tso31_services, readline) == 24) ? 1 : -1];
