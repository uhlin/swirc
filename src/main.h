#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
#include <string>
#include <vector>
#endif

struct locale_info {
	char	*lang_and_territory;
	char	*codeset;

#ifdef __cplusplus
	locale_info()
	{
		this->lang_and_territory = NULL;
		this->codeset = NULL;
	}

	locale_info(char *str1, char *str2)
	{
		this->lang_and_territory = str1;
		this->codeset = str2;
	}
#endif
};

struct cmdline_opt_values {
	char	*server;
	char	*port;
	char	*nickname;
	char	*username;
	char	*rl_name;
	char	*hostname;
	char	*config_file;

#ifdef __cplusplus
	cmdline_opt_values()
	{
		this->server		= NULL;
		this->port		= NULL;
		this->nickname		= NULL;
		this->username		= NULL;
		this->rl_name		= NULL;
		this->hostname		= NULL;
		this->config_file	= NULL;
	}
#endif
};

typedef const char chararray_t[];
typedef const char *stringarray_t[];

__SWIRC_BEGIN_DECLS
extern chararray_t	g_swircVersion;
extern chararray_t	g_swircYear;
extern chararray_t	g_swircAuthor;
extern chararray_t	g_swircWebAddr;

extern char *g_progname;
extern long int g_pid;

#ifdef __cplusplus
extern std::vector<std::string> g_join_list;
#endif

extern bool	g_auto_connect;
extern bool	g_bind_hostname;
extern bool	g_change_color_defs;
extern bool	g_connection_password;
extern bool	g_debug_logging;
extern bool	g_explicit_config_file;
extern bool	g_icb_mode;
extern bool	g_sasl_authentication;
extern bool	g_ssl_verify_peer;

extern struct cmdline_opt_values *g_cmdline_opts;

struct locale_info *get_locale_info(int category);
void free_locale_info(struct locale_info *);
void cmdline_options_destroy(void);
__SWIRC_END_DECLS

#endif
