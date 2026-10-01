/* SPDX-License-Identifier: MIT */
#define STEP (*p += x)
#define S2 STEP; STEP
#define S4 S2; S2
#define S8 S4; S4
#define S16 S8; S8
#define S32 S16; S16
#define S64 S32; S32
#define S128 S64; S64
#define S256 S128; S128
#define S512 S256; S256
int dispatch(int x, volatile int *p)
{
    static void *targets[] = { &&done, &&again };
    goto *targets[x & 1];
again:
    S512;
done:
    return *p;
}
