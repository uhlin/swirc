#ifndef ATOMIC_OPERATIONS_HEADER
#define ATOMIC_OPERATIONS_HEADER
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && \
	!defined(__STDC_NO_ATOMICS__)
#include "atomic/c11.h"
#elif defined(__cplusplus)
#include "atomic/c++.h"
#elif defined(__GNUC__) && __GNUC__ >= 5
#include "atomic/gcc.h"
#elif defined(BSD)
#include "atomic/bsd.h"
#elif defined(WIN32)
#include "atomic/w32.h"
#else
#error FATAL: Lacking atomic operations!
#endif
#endif /* ATOMIC_OPERATIONS_HEADER */
