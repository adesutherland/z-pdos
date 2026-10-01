#include "../string.h"
#if defined(__PDOS390__)
#ifdef memcpy
#error PDOS390 must use the library memcpy declaration
#endif
#ifdef memcmp
#error PDOS390 must use the library memcmp declaration
#endif
#else
#ifndef memcpy
#error The GCC control must retain its existing memcpy builtin selection
#endif
#ifndef memcmp
#error The GCC control must retain its existing memcmp builtin selection
#endif
#endif

int main(void)
{
    static const char original[] = "abcde";
    char copied[6];
    size_t i;
    if (memcpy(copied, original, sizeof original) != copied) return 1;
    for (i = 0; i < sizeof original; ++i) {
        if (copied[i] != original[i]) return 2;
    }
    if (memcmp(copied, original, sizeof original) != 0) return 3;
    copied[3] = 'c';
    if (memcmp(copied, original, sizeof original) >= 0) return 4;
    if (memcmp(original, copied, sizeof original) <= 0) return 5;
    return 0;
}
