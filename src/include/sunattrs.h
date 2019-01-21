/**
 * Asserts that the specified routine do not write global data
 * directly or indirectly. This behavior results in better
 * optimization of code around calls to such routines. In particular,
 * assignment statements or stores could be moved around such calls.
 *
 * If the assertion about global access is not true, then the behavior
 * of the program is undefined.
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
 */
#define NO_SIDE_EFFECT			__attribute__((const))

#define PRINTFLIKE(arg_no)
#define PTR_ARGS_NONNULL
#define SW_INLINE			inline
#define SW_NORET			__attribute__((noreturn))
