#define DOES_NOT_WRITE_GLOBAL_DATA
#define NO_SIDE_EFFECT
#define PRINTFLIKE(arg_no)
#define PTR_ARGS_NONNULL
#define SW_INLINE inline
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define NORETURN _Noreturn
#elif defined(__cplusplus) && __cplusplus >= 201103L
#define NORETURN [[noreturn]]
#else
#define NORETURN
#endif
