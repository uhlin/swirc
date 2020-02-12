#ifndef LOG_H
#define LOG_H

#ifdef __cplusplus
extern "C" {
#endif

extern const char g_log_filesuffix[];

char	*log_get_path(const char *server_host, const char *label);
void	 log_msg(const char *path, const char *text);
void	 log_toggle_on_off(void);

#ifdef __cplusplus
}
#endif

#endif
