/*
 * Written by Markus Uhlin
 * (Public domain.)
 */

#ifndef S_ERRNO_TYPE_H
#define S_ERRNO_TYPE_H

#ifndef HAVE_ERRNO_TYPE
#if defined(__STDC_LIB_EXT1__) || defined(_WIN32)
#define HAVE_ERRNO_TYPE 1
#else
#define HAVE_ERRNO_TYPE 0
#endif
#endif

#if !(HAVE_ERRNO_TYPE)
typedef int errno_t;
#endif

#endif
