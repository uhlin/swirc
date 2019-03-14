#ifndef ATOMIC_BSD_H
#define ATOMIC_BSD_H

static SW_INLINE bool
atomic_load_bool(volatile bool *obj)
{
    return (__sync_val_compare_and_swap(obj, false, false));
}

static SW_INLINE bool
atomic_swap_bool(volatile bool *obj, bool desired)
{
    return (__sync_lock_test_and_set(obj, desired));
}

#endif
