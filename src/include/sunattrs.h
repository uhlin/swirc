/**
 * Asserts that the specified routine do not write global data
 * directly or indirectly. This behavior results in better
 * optimization of code around calls to such routines. In particular,
 * assignment statements or stores could be moved around such calls.
 *
 * If the assertion about global access is not true, then the behavior
 * of the program is undefined.
 *
 * If a function takes a non-const pointer argument it must not modify
 * the array it points to, or any other object whose value the rest of
 * the program may depend on. However, the caller may safely change
 * the contents of the array between successive calls to the function
 * (doing so disables the optimization).
 *
 * Because a pure function cannot have any observable side effects it
 * does not make sense for such a function to return void.
 */
#define DOES_NOT_WRITE_GLOBAL_DATA	__attribute__((pure))

/**
 * Declares that the function has no side effects of any kind and
 * returns a result value that depends only on the passed
 * arguments. In addition, the function and any called descendants
 * behave as follows:
 *
 *   1. Do not access for reading or writing any part of the program
 *   state visible in the caller at the point of the call.
 *
 *   2. Do not perform I/O.
 *
 *   3. Do not change any part of the program state not visible at the
 *   point of the call.
 *
 * If the function does have side effects, the results of executing a
 * program that calls this function are undefined. The compiler takes
 * advantage of this information at optimization level of 3 or above.
 *
 * Note that a function that has pointer arguments and examines the
 * data pointed to must not be declared const if the pointed-to data
 * might change between successive invocations of the function. In
 * general, since a function cannot distinguish data that might change
 * from data that cannot, const functions should never take pointer
 * or, in C++, reference arguments.
 */
#define NO_SIDE_EFFECT			__attribute__((const))

#define PRINTFLIKE(arg_no)
#define PTR_ARGS_NONNULL
#define SW_INLINE			inline
#define NORETURN			__attribute__((noreturn))
