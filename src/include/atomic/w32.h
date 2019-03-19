#ifndef ATOMIC_WIN32_H
#define ATOMIC_WIN32_H

#include <intrin.h>

static SW_INLINE bool
atomic_load_bool(volatile bool *obj)
{
    static_assert(sizeof (bool) == 1, "bool not 8 bits");
    return (_InterlockedCompareExchange8((volatile char *) obj, false, false));
}

static SW_INLINE bool
atomic_swap_bool(volatile bool *obj, bool desired)
{
    static_assert(sizeof (bool) == 1, "bool not 8 bits");
    return (_InterlockedExchange8((volatile char *) obj, desired));
}

#endif
