#ifndef ATOMIC_BSD_H
#define ATOMIC_BSD_H

#define atomic_load_bool(obj) \
	__swirc_atomic_load_bool(obj)
#define atomic_swap_bool(obj, desired) \
	__swirc_atomic_swap_bool((obj), (desired))

static SW_INLINE bool
__swirc_atomic_load_bool(volatile bool *obj)
{
    return (__sync_val_compare_and_swap(obj, false, false));
}

static SW_INLINE bool
__swirc_atomic_swap_bool(volatile bool *obj, bool desired)
{
    return (__sync_lock_test_and_set(obj, desired));
}

#endif
