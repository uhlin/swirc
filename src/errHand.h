#ifndef SW_ERROR_HANDLING_H
#define SW_ERROR_HANDLING_H

/*lint -printf(1, err_dump, err_msg, err_quit, err_ret, err_sys) */
/*lint -printf(2, err_exit, err_log) */

/*lint -sem(err_dump, r_no) */
/*lint -sem(err_exit, r_no) */
/*lint -sem(err_quit, r_no) */
/*lint -sem(err_sys, r_no)  */

#ifdef __cplusplus
extern "C" {
#endif

NORETURN void	err_dump (const char *, ...) PRINTFLIKE(1);
NORETURN void	err_exit (int error, const char *, ...) PRINTFLIKE(2);
NORETURN void	err_quit (const char *, ...) PRINTFLIKE(1);
NORETURN void	err_sys  (const char *, ...) PRINTFLIKE(1);
void		err_log  (int error, const char *, ...) PRINTFLIKE(2);
void		err_msg  (const char *, ...) PRINTFLIKE(1);
void		err_ret  (const char *, ...) PRINTFLIKE(1);

#define MAXERROR 600

const char *errdesc_by_num(int);
const char *xstrerror(int errnum, char *strerrbuf, size_t buflen);

/*lint -printf(1, debug) */

void debug(const char *, ...) PRINTFLIKE(1);

#ifdef __cplusplus
}
#endif

#endif
