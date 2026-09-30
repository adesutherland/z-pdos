/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 */
#include "mf_classic.h"
#define CHECK(x) do { if (!(x)) return __LINE__; } while (0)
static const mf_octet line[] = {0x4c,0x41,0x42,0x45,0x4c,0x20,0x20,0x20,0x20,0x44,0x43,0x20,0x20,0x20,0x20,0x43,0x27,0x41,0x20,0x42,0x27,0x27,0x43,0x27,0x20,0x74,0x72,0x61,0x69,0x6c,0x69,0x6e,0x67,0x20,0x72,0x65,0x6d,0x61,0x72,0x6b,0x73};
static const mf_octet empty[] = {0x20,0x20,0x20};
static const mf_octet comment[] = {0x2a,0x20,0x63,0x6f,0x6d,0x6d,0x65,0x6e,0x74};
static const mf_octet shortline[] = {0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x4c,0x52,0x20,0x20,0x20,0x20,0x31,0x2c,0x32};
static const mf_octet badquote[] = {0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x44,0x43,0x20,0x20,0x20,0x20,0x43,0x27,0x41,0x20,0x42};
static const mf_octet tab[] = {0x09,0x20,0x4c,0x52,0x20,0x31,0x2c,0x32};
static const mf_octet invalidlabel[] = {0x31,0x41,0x42,0x43,0x20,0x4c,0x52,0x20,0x31,0x2c,0x32};
static const mf_octet labelonly[] = {0x4c,0x41,0x42,0x45,0x4c};
static const mf_octet operation[] = {0x44,0x43};
static const mf_octet label[] = {0x4c,0x41,0x42,0x45,0x4c};
static const mf_octet operand[] = {0x43,0x27,0x41,0x20,0x42,0x27,0x27,0x43,0x27};
static const mf_octet lr[] = {0x4c,0x52};
static const mf_octet lrarg[] = {0x31,0x2c,0x32};
struct input {
    struct mf_span lines[4];
    unsigned count, cursor;
    enum mf_status next_error, replay_error;
};
static struct mf_span span(const mf_octet *data, size_t length)
{
    struct mf_span s; s.data = data; s.length = length; return s;
}
static enum mf_status next(void *cookie, struct mf_record *record)
{
    struct input *input;
    input = (struct input *)cookie;
    if (input->next_error != MF_OK) return input->next_error;
    if (input->cursor == input->count) return MF_EOF;
    record->bytes = input->lines[input->cursor];
    record->origin.source = 7;
    record->origin.line = ++input->cursor;
    record->origin.column = 1;
    return MF_OK;
}
static enum mf_status replay(void *cookie)
{
    struct input *input;
    input = (struct input *)cookie;
    if (input->replay_error != MF_OK) return input->replay_error;
    input->cursor = 0; return MF_OK;
}
int main(void)
{
    struct input input;
    struct mf_records records;
    struct mf_reader reader;
    struct mf_statements statements;
    struct mf_statement statement;
    mf_octet buffer[80], card[80], ebcdic[80], overlong[81];
    unsigned i;
    input.count = 3; input.cursor = 0;
    input.next_error = input.replay_error = MF_OK;
    input.lines[0] = span(empty,sizeof(empty));
    input.lines[1] = span(comment,sizeof(comment));
    input.lines[2] = span(line,sizeof(line));
    records.cookie = &input; records.encoding = MF_ASCII;
    records.next = next; records.replay = replay;
    CHECK(mf_reader_init(&reader,&records,buffer,sizeof(buffer),&statements) == MF_OK);
    CHECK(statements.next(statements.cookie,&statement) == MF_OK);
    CHECK(statement.origin.source == 7 && statement.origin.line == 3 &&
          statement.origin.column == 1);
    CHECK(mf_span_equal(statement.label,span(label,sizeof(label))));
    CHECK(mf_span_equal(statement.operation,span(operation,sizeof(operation))));
    CHECK(mf_span_equal(statement.operand,span(operand,sizeof(operand))));
    CHECK(statements.next(statements.cookie,&statement) == MF_EOF);
    CHECK(statements.replay(statements.cookie) == MF_OK);
    CHECK(statements.next(statements.cookie,&statement) == MF_OK);
    input.replay_error = MF_REPLAY;
    CHECK(statements.replay(statements.cookie) == MF_REPLAY);
    input.replay_error = MF_IO;
    CHECK(statements.replay(statements.cookie) == MF_IO);
    input.replay_error = MF_OK;
    input.next_error = MF_IO;
    CHECK(statements.next(statements.cookie,&statement) == MF_IO);
    CHECK(statement.origin.source == 0 && statement.origin.line == 0);
    input.next_error = MF_REPLAY;
    CHECK(statements.next(statements.cookie,&statement) == MF_REPLAY);
    input.next_error = MF_OK;
    input.count = 1; input.cursor = 0;
    input.lines[0] = span(shortline,sizeof(shortline));
    CHECK(statements.next(statements.cookie,&statement) == MF_OK);
    CHECK(!statement.label.length &&
          mf_span_equal(statement.operation,span(lr,sizeof(lr))) &&
          mf_span_equal(statement.operand,span(lrarg,sizeof(lrarg))));
    /* A record provider may return a changed stream; reader exposes it, while
       the engine is responsible for comparing complete replay semantics. */
    input.lines[0] = span(line,sizeof(line));
    CHECK(statements.replay(statements.cookie) == MF_OK);
    CHECK(statements.next(statements.cookie,&statement) == MF_OK);
    CHECK(statement.label.length == sizeof(label));
    CHECK(mf_reader_init(&reader,&records,buffer,4,&statements) == MF_OK);
    CHECK(statements.replay(statements.cookie) == MF_OK);
    CHECK(statements.next(statements.cookie,&statement) == MF_LIMIT);
    CHECK(mf_reader_init(&reader,&records,buffer,sizeof(buffer),&statements) == MF_OK);
    for (i = 0; i < 80; ++i) card[i] = 0x20;
    for (i = 0; i < sizeof(shortline); ++i) card[i] = shortline[i];
    for (i = 72; i < 80; ++i) card[i] = 0xff; /* sequence is not syntax */
    input.lines[0] = span(card,80); input.cursor = 0;
    CHECK(statements.next(statements.cookie,&statement) == MF_OK);
    card[71] = 0x31; input.cursor = 0;
    CHECK(statements.next(statements.cookie,&statement) == MF_UNSUPPORTED);
    CHECK(reader.last_origin.column == 72);
    CHECK(statement.origin.source == 7 && statement.origin.line == 1 &&
          statement.origin.column == 72);
    card[71] = 0x20; card[30] = 0x80; input.cursor = 0;
    CHECK(statements.next(statements.cookie,&statement) == MF_SOURCE);
    CHECK(reader.last_origin.column == 31);
    CHECK(statement.origin.source == 7 && statement.origin.line == 1 &&
          statement.origin.column == 31);
    input.lines[0] = span(overlong,81); input.cursor = 0;
    CHECK(statements.next(statements.cookie,&statement) == MF_LIMIT);
    input.lines[0] = span(badquote,sizeof(badquote)); input.cursor = 0;
    CHECK(statements.next(statements.cookie,&statement) == MF_SOURCE);
    input.lines[0] = span(tab,sizeof(tab)); input.cursor = 0;
    CHECK(statements.next(statements.cookie,&statement) == MF_SOURCE);
    input.lines[0] = span(invalidlabel,sizeof(invalidlabel)); input.cursor = 0;
    CHECK(statements.next(statements.cookie,&statement) == MF_SOURCE);
    input.lines[0] = span(labelonly,sizeof(labelonly)); input.cursor = 0;
    CHECK(statements.next(statements.cookie,&statement) == MF_SOURCE);
    records.encoding = MF_CP037;
    for (i = 0; i < sizeof(line); ++i)
        CHECK(mf_ascii_to_ebcdic(line[i],ebcdic + i) == MF_OK);
    input.lines[0] = span(ebcdic,sizeof(line)); input.cursor = 0;
    CHECK(mf_reader_init(&reader,&records,buffer,sizeof(buffer),&statements) == MF_OK);
    CHECK(statements.next(statements.cookie,&statement) == MF_OK);
    CHECK(mf_span_equal(statement.label,span(label,sizeof(label))));
    CHECK(mf_span_equal(statement.operand,span(operand,sizeof(operand))));
    ebcdic[0] = 0xff; input.cursor = 0;
    CHECK(statements.next(statements.cookie,&statement) == MF_UNSUPPORTED);
    records.encoding = (enum mf_encoding)3;
    CHECK(mf_reader_init(&reader,&records,buffer,sizeof(buffer),&statements) == MF_UNSUPPORTED);
    return 0;
}
