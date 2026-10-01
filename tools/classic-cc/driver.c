/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Adrian Sutherland
 * Development-host launcher. The bootstrap assembler has no POSIX dependency;
 * this launcher for the native GCC cross-compiler uses the host exec interface.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef MF_CC_DIR
#error MF_CC_DIR must name the private GCC build directory
#endif

int main(int argc, char **argv)
{
    char **child;
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
    child = (char **)calloc((size_t)argc + 2, sizeof(*child));
    if (!child) return 2;
    child[0] = MF_CC_DIR "/xgcc";
    child[1] = "-B" MF_CC_DIR "/";
    for (i = 1; i < argc; ++i) child[i + 1] = argv[i];
    execv(child[0], child);
    perror("mf-classic-cc");
    free(child);
    return 2;
}
