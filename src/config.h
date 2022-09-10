#ifndef CONFIG_H
#define CONFIG_H

#include "integer-context.h"
#include "textBuffer.h"

#define RECONNECT_BACKOFF_DELAY_DEFAULT 60
#define RECONNECT_DELAY_DEFAULT         10
#define RECONNECT_DELAY_MAX_DEFAULT     240
#define RECONNECT_RETRIES_DEFAULT       30

typedef struct tagCONF_HTBL_ENTRY {
	char *name;
	char *value;
	struct tagCONF_HTBL_ENTRY *next;
} CONF_HTBL_ENTRY, *PCONF_HTBL_ENTRY;

__SWIRC_BEGIN_DECLS

/*lint -sem(get_list_of_matching_settings, r_null) */

PTEXTBUF get_list_of_matching_settings(const char *);

void	config_init(void);
void	config_deinit(void);

/*lint -sem(Config_mod, r_null) */

bool		 config_bool(const char *, bool);
char		*Config_mod(const char *);
const char	*Config(const char *);
int		 config_item_install(const char *name, const char *value);
int		 config_item_undef(const char *name);
long int	 config_integer(const struct integer_context *);
void		 config_create(const char *path, const char *mode);
void		 config_do_save(const char *path, const char *mode);
void		 config_readit(const char *path, const char *mode);

char get_sasl_passwd_type(void);

#define SASL_USERNAME_MAXLEN 480
#define SASL_PASSWORD_MAXLEN 480

/*lint -sem(config_get_normalized_sasl_username, r_null) */
/*lint -sem(config_get_normalized_sasl_password, r_null) */

const char	*config_get_normalized_sasl_username(void);
const char	*config_get_normalized_sasl_password(void);

void cmd_set(const char *);

long int	get_reconnect_backoff_delay(void);
long int	get_reconnect_delay(void);
long int	get_reconnect_delay_max(void);
long int	get_reconnect_retries(void);

__SWIRC_END_DECLS

#endif /* CONFIG_H */
