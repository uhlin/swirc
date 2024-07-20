#ifndef ATOMIC_OPERATIONS_HEADER
#define ATOMIC_OPERATIONS_HEADER
/* atomicops.h
   Copyright (C) 2019, 2024 Markus Uhlin. All rights reserved.

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

#ifndef __has_builtin
#define __has_builtin(func) 0
#endif

#if defined(__GNUC__) && __GNUC__ >= 5
#define HAVE_GCC_ATOMICS 1
#elif	defined(__GNUC__) &&\
	defined(__GNUC_MINOR__) &&\
	__GNUC__ >= 4 &&\
	__GNUC_MINOR__ >= 8
#define HAVE_GCC_ATOMICS 1
#elif	defined(__clang__) &&\
	__has_builtin(__atomic_load_n) &&\
	__has_builtin(__atomic_exchange_n)
#define HAVE_GCC_ATOMICS 1
#elif	(defined(__SUNPRO_C) && __SUNPRO_C >= 0x5150) ||\
	(defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x5150)
#define HAVE_GCC_ATOMICS 1
#else
#define HAVE_GCC_ATOMICS 0
#endif

#if HAVE_GCC_ATOMICS
#include "atomic/gcc.h"
#elif defined(BSD) || defined(OS_X)
#include "atomic/bsd.h"
#elif defined(WIN32)
#include "atomic/w32.h"
#else
#error FATAL: Lacking atomic operations!
#endif

#ifdef __cplusplus
#if __cplusplus > 201103L
#include <atomic>
#ifdef _Atomic
#
#else
#define _Atomic(x) std::atomic<x>
#endif
#endif
#else /* C */
#if defined(UNIX) && defined(__STDC_NO_ATOMICS__)
#define _Atomic(x) x
#else
#include <stdatomic.h>
#endif
#endif

#endif /* ATOMIC_OPERATIONS_HEADER */
