#ifndef SRC_INCLUDE_COMMON_H_
#define SRC_INCLUDE_COMMON_H_
/* common.h
   Copyright (C) 2016-2024 Markus Uhlin. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

   - Neither the name of the author nor the names of its contributors may be
     used to endorse or promote products derived from this software without
     specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

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

#if defined(__cplusplus) && __cplusplus >= 201703L
#define HAVE_STD_FS 1
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

#if defined(__cplusplus) && __cplusplus >= 201703L
#define MAYBE_UNUSED [[maybe_unused]]
#else
#define MAYBE_UNUSED
#endif

#if defined(UNIX)
#define SLASH "/"
#define SLASH_CHAR '/'
#elif defined(WIN32)
#define SLASH "\\"
#define SLASH_CHAR '\\'
#endif

#define UNUSED_PARAM(p) ((void) (p))
#define UNUSED_VAR(v) ((void) (v))

#define BZERO(b, len)	((void) memset(b, 0, len))
#define QUOTE(x)	#x
#define STRINGIFY(x)	QUOTE(x)
#define addrof(x)	(&(x))

#ifdef WIN32
#define strcasecmp	_stricmp
#define strncasecmp	_strnicmp
#define strtok_r	strtok_s
#endif

typedef char *STRING;
typedef const char *CSTRING;
typedef const char *const immutable_cp_t;

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

#include "errno-type.h"
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef __NetBSD__
#include <wchar.h>
#endif

#if defined(__cplusplus) && __cplusplus >= 201103L
template <typename T, size_t N>
constexpr size_t ARRAY_SIZE(T (&_array)[N]) noexcept
{
	return N;
}
#else
#define ARRAY_SIZE(_a)	(sizeof(_a) / sizeof((_a)[0]))
#endif

#endif
