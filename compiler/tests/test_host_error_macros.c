/* SPDX-License-Identifier: MIT
 * Exercise the maintained libiberty source with a function-backed CRT table.
 */
#include <errno.h>
#include <string.h>
/* Use only the exercised declarations; unrelated legacy interfaces remain
   outside this strict host fixture. These match src/include/libiberty.h. */
extern int errno_max(void);
extern const char *strerrno(int);
extern int strtoerrno(const char *);

int *mfqa_sys_nerr(void)
{
    static int count = 3;
    return &count;
}

char **mfqa_sys_errlist(void)
{
    static char *messages[] = {"ok", "permission denied", "missing file"};
    return messages;
}

int main(void)
{
    const char *name = strerrno(ENOENT);
    if (!name || strcmp(name, "ENOENT")) return 1;
    if (strtoerrno("ENOENT") != ENOENT) return 2;
    if (errno_max() < ENOENT) return 3;
    return 0;
}
