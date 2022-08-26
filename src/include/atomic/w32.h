#ifndef ATOMIC_WIN32_H
#define ATOMIC_WIN32_H

#include <intrin.h>

#ifdef __cplusplus
#define VCP_OBJ(_o)	(reinterpret_cast<volatile char *>(_o))
#else
#define VCP_OBJ(_o)	((volatile char *) (_o))
#endif

static inline bool
atomic_load_bool(volatile bool *obj)
{
	static_assert(sizeof(bool) == 1, "bool not 8 bits");
	return (_InterlockedCompareExchange8(VCP_OBJ(obj), false, false));
}

static inline bool
atomic_swap_bool(volatile bool *obj, bool desired)
{
	static_assert(sizeof(bool) == 1, "bool not 8 bits");
	return (_InterlockedExchange8(VCP_OBJ(obj), desired));
}

#endif
