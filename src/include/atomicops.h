#ifndef ATOMIC_OPERATIONS_HEADER
#define ATOMIC_OPERATIONS_HEADER

#ifndef __has_builtin
#define __has_builtin(func) 0
#endif

#if defined(__GNUC__) && __GNUC__ >= 5
#include "atomic/gcc.h"
#elif defined(BSD)
#include "atomic/bsd.h"
#elif defined(WIN32)
#include "atomic/w32.h"
#else
#error FATAL: Lacking atomic operations!
#endif
#endif /* ATOMIC_OPERATIONS_HEADER */
