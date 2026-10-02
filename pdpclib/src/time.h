/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  time.h - time header file.                                       */
/*                                                                   */
/*********************************************************************/

#ifndef __TIME_INCLUDED
#define __TIME_INCLUDED

#if !defined(__PDPCLIB_HEADFUNC) \
    && defined(__PDPCLIB_DLL) && !defined(__WATCOMC__) \
    && !defined(__NODECLSPEC__)
#ifdef __SCC__
#define __PDPCLIB_HEADFUNC __dllexport
#else
#define __PDPCLIB_HEADFUNC __declspec(dllexport)
#endif
#endif

#ifndef __PDPCLIB_HEADFUNC
#define __PDPCLIB_HEADFUNC
#endif

#define CLOCKS_PER_SEC 1000
#ifndef NULL
#define NULL ((void *)0)
#endif

typedef unsigned int clock_t;

#ifndef __SIZE_T_DEFINED
#define __SIZE_T_DEFINED
#if defined(__64BIT__)

#if defined(__LONG64__)
typedef unsigned long size_t;
#else
typedef unsigned long long size_t;
#endif

#elif (defined(__MVS__) \
    || defined(__MF32__) \
    || defined(__CMS__) || defined(__VSE__) || defined(__SMALLERC__) \
    || defined(__ARM__) \
    || defined(__SZ4__))
typedef unsigned long size_t;
#elif (defined(__MSDOS__) || defined(__DOS__) || defined(__POWERC) \
    || defined(__WIN32__) || defined(__AMIGA__) || defined(__EFI__) \
    || defined(__gnu_linux__) || defined(__ATARI__) \
    || defined(__OS2__) || defined(__32BIT__) || defined(__PDOS386__))
typedef unsigned int size_t;
#endif
#endif

#ifndef _UCRT
typedef unsigned long time_t;
#endif

struct tm
{
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

#ifdef _UCRT
/* 32-bit time_t has too small range
 * and there is going to be the 2038 problem,
 * so UCRT moved to 64-bit time_t (internally __time64_t).
 * _USE_32BIT_TIME_T is allowed as temporary solution
 * to avoid 64-bit time_t on 32-bit platforms.
 */
#ifndef _USE_32BIT_TIME_T
#ifndef __int64
#ifndef __NO_LONG_LONG
#define __int64 long long
#else
#define __int64 long
#endif
#endif /* __int64 */
typedef unsigned __int64 __time64_t;
typedef __time64_t time_t;
 
#define time _time64
#define difftime _difftime64
#define mktime _mktime64
#define ctime _ctime64
#define gmtime _gmtime64
#define localtime _localtime64
#else
#ifdef __64BIT__
#error _USE_32BIT_TIME_T is not allowed on 64-bit platforms.
#endif
typedef unsigned long time_t;

#define time _time32
#define difftime _difftime32
#define mktime _mktime32
#define ctime _ctime32
#define gmtime _gmtime32
#define localtime _localtime32
#endif /* _USE_32BIT_TIME_T */
#endif /* _UCRT */

__PDPCLIB_HEADFUNC time_t time(time_t *timer);
__PDPCLIB_HEADFUNC clock_t clock(void);
__PDPCLIB_HEADFUNC double difftime(time_t time1, time_t time0);
__PDPCLIB_HEADFUNC time_t mktime(struct tm *timeptr);
__PDPCLIB_HEADFUNC char *asctime(const struct tm *timeptr);
__PDPCLIB_HEADFUNC char *ctime(const time_t *timer);
__PDPCLIB_HEADFUNC struct tm *gmtime(const time_t *timer);
__PDPCLIB_HEADFUNC struct tm *localtime(const time_t *timer);
__PDPCLIB_HEADFUNC size_t strftime(char *s, size_t maxsize,
                const char *format, const struct tm *timeptr);


#if defined(__PDOSGEN__)
#include <__os.h>

#define ctime __os->Xctime
#define localtime __os->Xlocaltime
#define time __os->Xtime
#define mktime __os->Xmktime
#define clock __os->Xclock
#define strftime __os->Xstrftime

#endif


#endif
