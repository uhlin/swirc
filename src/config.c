/* User configuration
   Copyright (C) 2012-2016 Markus Uhlin. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

   - Neither the name of the author nor the names of its contributors may be
     used to endorse or promote products derived from this software without
     specific prior written permission.

   THIS SOFTWARE IS PROVIDED THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

#include "common.h"
#include "config.h"
#include "errHand.h"
#include "interpreter.h"
#include "libUtils.h"
#include "main.h"
#include "strHand.h"

#define ENTRY_FOREACH(entry_p) \
	for (entry_p = &hash_table[0]; \
	     entry_p < &hash_table[ARRAY_SIZE(hash_table)]; \
	     entry_p++)

#define WRITE_ITEM(name, value) \
	write_to_stream(fp, "%s = \"%s\";\n", name, value)

/* Objects with internal linkage
   ============================= */

static PCONF_HTBL_ENTRY hash_table[300];

static struct tagConfDefValues {
    char		*setting_name;
    enum setting_type	 type;
    char		*value;
} ConfDefValues[] = {
    { "alt_nick",                  TYPE_STRING,  "warezkid_" },
    { "cmd_hist_size",             TYPE_INTEGER, "50" },
    { "connection_timeout",        TYPE_INTEGER, "45" },
    { "disable_beeps",             TYPE_BOOLEAN, "no" },
    { "encoding",                  TYPE_STRING,  "iso-8859-1" },
    { "kick_close_window",         TYPE_BOOLEAN, "yes" },
    { "max_chat_windows",          TYPE_INTEGER, "60" },
    { "nickname",                  TYPE_STRING,  "warezkid" },
    { "part_message",              TYPE_STRING,  "" },
    { "quit_message",              TYPE_STRING,  "leaving" },
    { "real_name",                 TYPE_STRING,  "pinball" },
    { "recode",                    TYPE_BOOLEAN, "no" },
    { "show_ping_pong",            TYPE_BOOLEAN, "no" },
    { "skip_motd",                 TYPE_BOOLEAN, "no" },
    { "ssl_verify_peer",           TYPE_BOOLEAN, "yes" },
    { "startup_greeting",          TYPE_BOOLEAN, "yes" },
    { "textbuffer_size_absolute",  TYPE_INTEGER, "500" },
    { "theme",                     TYPE_STRING,  "default" },
    { "username",                  TYPE_STRING,  "swift" },
};

/* Static function declarations
   ============================ */

/*lint -sem(get_hash_table_entry, r_null) */

static PCONF_HTBL_ENTRY	get_hash_table_entry (const char *name);
static bool		is_recognized_setting(const char *setting_name);
static unsigned int	hash                 (const char *setting_name);
static void		hInstall             (const char *name, const char *value);
static void		hUndef               (PCONF_HTBL_ENTRY);
static void		init_missing_to_defs (void);

void
config_init(void)
{
    PCONF_HTBL_ENTRY *entry_p;

    ENTRY_FOREACH(entry_p) {
	*entry_p = NULL;
    }
}

void
config_deinit(void)
{
    PCONF_HTBL_ENTRY *entry_p;
    PCONF_HTBL_ENTRY p, tmp;

    ENTRY_FOREACH(entry_p) {
	for (p = *entry_p; p != NULL; p = tmp) {
	    tmp = p->next;
	    hUndef(p);
	}
    }
}

static void
hUndef(PCONF_HTBL_ENTRY entry)
{
    PCONF_HTBL_ENTRY tmp;
    unsigned int hashval = hash(entry->name);

    if ((tmp = hash_table[hashval]) == entry) {
	hash_table[hashval] = entry->next;
    } else {
	while (tmp->next != entry) {
	    tmp = tmp->next;
	}

	tmp->next = entry->next;
    }

    free_not_null(entry->name);
    free_not_null(entry->value);
    free_not_null(entry);
}

static void
hInstall(const char *name, const char *value)
{
    PCONF_HTBL_ENTRY item;
    const bool has_no_value = (value == NULL || *value == '\0');
    unsigned int hashval;

    item	= xcalloc(sizeof *item, 1);
    item->name	= sw_strdup(name);
    item->value	= sw_strdup(has_no_value ? "" : value);

    hashval             = hash(name);
    item->next          = hash_table[hashval];
    hash_table[hashval] = item;
}

/* hash pjw */
static unsigned int
hash(const char *setting_name)
{
    char c;
    unsigned int hashval = 0;
    unsigned int tmp;

    while ((c = *setting_name++) != 0) {
	hashval = (hashval << 4) + c;
	tmp = hashval & 0xf0000000;

	if (tmp) {
	    hashval ^= (tmp >> 24);
	    hashval ^= tmp;
	}
    }

    return (hashval % ARRAY_SIZE(hash_table));
}

int
config_item_undef(const char *name)
{
    PCONF_HTBL_ENTRY entry;

    if ((entry = get_hash_table_entry(name)) == NULL)
	return (ENOENT);

    hUndef(entry);
    return (0);
}

int
config_item_install(const char *name, const char *value)
{
    if (!name || !value) {
	return (EINVAL);
    } else if (get_hash_table_entry(name)) {
	return (EBUSY);
    } else {
	hInstall(name, value);
    }

    return (0);
}

static PCONF_HTBL_ENTRY
get_hash_table_entry(const char *name)
{
    PCONF_HTBL_ENTRY entry;

    if (!name)
	return (NULL);

    for (entry = hash_table[hash(name)]; entry != NULL; entry = entry->next) {
	if (Strings_match(name, entry->name))
	    return (entry);
    }

    return (NULL);
}

const char *
Config(const char *setting_name)
{
    PCONF_HTBL_ENTRY item;

    if (!setting_name)
	return ("");

    for (item = hash_table[hash(setting_name)]; item != NULL; item = item->next) {
	if (Strings_match(setting_name, item->name))
	    return (item->value);
    }

    return ("");
}

char *
Config_mod(const char *setting_name)
{
    PCONF_HTBL_ENTRY item;

    if (!setting_name)
	return (NULL);

    for (item = hash_table[hash(setting_name)]; item != NULL; item = item->next) {
	if (Strings_match(setting_name, item->name))
	    return (item->value);
    }

    return (NULL);
}

#if 0
short int
config_color_unparse(const char *setting_name, short int fallback_color)
{
    PCONF_HTBL_ENTRY item;

    if (!setting_name)
	err_exit(EINVAL, "config_color_unparse");

    for (item = hash_table[hash(setting_name)]; item != NULL; item = item->next) {
	if (Strings_match(setting_name, item->name)) {
	    if (Strings_match_ignore_case(item->value, "black"))        return (COLOR_BLACK);
	    else if (Strings_match_ignore_case(item->value, "red"))     return (COLOR_RED);
	    else if (Strings_match_ignore_case(item->value, "green"))   return (COLOR_GREEN);
	    else if (Strings_match_ignore_case(item->value, "yellow"))  return (COLOR_YELLOW);
	    else if (Strings_match_ignore_case(item->value, "blue"))    return (COLOR_BLUE);
	    else if (Strings_match_ignore_case(item->value, "magenta")) return (COLOR_MAGENTA);
	    else if (Strings_match_ignore_case(item->value, "cyan"))    return (COLOR_CYAN);
	    else if (Strings_match_ignore_case(item->value, "white"))   return (COLOR_WHITE);
	    else break;
	}
    }

    return (fallback_color);
}
#endif

bool
config_bool_unparse(const char *setting_name, bool fallback_default)
{
    PCONF_HTBL_ENTRY item;

    if (!setting_name)
	err_exit(EINVAL, "config_bool_unparse");

    for (item = hash_table[hash(setting_name)]; item != NULL; item = item->next) {
	if (Strings_match(setting_name, item->name)) {
	    if (Strings_match_ignore_case(item->value, "on") ||
		Strings_match_ignore_case(item->value, "true") ||
		Strings_match_ignore_case(item->value, "yes")) {
		return (true);
	    } else if (Strings_match_ignore_case(item->value, "off") ||
		       Strings_match_ignore_case(item->value, "false") ||
		       Strings_match_ignore_case(item->value, "no")) {
		return (false);
	    } else {
		break;
	    }
	}
    }

    return (fallback_default);
}

long int
config_integer_unparse(struct integer_unparse_context *ctx)
{
    PCONF_HTBL_ENTRY item;
    long int val;

    if (!ctx)
	err_exit(EINVAL, "config_integer_unparse");

    for (item = hash_table[hash(ctx->setting_name)]; item != NULL; item = item->next) {
	if (Strings_match(ctx->setting_name, item->name)) {
	    if (!is_numeric(item->value))
		break;
	    else {
		errno = 0;
		val   = strtol(item->value, NULL, 10);

		if (errno != 0 || (val < ctx->lo_limit || val > ctx->hi_limit)) break;
		else return (val);
	    }
	}
    }

    return (ctx->fallback_default);
}

void
config_create(const char *path, const char *mode)
{
    FILE			*fp    = fopen_handle_error(path, mode);
    struct tagConfDefValues	*cdv_p = NULL;
    const size_t		 ar_sz = ARRAY_SIZE(ConfDefValues);

    write_to_stream(fp, "# -*- mode: conf; -*-\n#\n# Swirc %s  --  default config\n", g_swircVersion);
    write_to_stream(fp, "# Automatically generated at %s\n\n", current_time("%c"));

    for (cdv_p = &ConfDefValues[0]; cdv_p < &ConfDefValues[ar_sz]; cdv_p++)
	WRITE_ITEM(cdv_p->setting_name, cdv_p->value);

    fclose_ensure_success(fp);
}

void
config_do_save(const char *path, const char *mode)
{
    FILE			*fp    = fopen_handle_error(path, mode);
    struct tagConfDefValues	*cdv_p = NULL;
    const size_t		 ar_sz = ARRAY_SIZE(ConfDefValues);

    write_to_stream(fp, "# -*- mode: conf; -*-\n#\n# Swirc %s  --  default config\n", g_swircVersion);
    write_to_stream(fp, "# Automatically generated at %s\n\n", current_time("%c"));

    for (cdv_p = &ConfDefValues[0]; cdv_p < &ConfDefValues[ar_sz]; cdv_p++)
	WRITE_ITEM(cdv_p->setting_name, Config(cdv_p->setting_name));

    fclose_ensure_success(fp);
}

void
config_readit(const char *path, const char *mode)
{
    FILE     *fp        = fopen_handle_error(path, mode);
    char      buf[3200] = "";
    long int  line_num  = 0;

    while (BZERO(buf, sizeof buf), fgets(buf, sizeof buf, fp) != NULL) {
	const char		*ccp = &buf[0];
	char			*line;
	struct Interpreter_in	 in;

	adv_while_isspace(&ccp);
	if (Strings_match(ccp, "") || *ccp == '#') {
	    line_num++;
	    continue;
	}

	line = trim(sw_strdup(ccp));
	in.path           = (char *) path;
	in.line           = line;
	in.line_num       = ++line_num;
	in.validator_func = is_recognized_setting;
	in.install_func   = config_item_install;
	Interpreter(&in);
	free(line), line = NULL;
    }

    if (feof(fp)) {
	fclose_ensure_success(fp);
	init_missing_to_defs();
    } else if (ferror(fp)) {
	err_quit("fgets returned NULL and the error indicator is set");
    } else {
	err_msg("fgets returned NULL for an unknown reason (bug!)");
	abort();
    }
}

static bool
is_recognized_setting(const char *setting_name)
{
    struct tagConfDefValues *cdv_p;
    const size_t ar_sz = ARRAY_SIZE(ConfDefValues);

    if (!setting_name || *setting_name == '\0') {
	return (false);
    }

    for (cdv_p = &ConfDefValues[0]; cdv_p < &ConfDefValues[ar_sz]; cdv_p++) {
	if (Strings_match(setting_name, cdv_p->setting_name))
	    return (true);
    }

    return (false);
}

static void
init_missing_to_defs(void)
{
    struct tagConfDefValues *cdv_p;
    const size_t ar_sz = ARRAY_SIZE(ConfDefValues);

    for (cdv_p = &ConfDefValues[0]; cdv_p < &ConfDefValues[ar_sz]; cdv_p++) {
	if (get_hash_table_entry(cdv_p->setting_name) == NULL)
	    hInstall(cdv_p->setting_name, cdv_p->value);
    }
}
