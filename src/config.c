/* User configuration
   Copyright (C) 2012-2018 Markus Uhlin. All rights reserved.

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

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
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
#include "nestHome.h"
#include "printtext.h"
#include "strHand.h"
#include "theme.h"

#define ENTRY_FOREACH(entry_p)\
    for (entry_p = &hash_table[0];\
	 entry_p < &hash_table[ARRAY_SIZE(hash_table)];\
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
    { "account_notify",            TYPE_BOOLEAN, "no" },
    { "alt_nick",                  TYPE_STRING,  "" },
    { "auto_op_yourself",          TYPE_BOOLEAN, "yes" },
    { "chanserv_host",             TYPE_STRING,  "services." },
    { "cipher_suite",              TYPE_STRING,  "compat" },
    { "cmd_hist_size",             TYPE_INTEGER, "50" },
    { "connection_timeout",        TYPE_INTEGER, "45" },
    { "disable_beeps",             TYPE_BOOLEAN, "no" },
    { "hostname_checking",         TYPE_BOOLEAN, "yes" },
    { "ircv3_server_time",         TYPE_BOOLEAN, "no" },
    { "kick_close_window",         TYPE_BOOLEAN, "yes" },
    { "max_chat_windows",          TYPE_INTEGER, "60" },
    { "nickname",                  TYPE_STRING,  "warezkid" },
    { "nickname_aliases",          TYPE_STRING,  "" },
    { "nickserv_host",             TYPE_STRING,  "services." },
    { "part_message",              TYPE_STRING,  "" },
    { "quit_message",              TYPE_STRING,  "Swirc IRC client" },
    { "real_name",                 TYPE_STRING,  "pinball" },
    { "sasl",                      TYPE_BOOLEAN, "no" },
    { "sasl_mechanism",            TYPE_STRING,  "PLAIN" },
    { "sasl_password",             TYPE_STRING,  "" },
    { "sasl_username",             TYPE_STRING,  "" },
    { "show_ping_pong",            TYPE_BOOLEAN, "no" },
    { "skip_motd",                 TYPE_BOOLEAN, "no" },
    { "ssl_verify_peer",           TYPE_BOOLEAN, "YES" },
    { "startup_greeting",          TYPE_BOOLEAN, "yes" },
    { "textbuffer_size_absolute",  TYPE_INTEGER, "1500" },
    { "theme",                     TYPE_STRING,  "default" },
    { "username",                  TYPE_STRING,  "swift" },
};

/* -------------------------------------------------- */

void
config_init(void)
{
    PCONF_HTBL_ENTRY *entry_p;

    ENTRY_FOREACH(entry_p) {
	*entry_p = NULL;
    }
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

/* -------------------------------------------------- */

bool
config_bool_unparse(const char *setting_name, bool fallback_default)
{
    PCONF_HTBL_ENTRY item;

    if (!setting_name)
	err_exit(EINVAL, "config_bool_unparse");

    for (item = hash_table[hash(setting_name)]; item; item = item->next) {
	if (strings_match(setting_name, item->name)) {
	    if (strings_match_ignore_case(item->value, "on") ||
		strings_match_ignore_case(item->value, "true") ||
		strings_match_ignore_case(item->value, "yes")) {
		return (true);
	    } else if (strings_match_ignore_case(item->value, "off") ||
		       strings_match_ignore_case(item->value, "false") ||
		       strings_match_ignore_case(item->value, "no")) {
		return (false);
	    } else {
		break;
	    }
	}
    }

    err_log(EINVAL, "warning: setting %s (bool): falling back to the default",
	    setting_name);
    return (fallback_default);
}

char *
Config_mod(const char *setting_name)
{
    PCONF_HTBL_ENTRY item;

    if (!setting_name)
	return (NULL);

    for (item = hash_table[hash(setting_name)]; item; item = item->next) {
	if (strings_match(setting_name, item->name))
	    return (item->value);
    }

    return (NULL);
}

const char *
Config(const char *setting_name)
{
    PCONF_HTBL_ENTRY item;

    if (!setting_name)
	return ("");

    for (item = hash_table[hash(setting_name)]; item; item = item->next) {
	if (strings_match(setting_name, item->name))
	    return (item->value);
    }

    return ("");
}

/*lint -sem(get_hash_table_entry, r_null) */
static PCONF_HTBL_ENTRY
get_hash_table_entry(const char *name)
{
    PCONF_HTBL_ENTRY entry;

    if (!name)
	return (NULL);

    for (entry = hash_table[hash(name)]; entry != NULL; entry = entry->next) {
	if (strings_match(name, entry->name))
	    return (entry);
    }

    return (NULL);
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

int
config_item_undef(const char *name)
{
    PCONF_HTBL_ENTRY entry;

    if ((entry = get_hash_table_entry(name)) == NULL)
	return (ENOENT);

    hUndef(entry);
    return (0);
}

long int
config_integer_unparse(struct integer_unparse_context *ctx)
{
    PCONF_HTBL_ENTRY item;
    long int val;

    if (!ctx)
	err_exit(EINVAL, "config_integer_unparse");

    for (item = hash_table[hash(ctx->setting_name)]; item; item = item->next) {
	if (strings_match(ctx->setting_name, item->name)) {
	    if (!is_numeric(item->value))
		break;
	    else {
		errno = 0;
		val   = strtol(item->value, NULL, 10);

		if (errno != 0 || (val < ctx->lo_limit || val > ctx->hi_limit))
		    break;
		else
		    return (val);
	    }
	}
    }

    err_log(ERANGE, "warning: setting %s (%ld-%ld): fallback value is %ld",
	ctx->setting_name, ctx->lo_limit, ctx->hi_limit, ctx->fallback_default);
    return (ctx->fallback_default);
}

void
config_create(const char *path, const char *mode)
{
    FILE *fp = fopen_exit_on_error(path, mode);

    write_to_stream(fp, "# -*- mode: conf; -*-\n#\n"
	"# Swirc %s  --  default config\n", g_swircVersion);
    write_to_stream(fp, "# Automatically generated at %s\n\n",
		    current_time("%c"));

    for (struct tagConfDefValues *cdv_p = &ConfDefValues[0];
	 cdv_p < &ConfDefValues[ARRAY_SIZE(ConfDefValues)]; cdv_p++)
	WRITE_ITEM(cdv_p->setting_name, cdv_p->value);

    fclose_ensure_success(fp);
}

void
config_do_save(const char *path, const char *mode)
{
    FILE *fp = fopen_exit_on_error(path, mode);

    write_to_stream(fp, "# -*- mode: conf; -*-\n#\n"
	"# Swirc %s  --  default config\n", g_swircVersion);
    write_to_stream(fp, "# Automatically generated at %s\n\n",
		    current_time("%c"));

    for (struct tagConfDefValues *cdv_p = &ConfDefValues[0];
	 cdv_p < &ConfDefValues[ARRAY_SIZE(ConfDefValues)]; cdv_p++)
	WRITE_ITEM(cdv_p->setting_name, Config(cdv_p->setting_name));

    fclose_ensure_success(fp);
}

static bool
is_recognized_setting(const char *setting_name)
{
    if (!setting_name || *setting_name == '\0') {
	return (false);
    }

    for (struct tagConfDefValues *cdv_p = &ConfDefValues[0];
	 cdv_p < &ConfDefValues[ARRAY_SIZE(ConfDefValues)]; cdv_p++) {
	if (strings_match(setting_name, cdv_p->setting_name))
	    return (true);
    }

    return (false);
}

static void
init_missing_to_defs(void)
{
    for (struct tagConfDefValues *cdv_p = &ConfDefValues[0];
	 cdv_p < &ConfDefValues[ARRAY_SIZE(ConfDefValues)]; cdv_p++) {
	if (get_hash_table_entry(cdv_p->setting_name) == NULL)
	    hInstall(cdv_p->setting_name, cdv_p->value);
    }
}

void
config_readit(const char *path, const char *mode)
{
    FILE     *fp        = fopen_exit_on_error(path, mode);
    char      buf[3200] = "";
    long int  line_num  = 0;

    while (BZERO(buf, sizeof buf), fgets(buf, sizeof buf, fp) != NULL) {
	const char		*ccp = &buf[0];
	char			*line;
	struct Interpreter_in	 in;

	adv_while_isspace(&ccp);
	if (strings_match(ccp, "") || *ccp == '#') {
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

/* -------------------------------------------------- */

static const char *
get_setting_type(const struct tagConfDefValues *cdv)
{
    if (cdv->type == TYPE_BOOLEAN)
	return "bool";
    else if (cdv->type == TYPE_INTEGER)
	return "int";
    else if (cdv->type == TYPE_STRING)
	return "string";
    return "unknown";
}

#define B1 Theme("notice_inner_b1")
#define B2 Theme("notice_inner_b2")

static void
output_values_for_all_settings()
{
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC3, true);

    for (struct tagConfDefValues *cdv_p = &ConfDefValues[0];
	 cdv_p < &ConfDefValues[ARRAY_SIZE(ConfDefValues)]; cdv_p++) {
	printtext(&ctx, "%s%s%s%s %s %s",
	    cdv_p->setting_name,
	    B1, get_setting_type(cdv_p), B2,
	    Theme("notice_sep"),
	    Config(cdv_p->setting_name));
    }
}

static void
output_value_for_specific_setting(const char *setting)
{
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC3, true);

    for (struct tagConfDefValues *cdv_p = &ConfDefValues[0];
	 cdv_p < &ConfDefValues[ARRAY_SIZE(ConfDefValues)]; cdv_p++) {
	if (strings_match(setting, cdv_p->setting_name)) {
	    printtext(&ctx, "%s%s%s%s %s %s",
		cdv_p->setting_name,
		B1, get_setting_type(cdv_p), B2,
		Theme("notice_sep"),
		Config(cdv_p->setting_name));
	    return;
	}
    }

    print_and_free("/set: no such setting", NULL);
}

static bool
set_value_for_setting(const char *setting, const char *value, char **err_reason)
{
    for (struct tagConfDefValues *cdv_p = &ConfDefValues[0];
	 cdv_p < &ConfDefValues[ARRAY_SIZE(ConfDefValues)]; cdv_p++) {
	if (strings_match(setting, cdv_p->setting_name)) {
	    if (cdv_p->type == TYPE_BOOLEAN &&
		!strings_match_ignore_case(value, "on")    &&
		!strings_match_ignore_case(value, "true")  &&
		!strings_match_ignore_case(value, "yes")   &&
		!strings_match_ignore_case(value, "off")   &&
		!strings_match_ignore_case(value, "false") &&
		!strings_match_ignore_case(value, "no")) {
		*err_reason =
		    "booleans must be on, true, yes, off, false or no";
		return false;
	    } else if (cdv_p->type == TYPE_INTEGER &&
		       !is_numeric(value)) {
		*err_reason = "integer not all numeric";
		return false;
	    } else if (config_item_undef(setting) != 0) {
		*err_reason = "config_item_undef";
		return false;
	    } else if (config_item_install(setting, value) != 0) {
		*err_reason = "config_item_install";
		return false;
	    }

	    config_do_save(g_config_file, "w");
	    return true;
	}
    }

    *err_reason = "no such setting";
    return false;
}

static void
try_to_set_value_for_setting(const char *setting, const char *value)
{
    PRINTTEXT_CONTEXT ctx;
    char *err_reason = "no error";

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_SUCCESS, true);

    for (struct tagConfDefValues *cdv_p = &ConfDefValues[0];
	 cdv_p < &ConfDefValues[ARRAY_SIZE(ConfDefValues)]; cdv_p++) {
	if (strings_match(setting, cdv_p->setting_name)) {
	    if (!set_value_for_setting(setting, value, &err_reason))
		print_and_free(err_reason, NULL);
	    else
		printtext(&ctx, "ok");
	    return;
	}
    }

    print_and_free("/set: no such setting", NULL);
}

/* usage: /set [[setting] [value]] */
void
cmd_set(const char *data)
{
    char *dcopy = sw_strdup(data);
    char *setting = NULL, *value = NULL;
    char *state = "";

    if (strings_match(dcopy, "")) {
	output_values_for_all_settings();
	free(dcopy);
	return;
    }
    strFeed(dcopy, 1);
    setting = strtok_r(dcopy, "\n", &state);
    value = strtok_r(NULL, "\n", &state);;
    if (setting == NULL) {
	print_and_free("/set: fatal error (shouldn't happen)", dcopy);
	return;
    } else if (value == NULL) {
	output_value_for_specific_setting(setting);
	free(dcopy);
	return;
    }
    try_to_set_value_for_setting(setting, value);
    free(dcopy);
}
