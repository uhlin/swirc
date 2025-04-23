#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
#include <string>
#include <vector>
#endif

#include <stdio.h> /* FILE */

#if defined(UNIX)
#define DEV_NULL "/dev/null"
#elif defined(WIN32)
#define DEV_NULL "nul"
#endif

struct locale_info {
	char	*lang_and_territory;
	char	*codeset;

#ifdef __cplusplus
	locale_info()
	    : lang_and_territory(nullptr)
	    , codeset(nullptr)
	{}

	locale_info(char *str1, char *str2)
	    : lang_and_territory(str1)
	    , codeset(str2)
	{}

	~locale_info();
#endif
};

struct cmdline_opt_values {
	char	*server;
	char	*port;
	char	*passwd;
	char	*nickname;
	char	*username;
	char	*rl_name;
	char	*hostname;
	char	*config_file;

#ifdef __cplusplus
	cmdline_opt_values() : server(nullptr)
	    , port(nullptr)
	    , passwd(nullptr)
	    , nickname(nullptr)
	    , username(nullptr)
	    , rl_name(nullptr)
	    , hostname(nullptr)
	    , config_file(nullptr)
	{
		/* empty */;
	}

	~cmdline_opt_values();
#endif
};

typedef char *(*SETLOCALE_FN)(int, const char *);
typedef const char chararray_t[];
typedef const char *stringarray_t[];
typedef int (*SSCANF_FN)(const char *, const char *, ...);

/*lint -sem(xsetlocale, r_null) */

__SWIRC_BEGIN_DECLS
extern chararray_t	g_swircVersion;
extern chararray_t	g_swircYear;
extern chararray_t	g_swircAuthor;
extern chararray_t	g_swircWebAddr;

extern FILE		*g_dev_null;
extern STRING		 g_progname;
extern STRING		 g_progpath;
extern long int		 g_pid;

extern int	 g_stderr_fd;
extern int	 g_stdout_fd;

extern SSCANF_FN xsscanf;

extern SETLOCALE_FN	 xsetlocale;
extern char		 g_locale[200];

#ifdef __cplusplus
extern std::vector<std::string> g_join_list;
#endif

extern bool	g_auto_connect;
extern bool	g_bind_hostname;
extern bool	g_change_color_defs;
extern bool	g_connection_password;
extern bool	g_debug_logging;
extern bool	g_explicit_config_file;
extern bool	g_force_tls;
extern bool	g_icb_mode;
extern bool	g_ircv3_extensions;
extern bool	g_sasl_authentication;
extern bool	g_ssl_verify_peer;

extern struct cmdline_opt_values *g_cmdline_opts;

struct locale_info *
	 get_locale_info(int category);
void	 free_locale_info(struct locale_info *);
void	 cmdline_options_destroy(void);
void	 redir_stderr(void);
void	 restore_stderr(void);
__SWIRC_END_DECLS

#endif
