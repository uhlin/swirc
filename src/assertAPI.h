#ifndef ASSERT_API_H
#define ASSERT_API_H

/*lint -sem(SWAssertFail, r_no)           */
/*lint -sem(SWAssertPerrorFail, r_no)     */
/*lint -sem(SWAssertNotReachedFail, r_no) */

#ifdef __cplusplus
extern "C" {
#endif

NORETURN void SWAssertFail           (const char *file, long int line, const char *fn, const char *assertion);
NORETURN void SWAssertPerrorFail     (const char *file, long int line, const char *fn, int errnum);
NORETURN void SWAssertNotReachedFail (const char *file, long int line, const char *fn);

#ifdef __cplusplus
}
#endif

#if defined(UNIX)
#define SW_ASSERT_FN __func__
#elif defined(WIN32)
#define SW_ASSERT_FN __FUNCTION__
#endif

#ifndef NDEBUG
#define sw_assert(expr) \
	((expr)         \
	 ? (void) 0     \
	 : SWAssertFail(__FILE__, __LINE__, SW_ASSERT_FN, #expr))

#define sw_assert_perror(errnum) \
	((errnum == 0)           \
	 ? (void) 0              \
	 : SWAssertPerrorFail(__FILE__, __LINE__, SW_ASSERT_FN, errnum))

#define sw_assert_not_reached() \
	SWAssertNotReachedFail(__FILE__, __LINE__, SW_ASSERT_FN)
#endif

#ifdef NDEBUG
#define sw_assert(expr)          ((void) 0)
#define sw_assert_perror(errnum) ((void) 0)
#define sw_assert_not_reached()  SWAssertNotReachedFail(__FILE__, __LINE__, SW_ASSERT_FN)
#endif

/*
 * Define sw_static_assert()
 */
#if defined(WIN32) || (defined(__cplusplus) && __cplusplus >= 201103L)
#define sw_static_assert(expr, msg) \
	static_assert((expr), msg)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define sw_static_assert(expr, msg) \
	_Static_assert((expr), msg)
#else
#pragma message("warning: sw_static_assert() is a no operation!")
#define sw_static_assert(expr, msg) ((void) 0)
#endif

#endif
