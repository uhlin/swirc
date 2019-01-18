#define PRINTFLIKE(arg_no)	__attribute__((format(printf, arg_no, arg_no + 1)))
#define PTR_ARGS_NONNULL	__attribute__((nonnull))
#define PURE			__attribute__((pure))
#define SW_INLINE		inline
#define SW_NORET		__attribute__((noreturn))
