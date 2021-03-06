#ifndef NEST_HOME_H
#define NEST_HOME_H

#ifdef __cplusplus
extern "C" {
#endif

extern char *g_home_dir;
extern char *g_tmp_dir;
extern char *g_log_dir;

extern const char g_config_filesuffix[];
extern const char g_theme_filesuffix[];

extern char *g_config_file;
extern char *g_theme_file;

void nestHome_init   (void);
void nestHome_deinit (void);

/*lint -sem(path_to_home, r_null) */

const char *path_to_home(void);

#ifdef __cplusplus
}
#endif

#endif
