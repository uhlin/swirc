#ifndef SW_ERROR_HANDLING_H
#define SW_ERROR_HANDLING_H

/*lint -printf(1, err_dump, err_msg, err_quit, err_ret, err_sys) */
/*lint -printf(2, err_exit, err_log) */

/*lint -sem(err_dump, r_no) */
/*lint -sem(err_exit, r_no) */
/*lint -sem(err_quit, r_no) */
/*lint -sem(err_sys, r_no)  */

SW_NORET void	err_dump (const char *fmt, ...) PRINTFLIKE(1);
SW_NORET void	err_exit (int error, const char *fmt, ...) PRINTFLIKE(2);
SW_NORET void	err_quit (const char *fmt, ...) PRINTFLIKE(1);
SW_NORET void	err_sys  (const char *fmt, ...) PRINTFLIKE(1);
void		err_log  (int error, const char *fmt, ...) PRINTFLIKE(2);
void		err_msg  (const char *fmt, ...) PRINTFLIKE(1);
void		err_ret  (const char *fmt, ...) PRINTFLIKE(1);

const char *errdesc_by_num(int);

#endif
