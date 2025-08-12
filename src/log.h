#ifndef LOG_H
#define LOG_H

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#if WIN32
#include <io.h>
#include <share.h>
#endif
#if UNIX
#include <unistd.h>
#endif

__SWIRC_BEGIN_DECLS
extern const char	g_log_filesuffix[5];

char	*log_get_path(const char *, const char *);
void	 log_msg(const char *, const char *);
void	 log_toggle_on_off(void);
__SWIRC_END_DECLS

#endif
