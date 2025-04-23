#define DOES_NOT_WRITE_GLOBAL_DATA
#define NONNULL

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define NORETURN _Noreturn
#elif defined(__cplusplus) && __cplusplus >= 201103L
#define NORETURN [[noreturn]]
#else
#define NORETURN
#endif

#define NO_SIDE_EFFECT
#define PRINTFLIKE(arg_no)
#define SCANFLIKE(arg_no)
#define SW_INLINE inline
