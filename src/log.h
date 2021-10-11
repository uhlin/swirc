#ifndef LOG_H
#define LOG_H

__SWIRC_BEGIN_DECLS
extern const char	g_log_filesuffix[];

char	*log_get_path(const char *, const char *);
void	 log_msg(const char *, const char *);
void	 log_toggle_on_off(void);
__SWIRC_END_DECLS

#endif
