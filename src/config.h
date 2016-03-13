#ifndef CONFIG_H
#define CONFIG_H

#include "int_unparse.h"

typedef struct tagCONF_HTBL_ENTRY {
    char *name;
    char *value;
    struct tagCONF_HTBL_ENTRY *next;
} CONF_HTBL_ENTRY, *PCONF_HTBL_ENTRY;

/*lint -sem(Config_mod, r_null) */

bool		 config_bool_unparse    (const char *setting_name, bool fallback_default);
char		*Config_mod             (const char *setting_name);
const char	*Config                 (const char *setting_name);
int		 config_item_install    (const char *name, const char *value);
int		 config_item_undef      (const char *name);
long int	 config_integer_unparse (struct integer_unparse_context *);
#if 0
short int	 config_color_unparse   (const char *setting_name, short int fallback_color);
#endif
void		 config_create          (const char *path, const char *mode);
void		 config_deinit          (void);
void		 config_do_save         (const char *path, const char *mode);
void		 config_init            (void);
void		 config_readit          (const char *path, const char *mode);

#endif
