/* SPDX-License-Identifier: MIT */
#ifdef TEST_CMS
#ifndef __CMS__
#error CMS target macro missing
#endif
#ifdef __MVS__
#error CMS target must not claim MVS
#endif
#else
#ifndef __MVS__
#error MVS target macro missing
#endif
#endif
typedef char int32[sizeof(int) == 4 ? 1 : -1];
typedef char long32[sizeof(long) == 4 ? 1 : -1];
typedef char pointer32[sizeof(void *) == 4 ? 1 : -1];
typedef char longlong64[sizeof(long long) == 8 ? 1 : -1];
typedef char unsigned_plain_char[(char)255 > 0 ? 1 : -1];
