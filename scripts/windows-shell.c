/* SPDX-License-Identifier: MIT
 * Native Windows adapter for the raw ADDRESS SHELL command interface.
 * Keep the complete shell program as one Bash -c argument.
 */
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *argument(const char *text)
{
    size_t n = strlen(text), slashes;
    char *result = (char *)malloc(n * 2 + 3), *out;
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
    *out++ = '"'; *out = '\0';
    return result;
}

int main(void)
{
    const char *bash = getenv("PDOS_WINDOWS_BASH"), *command = GetCommandLineA();
    const char *args[4];
    char *exe_arg, *command_arg;
    intptr_t status;
    if (!bash || !*bash) { fputs("PDOS_WINDOWS_BASH is missing\n", stderr); return 2; }
    /* Skip this executable and the explicit -- marker, retaining all quotes. */
    if (*command == '"') {
        ++command;
        while (*command && *command != '"') ++command;
        if (*command) ++command;
    } else while (*command && *command != ' ' && *command != '\t') ++command;
    while (*command == ' ' || *command == '\t') ++command;
    if (strncmp(command, "--", 2) || (command[2] != ' ' && command[2] != '\t')) return 2;
    command += 2;
    while (*command == ' ' || *command == '\t') ++command;
    exe_arg = argument(bash); command_arg = argument(command);
    if (!exe_arg || !command_arg) { free(exe_arg); free(command_arg); return 2; }
    args[0] = exe_arg; args[1] = "-c"; args[2] = command_arg; args[3] = NULL;
    status = _spawnv(_P_WAIT, bash, args);
    free(exe_arg); free(command_arg);
    if (status == -1) { perror("MSYS2 Bash"); return 2; }
    return (int)status;
}
