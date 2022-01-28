/* Support for themes
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

#if defined(CURSES_HDR)
#include CURSES_HDR
#elif UNIX
#include <curses.h>
#elif WIN32
#include "pdcurses/curses.h"
#else
#error "Cannot determine curses header file!"
#endif

#include "errHand.h"
#include "interpreter.h"
#include "libUtils.h"
#include "main.h"
#include "strHand.h"
#include "theme.h"

#define ENTRY_FOREACH()\
    for (PTHEME_HTBL_ENTRY *entry_p = &hash_table[0];\
	 entry_p < &hash_table[ARRAY_SIZE(hash_table)];\
	 entry_p++)
#define FOREACH_TDV()\
    for (struct tagThemeDefValues *tdv_p = &ThemeDefValues[0];\
	 tdv_p < &ThemeDefValues[ARRAY_SIZE(ThemeDefValues)];\
	 tdv_p++)

/* Objects with internal linkage
   ============================= */

static PTHEME_HTBL_ENTRY hash_table[300];

static struct tagThemeDefValues {
    char		*item_name;
    enum setting_type	 type;
    short int		 padding;
    char		*value;
} ThemeDefValues[] = {
    { "term_background",           TYPE_INTEGER, 9, "1" },
    { "term_enable_colors",        TYPE_BOOLEAN, 6, "yes" },
#if defined(UNIX)
    { "term_use_default_colors",   TYPE_BOOLEAN, 1, "yes" },
#elif defined(WIN32)
    { "term_use_default_colors",   TYPE_BOOLEAN, 1, "no" },
#endif

    { "color3",                    TYPE_STRING,  1, "\00303" },
    { "color4",                    TYPE_STRING,  1, "\00305" },

    { "gfx_failure",               TYPE_STRING,  1, "[\0034*\017]" },
    { "gfx_success",               TYPE_STRING,  1, "[\0039*\017]" },
    { "gfx_warning",               TYPE_STRING,  1, "[\0038*\017]" },

    { "left_bracket",              TYPE_STRING,  2, "\00314[\017" },
    { "right_bracket",             TYPE_STRING,  1, "\00314]\017" },

    { "logo_color",                TYPE_STRING,  1, "\00302" },

    { "nick_s1",                   TYPE_STRING,  1, "\00314:\017" },
    { "nick_s2",                   TYPE_STRING,  1, "\00314:\017" },

    { "nicklist_nick_color",       TYPE_INTEGER, 6, "15" },
    { "nicklist_privilege_color",  TYPE_INTEGER, 1, "3" },
    { "nicklist_vline_color",      TYPE_INTEGER, 5, "2" },

    { "notice_color1",             TYPE_STRING,  3, "\00313" },
    { "notice_color2",             TYPE_STRING,  3, "\00306" },
    { "notice_inner_b1",           TYPE_STRING,  1, "\00314(\017" },
    { "notice_inner_b2",           TYPE_STRING,  1, "\00314)\017" },
    { "notice_lb",                 TYPE_STRING,  7, "\00314-\017" },
    { "notice_rb",                 TYPE_STRING,  7, "\00314-\017" },
    { "notice_sep",                TYPE_STRING,  6, "\00314:\017" },

    { "primary_color",             TYPE_STRING,  3, "\00312" },
    { "secondary_color",           TYPE_STRING,  1, "\00300" },

    { "slogan",                    TYPE_STRING,  1,
      "\0033,1The universal IRC client\017" },

    { "specifier1",                TYPE_STRING,  1, "\00314[\0030-\00314]\017" },
    { "specifier2",                TYPE_STRING,  1, "\00314[\0030:\00314]\017" },
    { "specifier3",                TYPE_STRING,  1, "\00314[\0030x\00314]\017" },

    { "statusbar_bg",              TYPE_STRING,  11, "black" },
    { "statusbar_fg",              TYPE_STRING,  11, "white" },
    { "statusbar_leftBracket",     TYPE_STRING,  2, "\00312,1[\017" },
    { "statusbar_rightBracket",    TYPE_STRING,  1, "\00312,1]\017" },
    { "statusbar_spec",            TYPE_STRING,  9, "[-]" },

    { "time_format",               TYPE_STRING,  1,
      "\00314[\017%H:%M\00314]\017" },

    { "titlebar_bg",               TYPE_STRING,  1, "white" },
    { "titlebar_fg",               TYPE_STRING,  1, "black" },

    { "whois_acc",                 TYPE_STRING,  6, "\00314=\017> account  :" },
    { "whois_away",                TYPE_STRING,  5, "\00314=\017> away     :" },
    { "whois_cert",                TYPE_STRING,  5, "\00314=\017> cert     :" },
    { "whois_channels",            TYPE_STRING,  1, "\00314=\017> channels :" },
    { "whois_conn",                TYPE_STRING,  5, "\00314=\017> conn     :" },
    { "whois_host",                TYPE_STRING,  5, "\00314=\017> host     :" },
    { "whois_idle",                TYPE_STRING,  5, "\00314=\017> idle     :" },
    { "whois_ircName",             TYPE_STRING,  2, "\00314=\017> ircname  :" },
    { "whois_ircOp",               TYPE_STRING,  4, "\00314=\017> IRC op   :" },
    { "whois_modes",               TYPE_STRING,  4, "\00314=\017> modes    :" },
    { "whois_server",              TYPE_STRING,  3, "\00314=\017> server   :" },
    { "whois_service",             TYPE_STRING,  2, "\00314=\017> service  :" },
    { "whois_ssl",                 TYPE_STRING,  6, "\00314=\017> TLS/SSL  :" },
};

/* -------------------------------------------------- */

void
theme_init(void)
{
    ENTRY_FOREACH() {
	*entry_p = NULL;
    }
}

/* hash pjw */
static unsigned int
hash(const char *item_name)
{
    char c;
    unsigned int hashval = 0;
    unsigned int tmp;

    while ((c = *item_name++) != 0) {
	hashval = (hashval << 4) + c;
	tmp = hashval & 0xf0000000;

	if (tmp) {
	    hashval ^= (tmp >> 24);
	    hashval ^= tmp;
	}
    }

    return (hashval % ARRAY_SIZE(hash_table));
}

static void hUndef(PTHEME_HTBL_ENTRY) PTR_ARGS_NONNULL;

static void
hUndef(PTHEME_HTBL_ENTRY entry)
{
    PTHEME_HTBL_ENTRY *indirect = & (hash_table[hash(entry->name)]);

    while (*indirect != entry)
	indirect = & ((*indirect)->next);
    *indirect = entry->next;

    free(entry->name);
    free(entry->value);
    free(entry);
}

void
theme_deinit(void)
{
    PTHEME_HTBL_ENTRY p, tmp;

    ENTRY_FOREACH() {
	for (p = *entry_p; p != NULL; p = tmp) {
	    tmp = p->next;
	    hUndef(p);
	}
    }
}

/* -------------------------------------------------- */

bool
theme_bool(const char *item_name, bool fallback_default)
{
    PTHEME_HTBL_ENTRY item;

    if (!item_name)
	err_exit(EINVAL, "theme_bool");

    for (item = hash_table[hash(item_name)]; item != NULL; item = item->next) {
	if (strings_match(item_name, item->name)) {
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

    err_log(EINVAL, "warning: item %s (bool): falling back to the default",
	    item_name);
    return (fallback_default);
}

char *
Theme_mod(const char *item_name)
{
    PTHEME_HTBL_ENTRY item;

    if (!item_name)
	return (NULL);

    for (item = hash_table[hash(item_name)]; item != NULL; item = item->next) {
	if (strings_match(item_name, item->name))
	    return (item->value);
    }

    return (NULL);
}

const char *
Theme(const char *item_name)
{
    PTHEME_HTBL_ENTRY item;

    if (!item_name)
	return ("");

    for (item = hash_table[hash(item_name)]; item != NULL; item = item->next) {
	if (strings_match(item_name, item->name))
	    return (item->value);
    }

    return ("");
}

/*lint -sem(get_hash_table_entry, r_null) */
static PTHEME_HTBL_ENTRY
get_hash_table_entry(const char *name)
{
    PTHEME_HTBL_ENTRY entry;

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
    PTHEME_HTBL_ENTRY item;
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
theme_item_install(const char *name, const char *value)
{
    if (!name || !value) {
	return (EINVAL);
    } else if (get_hash_table_entry(name)) {
	return (EBUSY);
    }
#if WIN32
    else if (strings_match(name, "term_use_default_colors")) {
	hInstall(name, "NO");
    }
#endif
    else {
	hInstall(name, value);
    }

    return (0);
}

int
theme_item_undef(const char *name)
{
    PTHEME_HTBL_ENTRY entry;

    if ((entry = get_hash_table_entry(name)) == NULL)
	return (ENOENT);

    hUndef(entry);
    return (0);
}

long int
theme_integer(struct integer_context *ctx)
{
    PTHEME_HTBL_ENTRY item;
    long int val;

    if (!ctx)
	err_exit(EINVAL, "theme_integer");

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

    err_log(ERANGE, "warning: item %s (%ld-%ld): fallback value is %ld",
	ctx->setting_name, ctx->lo_limit, ctx->hi_limit, ctx->fallback_default);
    return (ctx->fallback_default);
}

short int
theme_color(const char *item_name, short int fallback_color)
{
    PTHEME_HTBL_ENTRY item;

    if (!item_name)
	err_exit(EINVAL, "theme_color");

    for (item = hash_table[hash(item_name)]; item != NULL; item = item->next) {
	if (strings_match(item_name, item->name)) {
	    if (strings_match_ignore_case(item->value, "black"))
		return COLOR_BLACK;
	    else if (strings_match_ignore_case(item->value, "red"))
		return COLOR_RED;
	    else if (strings_match_ignore_case(item->value, "green"))
		return COLOR_GREEN;
	    else if (strings_match_ignore_case(item->value, "yellow"))
		return COLOR_YELLOW;
	    else if (strings_match_ignore_case(item->value, "blue"))
		return COLOR_BLUE;
	    else if (strings_match_ignore_case(item->value, "magenta"))
		return COLOR_MAGENTA;
	    else if (strings_match_ignore_case(item->value, "cyan"))
		return COLOR_CYAN;
	    else if (strings_match_ignore_case(item->value, "white"))
		return COLOR_WHITE;
	    else break;
	}
    }

    err_log(EINVAL, "warning: item %s (color): falling back to the default",
	    item_name);
    return (fallback_color);
}

void
theme_create(const char *path, const char *mode)
{
    FILE *fp = fopen_exit_on_error(path, mode);

    write_to_stream(fp, "# -*- mode: conf; -*-\n#\n"
	"# Swirc %s  --  default theme\n", g_swircVersion);
    write_to_stream(fp, "# Automatically generated at %s\n\n",
		    current_time("%c"));

    FOREACH_TDV() {
	write_setting(fp, tdv_p->item_name, tdv_p->value, false,
	    tdv_p->padding);
    }

    fclose_ensure_success(fp);
}

void
theme_do_save(const char *path, const char *mode)
{
	FILE *fp;

	fp = fopen_exit_on_error(path, mode);

	write_to_stream(fp, "# -*- mode: conf; -*-\n#\n# Swirc %s  --  "
	    "default theme\n", g_swircVersion);
	write_to_stream(fp, "# Automatically generated at %s\n\n",
	    current_time("%c"));

	FOREACH_TDV() {
		write_setting(fp, tdv_p->item_name, Theme(tdv_p->item_name),
		    false, tdv_p->padding);
	}

	fclose_ensure_success(fp);
}

static bool
is_recognized_item(const char *item_name)
{
	if (item_name == NULL || strings_match(item_name, ""))
		return false;

	FOREACH_TDV() {
		if (strings_match(item_name, tdv_p->item_name))
			return true;
	}

	return false;
}

static void
init_missing_to_defs(void)
{
	FOREACH_TDV() {
		if (get_hash_table_entry(tdv_p->item_name) == NULL)
			hInstall(tdv_p->item_name, tdv_p->value);
	}
}

void
theme_readit(const char *path, const char *mode)
{
	FILE *fp;

	fp = fopen_exit_on_error(path, mode);

	Interpreter_processAllLines(fp, path, is_recognized_item,
	    theme_item_install);

	if (feof(fp)) {
		fclose_ensure_success(fp);
		init_missing_to_defs();
	} else if (ferror(fp)) {
		err_quit("theme_readit: %s", g_fgets_nullret_err1);
	} else {
		err_msg("theme_readit: %s", g_fgets_nullret_err2);
		abort();
	}
}
