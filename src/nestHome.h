#ifndef NEST_HOME_H
#define NEST_HOME_H

__SWIRC_BEGIN_DECLS
extern char	*g_encrypted_sasl_pass;
extern char	*g_user;

extern char	*g_home_dir;
extern char	*g_tmp_dir;
extern char	*g_log_dir;

extern char	*g_dcc_download_dir;
extern char	*g_dcc_upload_dir;

extern const char	g_config_filesuffix[6];
extern const char	g_theme_filesuffix[6];

extern char	*g_config_file;
extern char	*g_theme_file;

void	nestHome_init(void);
void	nestHome_deinit(void);

/*lint -sem(path_to_downloads, r_null) */
/*lint -sem(path_to_home, r_null) */

char		*path_to_downloads(void);
const char	*path_to_home(void);
__SWIRC_END_DECLS

#endif
