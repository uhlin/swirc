#define DOES_NOT_WRITE_GLOBAL_DATA	__attribute__((pure))
#define NO_SIDE_EFFECT			__attribute__((const))
#define PRINTFLIKE(arg_no)		__attribute__((format(printf, arg_no, arg_no + 1)))
#define PTR_ARGS_NONNULL		__attribute__((nonnull))
#define SW_INLINE			inline
#define NORETURN			__attribute__((noreturn))
