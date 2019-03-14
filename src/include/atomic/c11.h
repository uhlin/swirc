#ifndef ATOMIC_C11_H
#define ATOMIC_C11_H

#include <stdatomic.h>

static SW_INLINE bool
atomic_load_bool(volatile bool *obj)
{
    return (atomic_load(obj));
}

static SW_INLINE bool
atomic_swap_bool(volatile bool *obj, bool desired)
{
    return (atomic_exchange(obj, desired));
}

#endif
