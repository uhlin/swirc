#ifndef ATOMIC_OPERATIONS_HEADER
#define ATOMIC_OPERATIONS_HEADER

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
#endif /* ATOMIC_OPERATIONS_HEADER */
