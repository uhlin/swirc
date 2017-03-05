#ifndef MAIN_H
#define MAIN_H

struct locale_info {
    char *lang_and_territory;
    char *codeset;
};

struct cmdline_opt_values {
    char *server;
    char *port;
    char *nickname;
    char *username;
    char *rl_name;
    char *hostname;
    char *config_file;
};

extern const char g_swircVersion[];
extern const char g_swircYear[];
extern const char g_swircAuthor[];
extern const char g_swircWebAddr[];

extern bool g_auto_connect;
extern bool g_icb_mode;
extern bool g_connection_password;
extern bool g_bind_hostname;
extern bool g_explicit_config_file;

extern struct cmdline_opt_values *g_cmdline_opts;

struct locale_info	*get_locale_info(int category);
void			 free_locale_info(struct locale_info *);
void			 cmdline_options_destroy(void);

#endif
