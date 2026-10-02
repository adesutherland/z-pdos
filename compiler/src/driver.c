/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Development-host launcher. The bootstrap assembler has no POSIX dependency;
 * this launcher finds its private GCC tools relative to its own executable.
 */
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <process.h>
#include <io.h>
#define access _access
#ifdef X_OK
#undef X_OK
#endif
#define X_OK 0
#define EXE ".exe"
#else
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#define EXE ""
#endif

#ifndef MF_CC_VARIANT
#define MF_CC_VARIANT "mvs"
#endif

#if defined(_WIN32) || defined(MF_TEST_WINDOWS_QUOTING)
/* _spawnv joins argv with spaces; quote each argument for the Windows CRT. */
static char *windows_argument(const char *text)
{
    size_t length = strlen(text), slashes;
    char *result = (char *)malloc(length * 2 + 3), *out;
    if (!result) return NULL;
    out = result;
    *out++ = '"';
    while (*text) {
        slashes = 0;
        while (*text == '\\') { ++slashes; ++text; }
        if (!*text || *text == '"') slashes *= 2;
        while (slashes--) *out++ = '\\';
        if (*text == '"') *out++ = '\\';
        if (*text) *out++ = *text++;
    }
    *out++ = '"';
    *out = '\0';
    return result;
}
#endif

/* The launcher is a native desktop adapter, outside the bootstrap C core. */
static int executable_path(char *path, size_t size, const char *argument)
{
#ifdef _WIN32
    DWORD n = GetModuleFileNameA(NULL, path, (DWORD)size);
    (void)argument;
    return n > 0 && n < size;
#else
    char candidate[4096];
#ifdef __APPLE__
    uint32_t n = (uint32_t)sizeof(candidate);
    (void)size;
    if (_NSGetExecutablePath(candidate, &n) == 0 &&
        realpath(candidate, path)) return 1;
#else
    ssize_t n = readlink("/proc/self/exe", path, size - 1);
    if (n > 0 && (size_t)n < size - 1) {
        path[n] = '\0';
        return 1;
    }
#endif
    if (strchr(argument, '/') && realpath(argument, path)) return 1;
    if (!strchr(argument, '/')) {
        const char *start = getenv("PATH");
        while (start) {
            const char *end = strchr(start, ':');
            size_t len = end ? (size_t)(end - start) : strlen(start);
            if (len + strlen(argument) + 2 < sizeof(candidate)) {
                memcpy(candidate, start, len);
                candidate[len] = '\0';
                strcat(candidate, "/");
                strcat(candidate, argument);
                if (access(candidate, X_OK) == 0 && realpath(candidate, path))
                    return 1;
            }
            start = end ? end + 1 : NULL;
        }
    }
    return 0;
#endif
}

int main(int argc, char **argv)
{
    char **child;
    char path[4096], directory[4096], compiler[4096], cc1[4096], prefix[4100];
    char *separator;
    int i;
    int stage = 0;
    int query = 0;
    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-c")) {
            fprintf(stderr, "mf-classic-cc: object generation awaits the "
                    "mf-classic-as compiler-output contract\n");
            return 2;
        }
        if (!strncmp(argv[i], "--profile", 9)) {
            fprintf(stderr, "mf-classic-cc: named machine profiles are "
                    "roadmap contracts; no Classic profile is qualified yet\n");
            return 2;
        }
        if (!strcmp(argv[i], "-S") || !strcmp(argv[i], "-E") ||
            !strcmp(argv[i], "-fsyntax-only")) stage = 1;
        if (!strcmp(argv[i], "--version") || !strcmp(argv[i], "-dumpmachine") ||
            !strcmp(argv[i], "-dumpversion") || !strcmp(argv[i], "-dumpspecs") ||
            !strncmp(argv[i], "-print-", 7)) query = 1;
    }
    if (!stage && !query && !(argc == 2 && !strcmp(argv[1], "-v"))) {
        fprintf(stderr, "mf-classic-cc: use -S, -E or -fsyntax-only; "
                "assembly and linking await mf-classic-as integration\n");
        return 2;
    }
    if (!executable_path(path, sizeof(path), argv[0])) {
        fprintf(stderr, "mf-classic-cc: cannot locate its executable\n");
        return 2;
    }
    for (i = 0; path[i]; ++i) if (path[i] == '\\') path[i] = '/';
    separator = strrchr(path, '/');
    if (!separator) return 2;
    *separator = '\0';
    if (strlen(path) + 64 >= sizeof(directory)) return 2;
    strcpy(directory, path);
    strcpy(compiler, directory);
    strcat(compiler, "/xgcc" EXE);
    if (access(compiler, X_OK) != 0) {
        strcat(directory, "/../libexec/z-pdos/" MF_CC_VARIANT);
        strcpy(compiler, directory);
        strcat(compiler, "/xgcc" EXE);
    }
    strcpy(cc1, directory);
    strcat(cc1, "/cc1" EXE);
    if (access(compiler, X_OK) != 0 || access(cc1, X_OK) != 0) {
        fprintf(stderr, "mf-classic-cc: missing private xgcc/cc1 for %s\n",
                MF_CC_VARIANT);
        return 2;
    }
    strcpy(prefix, "-B");
    strcat(prefix, directory);
    strcat(prefix, "/");
    child = (char **)calloc((size_t)argc + 2, sizeof(*child));
    if (!child) return 2;
    child[0] = compiler;
    child[1] = prefix;
    for (i = 1; i < argc; ++i) child[i + 1] = argv[i];
#ifdef _WIN32
    for (i = 0; i < argc + 1; ++i) {
        child[i] = windows_argument(child[i]);
        if (!child[i]) return 2;
    }
    i = (int)_spawnv(_P_WAIT, compiler, (const char *const *)child);
    {
        int j;
        for (j = 0; j < argc + 1; ++j) free(child[j]);
    }
    if (i >= 0) { free(child); return i; }
#else
    execv(child[0], child);
#endif
    perror("mf-classic-cc");
    free(child);
    return 2;
}
