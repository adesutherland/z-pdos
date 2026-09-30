/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Standard-C host services; the portable core owns no FILE or heap APIs.
 */
#include "mf_classic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define HOST_BUDGET 33554432UL
#define CARD_CAPACITY 256

struct allocation { struct allocation *next; void *bytes; };
struct host_storage { struct allocation *head; size_t used, limit; };
struct host_source { FILE *file; mf_octet card[CARD_CAPACITY]; unsigned long line; };
struct host_sink { FILE *file; int completed; };

static void *host_acquire(void *cookie, size_t n)
{
    struct host_storage *storage; struct allocation *node; void *bytes;
    storage = (struct host_storage *)cookie;
    if (storage->used > storage->limit || n > storage->limit - storage->used) return NULL;
    node = (struct allocation *)malloc(sizeof *node); if (!node) return NULL;
    bytes = malloc(n ? n : 1);
    if (!bytes) { free(node); return NULL; }
    node->bytes = bytes; node->next = storage->head; storage->head = node;
    storage->used += n; return bytes;
}
static void host_release(struct host_storage *storage)
{
    struct allocation *node, *next;
    for (node = storage->head; node; node = next) {
        next = node->next; free(node->bytes); free(node);
    }
    storage->head = NULL;
}
/* Source files are canonical ASCII octets, opened in binary mode. On a host
 * with EBCDIC native text, callers still supply ASCII source files. */
static enum mf_status host_next(void *cookie, struct mf_record *record)
{
    struct host_source *source; size_t n; int c, too_long;
    source = (struct host_source *)cookie; n = 0; too_long = 0;
    record->origin.source = 1; record->origin.line = source->line; record->origin.column = 1;
    if (source->line != ULONG_MAX) ++record->origin.line;
    while ((c = fgetc(source->file)) != EOF) {
        if (c == 0x0a) break;
        if (n == sizeof source->card) too_long = 1;
        else source->card[n++] = (mf_octet)c;
    }
    if (ferror(source->file)) return MF_IO;
    if (c == EOF && !n && !too_long) return MF_EOF;
    if (source->line == ULONG_MAX) return MF_LIMIT;
    ++source->line;
    if (too_long) return MF_LIMIT;
    if (n && source->card[n - 1] == 0x0d) --n;
    record->bytes.data = source->card; record->bytes.length = n;
    return MF_OK;
}
static enum mf_status host_replay(void *cookie)
{
    struct host_source *source;
    source = (struct host_source *)cookie;
    if (fseek(source->file, 0L, SEEK_SET) != 0) return MF_IO;
    clearerr(source->file); source->line = 0; return MF_OK;
}
static enum mf_status sink_begin(void *cookie)
{ ((struct host_sink *)cookie)->completed = 0; return MF_OK; }
static enum mf_status sink_write(void *cookie, const mf_octet *data, size_t n)
{
    struct host_sink *sink;
    sink = (struct host_sink *)cookie;
    return fwrite(data, 1, n, sink->file) == n ? MF_OK : MF_IO;
}
static enum mf_status sink_finish(void *cookie, int valid)
{
    struct host_sink *sink;
    sink = (struct host_sink *)cookie;
    if (!valid) { sink->completed = 0; return MF_OK; }
    if (fflush(sink->file) != 0) return MF_IO;
    sink->completed = 1; return MF_OK;
}
static const char *status_name(enum mf_status status)
{
    switch (status) {
    case MF_OK: return "success";
    case MF_EOF: return "unexpected end of input";
    case MF_SOURCE: return "invalid source or operand class";
    case MF_UNSUPPORTED: return "unsupported bootstrap feature";
    case MF_LIMIT: return "configured capacity exhausted";
    case MF_IO: return "I/O failure";
    case MF_RANGE: return "operand or address outside supported range";
    case MF_DUPLICATE: return "duplicate symbol";
    case MF_UNDEFINED: return "undefined symbol";
    case MF_REPLAY: return "source changed between assembly passes";
    case MF_OBJECT: return "object cannot represent assembly";
    default: return "unknown failure";
    }
}
static void report(void *cookie, const struct mf_diagnostic *diagnostic)
{
    const char *path;
    path = (const char *)cookie;
    fprintf(stderr, "%s:%lu:%u: %s (status %d)\n", path,
        diagnostic->origin.line, diagnostic->origin.column,
        status_name(diagnostic->code), (int)diagnostic->code);
}
/* A standard-C existence check, conservative on systems exposing ENOENT.
 * Exclusive atomic publication is not provided by ISO C89 stdio. */
static int target_absent(const char *path)
{
    FILE *file;
    errno = 0; file = fopen(path, "rb");
    if (file) { fclose(file); return 0; }
#ifdef ENOENT
    return errno == ENOENT;
#else
    return errno == 0;
#endif
}
static enum mf_status publish(FILE *staged, const char *path)
{
    FILE *output; mf_octet bytes[4096]; size_t n; int failed;
    if (!target_absent(path)) return MF_IO;
    if (fseek(staged, 0L, SEEK_SET) != 0) return MF_IO;
    output = fopen(path, "wb"); if (!output) return MF_IO;
    failed = 0;
    while ((n = fread(bytes, 1, sizeof bytes, staged)) != 0) {
        if (fwrite(bytes, 1, n, output) != n) { failed = 1; break; }
    }
    if (ferror(staged)) failed = 1;
    if (fclose(output) != 0) failed = 1;
    if (failed) { remove(path); return MF_IO; }
    return MF_OK;
}
int main(int argc, char **argv)
{
    struct host_storage memory; struct host_source input; struct host_sink output;
    struct mf_storage storage; struct mf_records records; struct mf_reader reader;
    struct mf_statements source; struct mf_sink sink; struct mf_object_writer writer;
    struct mf_diagnostics diagnostics; struct mf_as_config config;
    struct mf_as_result result; struct mf_as *as; struct mf_obj *obj;
    mf_octet reader_buffer[CARD_CAPACITY]; enum mf_status status;
    enum mf_profile profile; const char *input_path, *output_path; int arg, rc;
    profile = MF_S360; arg = 1; as = NULL; obj = NULL;
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        puts("mf-classic-as 0.1.0-bootstrap"); return 0;
    }
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        puts("usage: mf-classic-as [--profile s360|s370] input.asm output.obj"); return 0;
    }
    if ((size_t)-1 < HOST_BUDGET || (size_t)-1 < 65536UL) {
        fprintf(stderr, "desktop storage configuration exceeds host size_t range\n"); return 2;
    }
    if (argc > arg && strcmp(argv[arg], "--profile") == 0) {
        if (argc <= arg + 1) { fprintf(stderr, "missing --profile value\n"); return 2; }
        if (strcmp(argv[arg + 1], "s360") == 0) profile = MF_S360;
        else if (strcmp(argv[arg + 1], "s370") == 0) profile = MF_S370;
        else { fprintf(stderr, "unsupported profile: %s\n", argv[arg + 1]); return 2; }
        arg += 2;
    }
    if (argc != arg + 2) {
        fprintf(stderr, "usage: mf-classic-as [--profile s360|s370] input.asm output.obj\n"); return 2;
    }
    input_path = argv[arg]; output_path = argv[arg + 1];
    if (strcmp(input_path, output_path) == 0 || !target_absent(output_path)) {
        fprintf(stderr, "output target already exists or cannot be checked: %s\n", output_path); return 2;
    }
    memset(&memory, 0, sizeof memory); memory.limit = (size_t)HOST_BUDGET;
    memset(&input, 0, sizeof input); memset(&output, 0, sizeof output);
    input.file = fopen(input_path, "rb");
    if (!input.file) { fprintf(stderr, "cannot open input: %s\n", input_path); return 2; }
    output.file = tmpfile();
    if (!output.file) { fprintf(stderr, "cannot stage object output\n"); fclose(input.file); return 2; }
    storage.cookie = &memory; storage.acquire = host_acquire;
    records.cookie = &input; records.encoding = MF_ASCII; records.next = host_next; records.replay = host_replay;
    sink.cookie = &output; sink.begin = sink_begin; sink.write = sink_write; sink.finish = sink_finish;
    diagnostics.cookie = (void *)input_path; diagnostics.report = report;
    config.profile = profile; config.max_sections = 64; config.max_symbols = 4096;
    config.max_fixups = (size_t)65536UL; config.max_statement = 256; config.max_expression_depth = 32;
    status = mf_reader_init(&reader, &records, reader_buffer, sizeof reader_buffer, &source);
    if (status == MF_OK) status = mf_as_create(&config, &storage, &as);
    if (status == MF_OK) status = mf_obj_create(&storage, &sink, config.max_sections, config.max_symbols, &obj, &writer);
    if (status == MF_OK) status = mf_as_assemble(as, &source, &writer, &diagnostics, &result);
    if (fclose(input.file) != 0 && status == MF_OK) status = MF_IO;
    if (status == MF_OK && output.completed) status = publish(output.file, output_path);
    else if (status == MF_OK) status = MF_IO;
    if (fclose(output.file) != 0 && status == MF_OK) { remove(output_path); status = MF_IO; }
    if (status == MF_OK) {
        fprintf(stderr, "assembled %lu statements, %lu sections, %lu symbols, %lu fixups\n",
            result.statements, (unsigned long)result.sections, (unsigned long)result.symbols, (unsigned long)result.fixups);
        rc = 0;
    } else { fprintf(stderr, "assembly failed: %s\n", status_name(status)); rc = 1; }
    mf_obj_destroy(obj); mf_as_destroy(as); host_release(&memory);
    return rc;
}
