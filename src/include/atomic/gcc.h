#ifndef ATOMIC_GCC_H
#define ATOMIC_GCC_H

static SW_INLINE bool
atomic_load_bool(volatile bool *obj)
{
    return (__atomic_load_n(obj, __ATOMIC_SEQ_CST));
}

static SW_INLINE bool
atomic_swap_bool(volatile bool *obj, bool desired)
{
    return (__atomic_exchange_n(obj, desired, __ATOMIC_SEQ_CST));
}

#endif
