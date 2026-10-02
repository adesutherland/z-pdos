/* SPDX-License-Identifier: MIT
 * Independent Windows CRT argument vectors, executable on every host. */
#define MF_TEST_WINDOWS_QUOTING 1
#define main imported_driver_main
#include "../src/driver.c"
#undef main

int main(void)
{
    static const char *inputs[] = {
        "", "plain", "path with spaces", "C:\\folder name\\", "a\"b", "a\\\"b"
    };
    static const char *expected[] = {
        "\"\"", "\"plain\"", "\"path with spaces\"",
        "\"C:\\folder name\\\\\"", "\"a\\\"b\"", "\"a\\\\\\\"b\""
    };
    size_t i;
    for (i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i) {
        char *quoted = windows_argument(inputs[i]);
        if (!quoted || strcmp(quoted, expected[i])) {
            fprintf(stderr, "Windows argument quoting failed at vector %lu\n",
                    (unsigned long)i);
            free(quoted);
            return 1;
        }
        free(quoted);
    }
    puts("Windows CRT spaces, quotes and trailing backslashes pass");
    return 0;
}
