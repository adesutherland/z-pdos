/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Original identity statement provider. All lexical codes are numeric ASCII;
 * original fixed-card columns are retained before any text decoding.
 */
#include "mf_classic.h"

static int name_first(mf_octet ch)
{
    return (ch >= 0x41 && ch <= 0x5a) ||
           (ch >= 0x61 && ch <= 0x7a) ||
           ch == 0x24 || ch == 0x23 || ch == 0x40 || ch == 0x5f;
}

static int name_rest(mf_octet ch)
{
    return name_first(ch) || (ch >= 0x30 && ch <= 0x39);
}

static enum mf_status reader_next_inner(void *cookie, struct mf_statement *out)
{
    struct mf_reader *reader;
    struct mf_record record;
    struct mf_statement statement;
    size_t n, i, start;
    enum mf_status status;
    mf_octet first, continuation;
    int quoted;
    reader = (struct mf_reader *)cookie;
    if (!out) return MF_SOURCE;
    for (;;) {
        status = reader->records.next(reader->records.cookie, &record);
        if (status != MF_OK) return status;
        reader->last_origin = record.origin;
        if (record.bytes.length && !record.bytes.data) return MF_SOURCE;
        if (record.bytes.length > 80) return MF_LIMIT;
        if (!record.bytes.length) continue;
        first = record.bytes.data[0];
        if (reader->records.encoding == MF_CP037) {
            status = mf_ebcdic_to_ascii(first, &first);
            if (status != MF_OK) return status;
        }
        if (first == 0x2a) continue;
        if (record.bytes.length >= 72) {
            continuation = record.bytes.data[71];
            if (reader->records.encoding == MF_CP037) {
                status = mf_ebcdic_to_ascii(continuation, &continuation);
                if (status != MF_OK) return status;
            }
            if (continuation != 0x20) {
                reader->last_origin.column = 72;
                return MF_UNSUPPORTED;
            }
        }
        n = record.bytes.length < 71 ? record.bytes.length : 71;
        if (n > reader->capacity) return MF_LIMIT;
        for (i = 0; i < n; ++i) {
            reader->buffer[i] = record.bytes.data[i];
            if (reader->records.encoding == MF_CP037) {
                status = mf_ebcdic_to_ascii(reader->buffer[i],
                                           &reader->buffer[i]);
                if (status != MF_OK) {
                    reader->last_origin.column = (unsigned)i + 1;
                    return status;
                }
            }
            if (reader->buffer[i] < 0x20 || reader->buffer[i] > 0x7e) {
                reader->last_origin.column = (unsigned)i + 1;
                return MF_SOURCE;
            }
        }
        statement.origin = record.origin;
        statement.label.data = reader->buffer;
        statement.label.length = 0;
        statement.operation = statement.label;
        statement.operand = statement.label;
        i = 0;
        if (reader->buffer[0] != 0x20) {
            if (!name_first(reader->buffer[0])) return MF_SOURCE;
            while (i < n && reader->buffer[i] != 0x20) {
                if (!name_rest(reader->buffer[i])) {
                    reader->last_origin.column = (unsigned)i + 1;
                    return MF_SOURCE;
                }
                ++i;
            }
            statement.label.length = i;
        }
        while (i < n && reader->buffer[i] == 0x20) ++i;
        if (i == n) {
            if (statement.label.length) return MF_SOURCE;
            continue;
        }
        start = i;
        while (i < n && reader->buffer[i] != 0x20) {
            if (!name_rest(reader->buffer[i])) {
                reader->last_origin.column = (unsigned)i + 1;
                return MF_SOURCE;
            }
            ++i;
        }
        statement.operation.data = reader->buffer + start;
        statement.operation.length = i - start;
        while (i < n && reader->buffer[i] == 0x20) ++i;
        start = i;
        quoted = 0;
        while (i < n) {
            if (reader->buffer[i] == 0x27 && (quoted || i == start ||
                (reader->buffer[i-1] != 0x4c && reader->buffer[i-1] != 0x6c))) quoted = !quoted;
            if (!quoted && reader->buffer[i] == 0x20) break;
            ++i;
        }
        if (quoted) {
            reader->last_origin.column = (unsigned)start + 1;
            return MF_SOURCE;
        }
        statement.operand.data = reader->buffer + start;
        statement.operand.length = i - start;
        *out = statement;
        return MF_OK;
    }
}

/* On failure only origin is valid; statement spans are not usable. */
static enum mf_status reader_next(void *cookie, struct mf_statement *out)
{
    struct mf_reader *reader;
    enum mf_status status;
    reader = (struct mf_reader *)cookie;
    if (!out) return MF_SOURCE;
    reader->last_origin.source = 0;
    reader->last_origin.line = 0;
    reader->last_origin.column = 0;
    out->origin = reader->last_origin;
    status = reader_next_inner(cookie, out);
    if (status != MF_OK) out->origin = reader->last_origin;
    return status;
}

static enum mf_status reader_replay(void *cookie)
{
    struct mf_reader *reader;
    reader = (struct mf_reader *)cookie;
    return reader->records.replay(reader->records.cookie);
}

enum mf_status mf_reader_init(struct mf_reader *reader,
    const struct mf_records *records, mf_octet *buffer, size_t capacity,
    struct mf_statements *out)
{
    if (!reader || !records || !records->next || !records->replay ||
        !buffer || !capacity || !out) return MF_SOURCE;
    if (records->encoding != MF_ASCII && records->encoding != MF_CP037)
        return MF_UNSUPPORTED;
    reader->records = *records;
    reader->buffer = buffer;
    reader->capacity = capacity;
    reader->last_origin.source = 0;
    reader->last_origin.line = 0;
    reader->last_origin.column = 0;
    out->cookie = reader;
    out->next = reader_next;
    out->replay = reader_replay;
    return MF_OK;
}
