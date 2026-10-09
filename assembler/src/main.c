/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Standard-C host services; the portable core owns no FILE or heap APIs.
 */
#include "mf_classic.h"
#ifdef MF_WITH_TRADITIONAL_MACROS
#include "mf_classic_macro.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define HOST_BUDGET 33554432UL
#define CARD_CAPACITY 256

struct allocation { struct allocation *next; void *bytes; };
struct host_storage { struct allocation *head; size_t used, limit; };
struct host_source { FILE *file; mf_octet card[CARD_CAPACITY]; unsigned long line; unsigned identity; };
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
    record->origin.source = source->identity; record->origin.line = source->line; record->origin.column = 1;
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
#ifdef MF_WITH_TRADITIONAL_MACROS
#define LIB_DIRS 16
#define LIB_MEMBERS 256
#define LIB_PATH 1024
struct host_library {
    const char *directories[LIB_DIRS]; size_t directory_count, known;
    char paths[LIB_MEMBERS][LIB_PATH];
    struct host_source handles[16];
    struct mf_origin last_invocation, last_model;
    char last_operation[64];
};
static enum mf_status library_open(void *cookie, struct mf_span name,
    struct mf_records *records, unsigned *identity)
{
    struct host_library *library; struct host_source *source;
    char member[128], path[LIB_PATH]; size_t i, j, k, n; FILE *file;
    static const char *suffixes[] = {".mac", ".asm", ""};
    library = (struct host_library *)cookie;
    if (name.length >= sizeof member) return MF_LIMIT;
    for (i = 0; i < name.length; ++i) {
        mf_octet c; c = name.data[i];
        member[i] = (char)(c >= 0x41 && c <= 0x5a ? c + 0x20 : c);
    }
    member[name.length] = 0; source = NULL;
    for (i = 0; i < 16; ++i) if (!library->handles[i].file) { source = library->handles + i; break; }
    if (!source) return MF_LIMIT;
    file = NULL;
    for (i = 0; i < library->directory_count && !file; ++i) {
        for (j = 0; j < 6 && !file; ++j) {
            if (j == 3) for (k = 0; k < name.length; ++k) member[k] = (char)name.data[k];
            n = strlen(library->directories[i]);
            if (n + name.length + strlen(suffixes[j % 3]) + 2 > sizeof path) return MF_LIMIT;
            strcpy(path, library->directories[i]); strcat(path, "/");
            strcat(path, member); strcat(path, suffixes[j % 3]);
            errno = 0; file = fopen(path, "rb");
            if (!file && errno != ENOENT) return MF_IO;
        }
        for (k = 0; k < name.length; ++k) {
            mf_octet c; c = name.data[k]; member[k] = (char)(c >= 0x41 && c <= 0x5a ? c + 0x20 : c);
        }
    }
    if (!file) return MF_UNDEFINED;
    for (i = 0; i < library->known; ++i) if (!strcmp(path, library->paths[i])) break;
    if (i == library->known) {
        if (i == LIB_MEMBERS) { fclose(file); return MF_LIMIT; }
        strcpy(library->paths[i], path); ++library->known;
    }
    memset(source, 0, sizeof *source); source->file = file; source->identity = (unsigned)i + 1;
    records->cookie = source; records->encoding = MF_ASCII;
    records->next = host_next; records->replay = host_replay; *identity = source->identity;
    return MF_OK;
}
static enum mf_status library_close(void *cookie, struct mf_records *records)
{
    struct host_source *source; int rc; (void)cookie;
    source = (struct host_source *)records->cookie; rc = fclose(source->file);
    source->file = NULL; return rc ? MF_IO : MF_OK;
}
#endif
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
#ifdef MF_WITH_TRADITIONAL_MACROS
    {
        struct host_library *library; library = (struct host_library *)cookie;
        path = diagnostic->origin.source && diagnostic->origin.source <= library->known ?
            library->paths[diagnostic->origin.source - 1] : library->paths[0];
        if (library->last_operation[0] &&
            diagnostic->origin.source == library->last_invocation.source &&
            diagnostic->origin.line == library->last_invocation.line) {
            const char *model_path;
            model_path = library->last_model.source && library->last_model.source <= library->known ?
                library->paths[library->last_model.source - 1] : library->paths[0];
            fprintf(stderr, "expanded %s from %s:%lu:%u\n", library->last_operation,
                model_path, library->last_model.line, library->last_model.column);
        }
    }
#else
    path = (const char *)cookie;
#endif
    fprintf(stderr, "%s:%lu:%u: %s (status %d)\n", path,
        diagnostic->origin.line, diagnostic->origin.column,
        status_name(diagnostic->code), (int)diagnostic->code);
}
#ifdef MF_WITH_TRADITIONAL_MACROS
static void macro_report(void *cookie, const struct mf_macro_event *event)
{
    struct host_library *library; size_t i; const char *path;
    library = (struct host_library *)cookie;
    if (event->status == MF_OK) {
        library->last_operation[0] = 0;
        if (event->depth && event->detail.length < sizeof library->last_operation) {
            memcpy(library->last_operation, event->detail.data, event->detail.length);
            library->last_operation[event->detail.length] = 0;
            library->last_invocation = event->origin;
            library->last_model = event->frames[event->depth - 1].model;
        }
        return;
    }
    for (i = 0; i < event->depth; ++i) {
        const struct mf_macro_frame_info *f; f = event->frames+i;
        path = f->model.source && f->model.source <= library->known ?
            library->paths[f->model.source-1] : library->paths[0];
        fprintf(stderr,"macro model %s:%lu:%u\n",path,f->model.line,f->model.column);
    }
}
#endif
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
    enum mf_profile profile; const char *input_path, *output_path; int arg, rc, use_macros; size_t literal_limit, symbol_limit, macro_model_limit, macro_limit;
#ifdef MF_WITH_TRADITIONAL_MACROS
    struct mf_macro_config macro_config; struct mf_macro *macros; struct mf_macro_observer macro_observer;
    struct host_library library; struct mf_macro_library resolver;
    memset(&library, 0, sizeof library);
    macros = NULL;
#endif
    profile = MF_S360; arg = 1; as = NULL; obj = NULL;
    use_macros = 0; literal_limit = 256; symbol_limit = 4096; macro_model_limit = 1024; macro_limit = 64;
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        puts("mf-classic-as 0.1.0-bootstrap"); return 0;
    }
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
#ifdef MF_WITH_TRADITIONAL_MACROS
        puts("usage: mf-classic-as [--profile s360|s370|esa390|z900] [--literal-limit 0..65536] [--symbol-limit 1..65536] [--macros] [--macro-model-limit 1..65536] [--macro-limit 1..1024] [-I directory] input.asm output.obj"); return 0;
#else
        puts("usage: mf-classic-as [--profile s360|s370|esa390|z900] [--literal-limit 0..65536] [--symbol-limit 1..65536] input.asm output.obj"); return 0;
#endif
    }
    if ((size_t)-1 < HOST_BUDGET || (size_t)-1 < 65536UL) {
        fprintf(stderr, "desktop storage configuration exceeds host size_t range\n"); return 2;
    }
    while (argc > arg) {
        if (strcmp(argv[arg], "--profile") == 0) {
            if (argc <= arg + 1) { fprintf(stderr, "missing --profile value\n"); return 2; }
            if (strcmp(argv[arg + 1], "s360") == 0) profile = MF_S360;
            else if (strcmp(argv[arg + 1], "s370") == 0) profile = MF_S370;
            else if (strcmp(argv[arg + 1], "esa390") == 0) profile = MF_ESA390;
            else if (strcmp(argv[arg + 1], "z900") == 0) profile = MF_Z900;
            else { fprintf(stderr, "unsupported profile: %s\n", argv[arg + 1]); return 2; }
            arg += 2;
        } else if (strcmp(argv[arg], "--literal-limit") == 0 || strcmp(argv[arg], "--symbol-limit") == 0 || strcmp(argv[arg], "--macro-model-limit") == 0 || strcmp(argv[arg], "--macro-limit") == 0) {
            const char *p; unsigned long n, maximum;
            int models, definitions, symbols; symbols = strcmp(argv[arg], "--symbol-limit") == 0; models = strcmp(argv[arg], "--macro-model-limit") == 0;
            definitions = strcmp(argv[arg], "--macro-limit") == 0; maximum = definitions ? 1024UL : 65536UL;
            if (argc <= arg + 1) { fprintf(stderr, "missing %s value\n",argv[arg]); return 2; }
            p = argv[arg + 1]; n = 0;
            if (!*p) { fprintf(stderr, "empty %s value\n",argv[arg]); return 2; }
            while (*p) {
                if (*p < '0' || *p > '9' || n > (maximum - (unsigned long)(*p - '0')) / 10UL) {
                    fprintf(stderr, "%s must be decimal %u..%lu\n",argv[arg],models || definitions || symbols ? 1U : 0U,maximum); return 2;
                }
                n = n * 10UL + (unsigned long)(*p++ - '0');
            }
            if (models || definitions || symbols) {
                if (!n) { fprintf(stderr,"%s must be positive\n",argv[arg]); return 2; }
                if (models) macro_model_limit = (size_t)n; else if (symbols) symbol_limit = (size_t)n; else macro_limit = (size_t)n;
            } else literal_limit = (size_t)n;
            arg += 2;
        } else if (strcmp(argv[arg], "--macros") == 0) {
#ifdef MF_WITH_TRADITIONAL_MACROS
            use_macros = 1; ++arg;
#else
            fprintf(stderr, "traditional macro provider is not included in this build\n"); return 2;
#endif
        } else if (strcmp(argv[arg], "-I") == 0) {
#ifdef MF_WITH_TRADITIONAL_MACROS
            if (argc <= arg + 1 || library.directory_count == LIB_DIRS) {
                fprintf(stderr, "missing or excessive library directory\n"); return 2;
            }
            library.directories[library.directory_count++] = argv[arg + 1]; arg += 2;
#else
            fprintf(stderr, "library provider is not included in this build\n"); return 2;
#endif
        } else break;
    }
    if (argc != arg + 2) {
#ifdef MF_WITH_TRADITIONAL_MACROS
        fprintf(stderr, "usage: mf-classic-as [--profile s360|s370|esa390|z900] [--literal-limit 0..65536] [--symbol-limit 1..65536] [--macros] [--macro-model-limit 1..65536] [--macro-limit 1..1024] [-I directory] input.asm output.obj\n"); return 2;
#else
        fprintf(stderr, "usage: mf-classic-as [--profile s360|s370|esa390|z900] [--literal-limit 0..65536] [--symbol-limit 1..65536] input.asm output.obj\n"); return 2;
#endif
    }
    if (!use_macros && (macro_model_limit != 1024 || macro_limit != 64)) { fprintf(stderr,"macro limits require --macros\n"); return 2; }
    input_path = argv[arg]; output_path = argv[arg + 1];
    if (strcmp(input_path, output_path) == 0 || !target_absent(output_path)) {
        fprintf(stderr, "output target already exists or cannot be checked: %s\n", output_path); return 2;
    }
    memset(&memory, 0, sizeof memory); memory.limit = (size_t)HOST_BUDGET;
    memset(&input, 0, sizeof input); memset(&output, 0, sizeof output);
    input.identity = 1;
#ifdef MF_WITH_TRADITIONAL_MACROS
    if (strlen(input_path) >= LIB_PATH) { fprintf(stderr, "input path too long\n"); return 2; }
    strcpy(library.paths[0], input_path); library.known = 1;
    if (library.directory_count && !use_macros) {
        fprintf(stderr, "-I requires --macros\n"); return 2;
    }
#endif
    input.file = fopen(input_path, "rb");
    if (!input.file) { fprintf(stderr, "cannot open input: %s\n", input_path); return 2; }
    output.file = tmpfile();
    if (!output.file) { fprintf(stderr, "cannot stage object output\n"); fclose(input.file); return 2; }
    storage.cookie = &memory; storage.acquire = host_acquire;
    records.cookie = &input; records.encoding = MF_ASCII; records.next = host_next; records.replay = host_replay;
    sink.cookie = &output; sink.begin = sink_begin; sink.write = sink_write; sink.finish = sink_finish;
#ifdef MF_WITH_TRADITIONAL_MACROS
    diagnostics.cookie = &library;
#else
    diagnostics.cookie = (void *)input_path;
#endif
    diagnostics.report = report;
    config.profile = profile; config.max_sections = 64; config.max_symbols = symbol_limit;
    config.max_literals = literal_limit;
    config.max_fixups = (size_t)65536UL; config.max_statement = 256; config.max_expression_depth = 32;
#ifdef MF_WITH_TRADITIONAL_MACROS
    if (use_macros) {
        macro_config.max_macros = macro_limit; macro_config.max_parameters = 16;
        macro_config.max_model_statements = macro_model_limit; macro_config.max_definition_bytes = 262144UL;
        macro_config.max_depth = 16; macro_config.max_argument_bytes = 4096;
        macro_config.max_statement_bytes = CARD_CAPACITY; macro_config.max_steps = 1000000UL;
        macro_observer.cookie = &library; macro_observer.notify = macro_report;
        status = mf_macro_create(&macro_config, &storage, &records, &macro_observer, &macros, &source);
        if (status == MF_OK && library.directory_count) {
            resolver.cookie = &library; resolver.open = library_open; resolver.close = library_close;
            status = mf_macro_set_library(macros, &resolver);
        }
    } else
#else
    (void)use_macros;
#endif
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
    mf_obj_destroy(obj); mf_as_destroy(as);
#ifdef MF_WITH_TRADITIONAL_MACROS
    mf_macro_destroy(macros);
#endif
    host_release(&memory);
    return rc;
}
