#ifndef ATOMIC_OPERATIONS_HEADER
#define ATOMIC_OPERATIONS_HEADER
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && \
	!defined(__STDC_NO_ATOMICS__)
#include <stdatomic.h>
static SW_INLINE bool
atomic_swap_bool(volatile bool *obj, bool desired)
{
    return (atomic_exchange(obj, desired));
}
#elif defined(__cplusplus)
/* do something */
#elif defined(__GNUC__) && __GNUC__ >= 5
static SW_INLINE bool
atomic_swap_bool(volatile bool *obj, bool desired)
{
    return (__atomic_exchange_n(obj, desired, __ATOMIC_SEQ_CST));
}
#elif defined(BSD)
static SW_INLINE bool
atomic_swap_bool(volatile bool *obj, bool desired)
{
    return (__sync_lock_test_and_set(obj, desired));
}
#elif defined(WIN32)
static SW_INLINE bool
atomic_swap_bool(volatile bool *obj, bool desired)
{
    return (InterlockedExchange(obj, desired));
}
#else
#error FATAL: Lacking atomic operations!
#endif
#endif /* ATOMIC_OPERATIONS_HEADER */
