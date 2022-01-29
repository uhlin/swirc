/* User configuration
   Copyright (C) 2012-2022 Markus Uhlin. All rights reserved.

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

#include <string>

#ifdef HAVE_LIBIDN
#include <stringprep.h>
#else
#pragma message("No GNU libidn")
#endif

#define ENTRY_FOREACH()\
    for (PCONF_HTBL_ENTRY *entry_p = &hash_table[0];\
	 entry_p < &hash_table[ARRAY_SIZE(hash_table)];\
	 entry_p++)
#define FOREACH_CDV()\
    for (struct tagConfDefValues *cdv_p = &ConfDefValues[0];\
	 cdv_p < &ConfDefValues[ARRAY_SIZE(ConfDefValues)];\
	 cdv_p++)

/* Objects with internal linkage
   ============================= */

static PCONF_HTBL_ENTRY hash_table[300];

static struct tagConfDefValues {
	const char		*setting_name;
	enum setting_type	 type;
	short int		 padding;
	const char		*value;
} ConfDefValues[] = {
	{ "nickname",                  TYPE_STRING,  2, "" },
	{ "nickname_aliases",          TYPE_STRING,  1, "" },
	{ "alt_nick",                  TYPE_STRING,  2, "" },
	{ "username",                  TYPE_STRING,  2, "" },
	{ "real_name",                 TYPE_STRING,  2, "" },

	{ "sasl",                      TYPE_BOOLEAN, 2, "no" },
	{ "sasl_mechanism",            TYPE_STRING,  1, "PLAIN" },
	{ "sasl_username",             TYPE_STRING,  1, "" },
	{ "sasl_password",             TYPE_STRING,  1, "" },

	{ "chanserv_host",             TYPE_STRING,  1, "services." },
	{ "nickserv_host",             TYPE_STRING,  1, "services." },

	{ "account_notify",            TYPE_BOOLEAN, 2, "no" },
	{ "away_notify",               TYPE_BOOLEAN, 2, "no" },
	{ "invite_notify",             TYPE_BOOLEAN, 2, "no" },
	{ "ircv3_server_time",         TYPE_BOOLEAN, 1, "no" },

	{ "part_message",              TYPE_STRING,  1, "" },
	{ "quit_message",              TYPE_STRING,  1, "Swirc IRC client" },

	{ "reconnect_backoff_delay",   TYPE_INTEGER, 1, STRINGIFY(RECONNECT_BACKOFF_DELAY_DEFAULT) },
	{ "reconnect_delay",           TYPE_INTEGER, 2, STRINGIFY(RECONNECT_DELAY_DEFAULT) },
	{ "reconnect_delay_max",       TYPE_INTEGER, 1, STRINGIFY(RECONNECT_DELAY_MAX_DEFAULT) },
	{ "reconnect_retries",         TYPE_INTEGER, 1, STRINGIFY(RECONNECT_RETRIES_DEFAULT) },

	{ "auto_op_yourself",          TYPE_BOOLEAN, 2, "yes" },
	{ "beeps",                     TYPE_BOOLEAN, 4, "yes" },
	{ "cipher_suite",              TYPE_STRING,  3, "compat" },
	{ "cmd_hist_size",             TYPE_INTEGER, 3, "50" },
	{ "connection_timeout",        TYPE_INTEGER, 2, "45" },
	{ "hostname_checking",         TYPE_BOOLEAN, 2, "yes" },
	{ "joins_parts_quits",         TYPE_BOOLEAN, 2, "no" },
	{ "kick_close_window",         TYPE_BOOLEAN, 2, "yes" },
	{ "max_chat_windows",          TYPE_INTEGER, 2, "60" },
	{ "server_cipher_suite",       TYPE_STRING,  2, "compat" },
	{ "show_ping_pong",            TYPE_BOOLEAN, 3, "no" },
	{ "skip_motd",                 TYPE_BOOLEAN, 3, "no" },
	{ "ssl_verify_peer",           TYPE_BOOLEAN, 3, "yes" },
	{ "startup_greeting",          TYPE_BOOLEAN, 2, "yes" },
	{ "textbuffer_size_absolute",  TYPE_INTEGER, 1, "1500" },
	{ "theme",                     TYPE_STRING,  4, "default" },
};

/* -------------------------------------------------- */

static bool
got_hits(const char *search_var)
{
    FOREACH_CDV() {
	if (!strncmp(search_var, cdv_p->setting_name, strlen(search_var)))
	    return true;
    }

    return false;
}

PTEXTBUF
get_list_of_matching_settings(const char *search_var)
{
    if (!got_hits(search_var))
	return NULL;

    PTEXTBUF matches = textBuf_new();

    FOREACH_CDV() {
	if (!strncmp(search_var, cdv_p->setting_name, strlen(search_var))) {
	    if (textBuf_size(matches) == 0) {
		if ((errno = textBuf_ins_next(matches, NULL, cdv_p->setting_name, -1)) != 0)
		    err_sys("get_list_of_matching_settings: textBuf_ins_next");
	    } else {
		errno =
		    textBuf_ins_next(matches, textBuf_tail(matches), cdv_p->setting_name, -1);
		if (errno)
		    err_sys("get_list_of_matching_settings: textBuf_ins_next");
	    }
	}
    }

    return matches;
}

/* -------------------------------------------------- */

void
config_init(void)
{
    ENTRY_FOREACH() {
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

static void hUndef(PCONF_HTBL_ENTRY) PTR_ARGS_NONNULL;

static void
hUndef(PCONF_HTBL_ENTRY entry)
{
    PCONF_HTBL_ENTRY *indirect = & (hash_table[hash(entry->name)]);

    while (*indirect != entry)
	indirect = & ((*indirect)->next);
    *indirect = entry->next;

    free(entry->name);
    free(entry->value);
    free(entry);
}

void
config_deinit(void)
{
    PCONF_HTBL_ENTRY p, tmp;

    ENTRY_FOREACH() {
	for (p = *entry_p; p != NULL; p = tmp) {
	    tmp = p->next;
	    hUndef(p);
	}
    }
}

/* -------------------------------------------------- */

bool
config_bool(const char *setting_name, bool fallback_default)
{
    PCONF_HTBL_ENTRY item;

    if (!setting_name)
	err_exit(EINVAL, "config_bool");

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

	if (setting_name == NULL)
		return "";

	for (item = hash_table[hash(setting_name)];
	    item != NULL;
	    item = item->next) {
		if (strings_match(setting_name, item->name))
			return item->value;
	}

	return "";
}

/*lint -sem(get_hash_table_entry, r_null) */
static PCONF_HTBL_ENTRY
get_hash_table_entry(const char *name)
{
	PCONF_HTBL_ENTRY entry;

	if (name == NULL)
		return NULL;

	for (entry = hash_table[hash(name)];
	    entry != NULL;
	    entry = entry->next) {
		if (strings_match(name, entry->name))
			return entry;
	}

	return NULL;
}

static void
hInstall(const char *name, const char *value)
{
	PCONF_HTBL_ENTRY item;
	const bool has_no_value = (value == NULL || *value == '\0');
	unsigned int hashval;

	item        = static_cast<PCONF_HTBL_ENTRY>(xcalloc(sizeof *item, 1));
	item->name  = sw_strdup(name);
	item->value = sw_strdup(has_no_value ? "" : value);

	hashval             = hash(name);
	item->next          = hash_table[hashval];
	hash_table[hashval] = item;
}

int
config_item_install(const char *name, const char *value)
{
	if (name == NULL || value == NULL)
		return EINVAL;
	else if (get_hash_table_entry(name))
		return EBUSY;
	else
		hInstall(name, value);
	return 0;
}

int
config_item_undef(const char *name)
{
	PCONF_HTBL_ENTRY entry;

	if ((entry = get_hash_table_entry(name)) == NULL)
		return ENOENT;

	hUndef(entry);
	return 0;
}

long int
config_integer(struct integer_context *ctx)
{
	PCONF_HTBL_ENTRY item;
	long int val;

	if (ctx == NULL)
		err_exit(EINVAL, "config_integer");

	for (item = hash_table[hash(ctx->setting_name)];
	    item != NULL;
	    item = item->next) {
		if (strings_match(ctx->setting_name, item->name)) {
			if (getval_strtol(item->value,
			    ctx->lo_limit, ctx->hi_limit, &val))
				return val;
			else
				break;
		}
	}

	err_log(ERANGE, "warning: setting %s (%ld-%ld): fallback value is %ld",
	    ctx->setting_name, ctx->lo_limit, ctx->hi_limit,
	    ctx->fallback_default);
	return ctx->fallback_default;
}

static void
write_config_header(FILE *fp)
{
	write_to_stream(fp, "# -*- mode: conf; -*-\n#\n# Swirc %s  --  "
	    "default config\n", g_swircVersion);
	write_to_stream(fp, "# Automatically generated at %s\n\n",
	    current_time("%c"));
}

void
config_create(const char *path, const char *mode)
{
	FILE	*fp;

	fp = fopen_exit_on_error(path, mode);
	write_config_header(fp);

	FOREACH_CDV() {
		write_setting(fp, cdv_p->setting_name, cdv_p->value, true,
		    cdv_p->padding);
	}

	fclose_ensure_success(fp);
}

void
config_do_save(const char *path, const char *mode)
{
	FILE	*fp;

	fp = fopen_exit_on_error(path, mode);
	write_config_header(fp);

	FOREACH_CDV() {
		write_setting(fp, cdv_p->setting_name,
		    Config(cdv_p->setting_name), true, cdv_p->padding);
	}

	fclose_ensure_success(fp);
}

static bool
is_recognized_setting(const char *setting_name)
{
	if (setting_name == NULL || strings_match(setting_name, ""))
		return false;

	FOREACH_CDV() {
		if (strings_match(setting_name, cdv_p->setting_name))
			return true;
	}

	return false;
}

static void
init_missing_to_defs(void)
{
	FOREACH_CDV() {
		if (get_hash_table_entry(cdv_p->setting_name) == NULL)
			hInstall(cdv_p->setting_name, cdv_p->value);
	}
}

void
config_readit(const char *path, const char *mode)
{
	FILE *fp;

	fp = fopen_exit_on_error(path, mode);

	Interpreter_processAllLines(fp, path, is_recognized_setting,
	    config_item_install);

	if (feof(fp)) {
		fclose_ensure_success(fp);
		init_missing_to_defs();
	} else if (ferror(fp)) {
		err_quit("config_readit: %s", g_fgets_nullret_err1);
	} else {
		err_msg("config_readit: %s", g_fgets_nullret_err2);
		abort();
	}
}

const char *
config_get_normalized_sasl_username(void)
{
#ifdef HAVE_LIBIDN
	Stringprep_profile_flags flags;
	char *str = NULL;
	int ret;
	static char buf[SASL_USERNAME_MAXLEN] = { '\0' };

	if (strings_match(Config("sasl_username"), "")) {
		return NULL;
	} else if ((str = stringprep_locale_to_utf8(Config("sasl_username"))) ==
	    NULL || sw_strcpy(buf, str, ARRAY_SIZE(buf)) != 0) {
		free(str);
		return NULL;
	}

	free(str);

	flags = static_cast<Stringprep_profile_flags>(0);
	ret = stringprep(buf, ARRAY_SIZE(buf), flags, stringprep_saslprep);

	return (ret == STRINGPREP_OK ? &buf[0] : NULL);
#else
	/*
	 * !HAVE_LIBIDN
	 */

	if (strings_match(Config("sasl_username"), ""))
		return NULL;
	return Config("sasl_username");
#endif
}

const char *
config_get_normalized_sasl_password(void)
{
#ifdef HAVE_LIBIDN
	Stringprep_profile_flags flags;
	char *str = NULL;
	int ret;
	static char buf[SASL_PASSWORD_MAXLEN] = { '\0' };

	if (strings_match(Config("sasl_password"), "")) {
		return NULL;
	} else if ((str = stringprep_locale_to_utf8(Config("sasl_password"))) ==
	    NULL || sw_strcpy(buf, str, ARRAY_SIZE(buf)) != 0) {
		free(str);
		return NULL;
	}

	free(str);

	flags = static_cast<Stringprep_profile_flags>(0);
	ret = stringprep(buf, ARRAY_SIZE(buf), flags, stringprep_saslprep);

	return (ret == STRINGPREP_OK ? &buf[0] : NULL);
#else
	/*
	 * !HAVE_LIBIDN
	 */

	if (strings_match(Config("sasl_password"), ""))
		return NULL;
	return Config("sasl_password");
#endif
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

static std::string
get_string(const char *name, const char *type)
{
	std::string str(name);

	(void) str.append(B1).append(type).append(B2);
	(void) str.append(" ").append(Theme("notice_sep")).append(" ");
	(void) str.append(Config(name));

	return str;
}

static void
output_values_for_all_settings(void)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC3, true);

	FOREACH_CDV() {
		printtext(&ctx, "%s", get_string(cdv_p->setting_name,
		    get_setting_type(cdv_p)).c_str());
	}
}

static void
output_value_for_specific_setting(const char *setting)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC3, true);

	FOREACH_CDV() {
		if (strings_match(setting, cdv_p->setting_name)) {
			printtext(&ctx, "%s", get_string(cdv_p->setting_name,
			    get_setting_type(cdv_p)).c_str());
			return;
		}
	}

	print_and_free("/set: no such setting", NULL);
}

static bool
set_value_for_setting(const char *setting, const char *value,
    const char **err_reason)
{
	FOREACH_CDV() {
		if (strings_match(setting, cdv_p->setting_name)) {
			if (cdv_p->type == TYPE_BOOLEAN &&
			    !strings_match_ignore_case(value, "on") &&
			    !strings_match_ignore_case(value, "true") &&
			    !strings_match_ignore_case(value, "yes") &&
			    !strings_match_ignore_case(value, "off") &&
			    !strings_match_ignore_case(value, "false") &&
			    !strings_match_ignore_case(value, "no")) {
				*err_reason = "booleans must be on, true, yes, "
				    "off, false or no";
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
	PRINTTEXT_CONTEXT	 ctx;
	const char		*err_reason = "no error";

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_SUCCESS, true);

	FOREACH_CDV() {
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

/*
 * usage: /set [[setting] [value]]
 */
void
cmd_set(const char *data)
{
	char *dcopy = sw_strdup(data);
	char *setting, *value;
	char *state = const_cast<char *>("");

	if (strings_match(dcopy, "")) {
		output_values_for_all_settings();
		free(dcopy);
		return;
	}

	(void) strFeed(dcopy, 1);

	if ((setting = strtok_r(dcopy, "\n", &state)) == NULL) {
		print_and_free("/set: fatal error (shouldn't happen)", dcopy);
		return;
	} else if ((value = strtok_r(NULL, "\n", &state)) == NULL) {
		output_value_for_specific_setting(setting);
		free(dcopy);
		return;
	}

	try_to_set_value_for_setting(setting, value);
	free(dcopy);
}

/* -------------------------------------------------- */

long int
get_reconnect_backoff_delay(void)
{
	struct integer_context ctx("reconnect_backoff_delay", 0, 99,
	    RECONNECT_BACKOFF_DELAY_DEFAULT);

	return config_integer(&ctx);
}

long int
get_reconnect_delay(void)
{
	struct integer_context ctx("reconnect_delay", 0, 999,
	    RECONNECT_DELAY_DEFAULT);

	return config_integer(&ctx);
}

long int
get_reconnect_delay_max(void)
{
	struct integer_context ctx("reconnect_delay_max", 0, 999,
	    RECONNECT_DELAY_MAX_DEFAULT);

	return config_integer(&ctx);
}

long int
get_reconnect_retries(void)
{
	struct integer_context ctx("reconnect_retries", 0, 999,
	    RECONNECT_RETRIES_DEFAULT);

	return config_integer(&ctx);
}
