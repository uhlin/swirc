#ifndef COMMON_H
#define COMMON_H

#if !defined(UNIX) && !defined(WIN32)
#error PLATFORM UNSUPPORTED
#elif defined(UNIX) && defined(WIN32)
#error INCOMPATIBLE BUILD TARGETS
#else
#
#endif

/* Define HAVE_BCI (aka bounds-checking interfaces) if appropriate.
   Unfortunately the MSVC implementation of BCI might differ slightly
   compared to the C11 standard.  Check if a function differ before
   using this macro. */
#if defined(__STDC_LIB_EXT1__) || defined(WIN32)
#define HAVE_BCI 1

#ifdef UNIX
#define __STDC_WANT_LIB_EXT1__ 1
#endif
#endif

#if defined(UNIX) && defined(__GNUC__)
#include "gnuattrs.h"
#elif defined(UNIX) && (defined(__SUNPRO_C) || defined(__SUNPRO_CC))
#include "sunattrs.h"
#elif defined(WIN32)
#include "winattrs.h"
#else
#include "fallbackattrs.h"
#endif

#if defined(UNIX)
#define SLASH "/"
#elif defined(WIN32)
#define SLASH "\\"
#endif

#define UNUSED_PARAM(p) ((void) (p))
#define UNUSED_VAR(v) ((void) (v))

#define ARRAY_SIZE(ar)	(sizeof(ar) / sizeof((ar)[0]))
#define BZERO(b, len)	((void) memset(b, 0, len))
#define STRING(x)	#x
#define STRINGIFY(x)	STRING(x)
#define addrof(x)	(&(x))

#ifdef WIN32
#define strcasecmp	_stricmp
#define strncasecmp	_strnicmp
#define strtok_r	strtok_s
#endif

#if defined(WIN32) && !defined(_SSIZE_T_DEFINED)
#define _SSIZE_T_DEFINED 1
typedef long int ssize_t;
#endif

#if defined(WIN32) && !defined(__func__)
#define __func__ __FUNCTION__
#endif

#ifdef __cplusplus
#define __SWIRC_BEGIN_DECLS	extern "C" {
#define __SWIRC_END_DECLS	}
#else
#define __SWIRC_BEGIN_DECLS
#define __SWIRC_END_DECLS
#endif

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef __NetBSD__
#include <wchar.h>
#endif

#endif
