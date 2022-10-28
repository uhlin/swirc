/* commands/theme.c  --  management of themes on-the-fly
   Copyright (C) 2017-2022 Markus Uhlin. All rights reserved.

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

#include <curl/curl.h>
#if defined(_lint) && defined(curl_easy_setopt)
#undef curl_easy_setopt
#endif

#include "../assertAPI.h"
#include "../config.h"
#include "../errHand.h"
#include "../filePred.h"
#include "../interpreter.h"
#include "../libUtils.h"
#include "../main.h"
#include "../nestHome.h"
#include "../printtext.h"
#include "../statusbar.h"
#include "../strHand.h"
#include "../strdup_printf.h"
#include "../theme.h"
#include "../titlebar.h"

/* this is not '../theme.h' */
#include "theme.h"

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

static THEME_INFO theme_info_array[MAX_NO_THEMES];

static void free_theme_info(PTHEME_INFO) PTR_ARGS_NONNULL;
static void url_to_file(const char *, const char *) PTR_ARGS_NONNULL;
static void install_theme(const char *) PTR_ARGS_NONNULL;
static void set_theme(const char *) PTR_ARGS_NONNULL;

static bool
is_instruction_ok(const char *instruction)
{
    if (!instruction)
	return false;
    else if (strings_match(instruction, "install"))
	return true;
    else if (strings_match(instruction, "list-remote"))
	return true;
    else if (strings_match(instruction, "set"))
	return true;
    else
	return false;

    /*NOTREACHED*/ sw_assert_not_reached();
    /*NOTREACHED*/ return false;
}

static void
theme_info_array_init(void)
{
    PTHEME_INFO ar_p;

    THEME_INFO_FOREACH(ar_p) {
	ar_p->filename	= NULL;
	ar_p->version	= NULL;
	ar_p->author	= NULL;
	ar_p->email	= NULL;
	ar_p->timestamp = NULL;
	ar_p->comment	= NULL;
    }
}

static void
free_theme_info(PTHEME_INFO info)
{
    free_and_null(&info->filename);
    free_and_null(&info->version);
    free_and_null(&info->author);
    free_and_null(&info->email);
    free_and_null(&info->timestamp);
    free_and_null(&info->comment);
}

static void
theme_info_array_deinit(void)
{
    PTHEME_INFO ar_p;

    THEME_INFO_FOREACH(ar_p) {
	free_theme_info(ar_p);
    }
}

static size_t
write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    return fwrite(ptr, size, nmemb, (FILE *) stream);
}

static void
url_to_file(const char *url, const char *path)
{
    CURL	*curl_handle = NULL;
    CURLcode	 ret = CURLE_OK;
    FILE	*pagefile = NULL;
    const char	*failed_op = "";

    errno = 0;

    if ((ret = curl_global_init(CURL_GLOBAL_ALL)) != CURLE_OK) {
	failed_op = "curl_global_init";
	goto err;
    } else if ((curl_handle = curl_easy_init()) == NULL) {
	failed_op = "curl_easy_init";
	goto err;
    } else if ((ret = curl_easy_setopt(curl_handle, CURLOPT_URL, url)) !=
	       CURLE_OK) {
	failed_op = "curl_easy_setopt: CURLOPT_URL";
	goto err;
    } else if ((ret = curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L)) !=
	       CURLE_OK) {
	failed_op = "curl_easy_setopt: CURLOPT_VERBOSE";
	goto err;
    } else if ((ret = curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L)) !=
	       CURLE_OK) {
	failed_op = "curl_easy_setopt: CURLOPT_NOPROGRESS";
	goto err;
    } else if ((ret = curl_easy_setopt(curl_handle,
				       CURLOPT_WRITEFUNCTION,
				       write_data)) != CURLE_OK) {
	failed_op = "curl_easy_setopt: CURLOPT_WRITEFUNCTION";
	goto err;
    } else if ((pagefile = xfopen(path, "w")) == NULL) {
	failed_op = "xfopen";
	goto err;
    } else if ((ret = curl_easy_setopt(curl_handle,
				       CURLOPT_WRITEDATA,
				       pagefile)) != CURLE_OK) {
	failed_op = "curl_easy_setopt: CURLOPT_WRITEDATA";
	goto err;
    } else if ((ret = curl_easy_perform(curl_handle)) != CURLE_OK) {
	failed_op = "curl_easy_perform";
	goto err;
    }

    curl_easy_cleanup(curl_handle);
    fclose(pagefile);
    return;

  err:
    if (curl_handle)
	curl_easy_cleanup(curl_handle);
    if (pagefile)
	fclose(pagefile);
    err_log(errno, "url_to_file: %s: %s", failed_op, curl_easy_strerror(ret));
}

static bool
get_next_line_from_file(FILE *fp, char **line)
{
    const int LINE_MAX_LEN = 2048;

    if (fp == NULL || line == NULL)
	err_exit(EINVAL, "get_next_line_from_file");

    if (*line) {
	free(*line);
	*line = NULL;
    }

    if ((*line = malloc(LINE_MAX_LEN)) == NULL)
	err_exit(ENOMEM, "get_next_line_from_file");

    return (fgets(*line, LINE_MAX_LEN, fp) ? true : false);
}

static bool
add_to_array(PTHEME_INFO info)
{
    PTHEME_INFO ar_p;

    if (!info)
	return false;

    THEME_INFO_FOREACH(ar_p) {
	if (ar_p->filename == NULL) {
	    ar_p->filename  = sw_strdup(info->filename);
	    ar_p->version   = sw_strdup(info->version);
	    ar_p->author    = sw_strdup(info->author);
	    ar_p->email	    = sw_strdup(info->email);
	    ar_p->timestamp = sw_strdup(info->timestamp);
	    ar_p->comment   = sw_strdup(info->comment);
	    free_theme_info(info);
	    return true;
	}
    }

    sw_assert_not_reached();
    return false;
}

/*lint -sem(tokenize, r_null) */
static PTHEME_INFO
tokenize(const char *string)
{
    char *s_copy = sw_strdup(string);
    char *state = "";
    const char delim[] = ":";
    static THEME_INFO info = { NULL };

    char	*filename  = strtok_r(s_copy, delim, &state);
    char	*version   = strtok_r(NULL, delim, &state);
    char	*author	   = strtok_r(NULL, delim, &state);
    char	*email	   = strtok_r(NULL, delim, &state);
    char	*timestamp = strtok_r(NULL, delim, &state);
    char	*comment   = strtok_r(NULL, delim, &state);

    if (filename == NULL || version == NULL || author == NULL ||
	email == NULL || timestamp == NULL || comment == NULL) {
	free(s_copy);
	return NULL;
    }

    info.filename  = sw_strdup(filename);
    info.version   = sw_strdup(version);
    info.author	   = sw_strdup(author);
    info.email	   = sw_strdup(email);
    info.timestamp = sw_strdup(timestamp);
    info.comment   = sw_strdup(comment);

    free(s_copy);
    return (&info);
}

static read_result_t
read_db(const char *path, int *themes_read)
{
    FILE *fp = NULL;
    char *line = NULL;
    read_result_t res = READ_INCOMPLETE;

    if (!path || (fp = xfopen(path, "r")) == NULL)
	return FOPEN_FAILED;

    while (get_next_line_from_file(fp, &line)) {
	const char *cp = trim(&line[0]);

	adv_while_isspace(&cp);

	if (strings_match(cp, "") || *cp == '#')
	    continue;

	if (!add_to_array(tokenize(cp))) {
	    fclose(fp);
	    free(line);
	    return PARSE_ERROR;
	}

	if (themes_read)
	    (*themes_read)++;
    }

    res = (feof(fp) ? READ_DB_OK : READ_INCOMPLETE);

    fclose(fp);
    free(line);

    return res;
}

static void
clean_up(char *url, char *path)
{
    if (url)
	free(url);

    if (path) {
	if (remove(path) != 0)
	    err_log(errno, "failed to remove: %s", path);
	free(path);
    }

    theme_info_array_deinit();
}

static bool
theme_is_in_db(const char *name)
{
    PRINTTEXT_CONTEXT ctx;
    PTHEME_INFO ar_p = NULL;

    THEME_INFO_FOREACH(ar_p) {
	if (ar_p->filename && name && strings_match(ar_p->filename, name))
	    return true;
    }

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_FAILURE, true);
    printtext(&ctx, "theme not in database");
    return false;
}

static void
install_theme(const char *name)
{
	PRINTTEXT_CONTEXT	 ctx;
	char			*url, *dest;

	url = strdup_printf("%s%s%s%s", g_swircWebAddr, "themes/", name,
	    g_theme_filesuffix);
	dest = strdup_printf("%s" SLASH "%s%s", g_home_dir, name,
	    g_theme_filesuffix);

	url_to_file(url, dest);
	printtext_context_init(&ctx, g_active_window, (file_exists(dest) ?
	    TYPE_SPEC1_SUCCESS : TYPE_SPEC1_FAILURE), true);

	if (ctx.spec_type == TYPE_SPEC1_SUCCESS)
		printtext(&ctx, "theme installed (use 'set' to activate it)");
	else
		printtext(&ctx, "failed to install theme :-(");

	free(url);
	free(dest);
}

static void
list_remote(void)
{
#define B1 Theme("notice_inner_b1")
#define B2 Theme("notice_inner_b2")
	PRINTTEXT_CONTEXT	ctx;
	PTHEME_INFO		ar_p = NULL;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC2, true);

	THEME_INFO_FOREACH(ar_p) {
		if (ar_p->filename) {
			printtext(&ctx, "----- %s%s%c %cv%s%c %s%s%s%c%s -----",
			    COLOR1, ar_p->filename, NORMAL,
			    UNDERLINE, ar_p->version, NORMAL,
			    B1, COLOR4, ar_p->comment, NORMAL, B2);
			printtext(&ctx, "%sAuthor%c: %s", COLOR2, NORMAL,
			    ar_p->author);
			printtext(&ctx, "%sE-mail%c: %s", COLOR2, NORMAL,
			    ar_p->email);
			printtext(&ctx, "%sAdded%c:  %s", COLOR2, NORMAL,
			    ar_p->timestamp);
		}
	}
}

static void
set_theme(const char *name)
{
	PRINTTEXT_CONTEXT ctx;
	char buf[PATH_MAX] = { '\0' };

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_FAILURE, true);

	if ((errno = sw_strcpy(buf, g_home_dir, sizeof buf)) != 0 ||
	    (errno = sw_strcat(buf, SLASH, sizeof buf)) != 0 ||
	    (errno = sw_strcat(buf, name, sizeof buf)) != 0 ||
	    (errno = sw_strcat(buf, g_theme_filesuffix, sizeof buf)) != 0) {
		printtext(&ctx, "error building name path (errno=%d)", errno);
		return;
	} else if (!is_regular_file(buf)) {
		printtext(&ctx, "non-existent");
		return;
	}

	theme_deinit();
	theme_init();
	theme_readit(buf, "r");

	titlebar(" %s ", (g_active_window->title ? g_active_window->title :
	    ""));
	statusbar_update_display_beta();

	ctx.spec_type = TYPE_SPEC1_SUCCESS;
	printtext(&ctx, "theme activated");

	if ((errno = config_item_undef("theme")) != 0)
		err_log(errno, "%s: config_item_undef", __func__);
	if ((errno = config_item_install("theme", name)) != 0)
		err_log(errno, "%s: config_item_install", __func__);

	BZERO(buf, sizeof buf);

	if ((errno = sw_strcpy(buf, g_home_dir, sizeof buf)) != 0 ||
	    (errno = sw_strcat(buf, SLASH, sizeof buf)) != 0 ||
	    (errno = sw_strcat(buf, "swirc", sizeof buf)) != 0 ||
	    (errno = sw_strcat(buf, g_config_filesuffix, sizeof buf)) != 0) {
		ctx.spec_type = TYPE_SPEC1_FAILURE;
		printtext(&ctx, "error building path to swirc%s (errno=%d)",
		    g_config_filesuffix, errno);
		printtext(&ctx, "error saving swirc%s", g_config_filesuffix);
		return;
	}

	config_do_save(buf, "w");
}

/*
 * usage: /theme [install | list-remote | set] [name]
 */
void
cmd_theme(const char *data)
{
	char			*db_path, *instruction, *name, *url;
	char			*dcopy = sw_strdup(data);
	char			*state = "";
	int			 themes_read = 0;
	static const char	 cmd[] = "/theme";

	if (strings_match(dcopy, "") || (instruction = strtok_r(dcopy, " ",
	    &state)) == NULL) {
		printf_and_free(dcopy, "%s: missing arguments", cmd);
		return;
	}

	const bool has_second_arg = (name = strtok_r(NULL, " ", &state)) !=
	    NULL;

	if (strtok_r(NULL, " ", &state) != NULL) {
		printf_and_free(dcopy, "%s: implicit trailing data", cmd);
		return;
	} else if (!is_instruction_ok(instruction)) {
		printf_and_free(dcopy, "%s: bogus instruction!", cmd);
		return;
	} else if (!has_second_arg && (strings_match(instruction, "install") ||
	    strings_match(instruction, "set"))) {
		printf_and_free(dcopy, "%s: missing arguments", cmd);
		return;
	}

	url = strdup_printf("%s%s", g_swircWebAddr, "themes/themes");
	db_path = strdup_printf("%s" SLASH "%s", g_tmp_dir, "themes");

	theme_info_array_init();
	url_to_file(url, db_path);

	switch (read_db(db_path, &themes_read)) {
	case FOPEN_FAILED:
		printf_and_free(dcopy, "%s: cannot open database", cmd);
		clean_up(url, db_path);
		return;
	case PARSE_ERROR:
		printf_and_free(dcopy, "%s: failed to read database", cmd);
		clean_up(url, db_path);
		return;
	case READ_INCOMPLETE:
		printf_and_free(dcopy, "%s: end-of-file indicator not set",
		    cmd);
		clean_up(url, db_path);
		return;
	case READ_DB_OK:
	default:
		break;
	}

	if (strings_match(instruction, "install")) {
		if (theme_is_in_db(name))
			install_theme(name);
	} else if (strings_match(instruction, "list-remote")) {
		list_remote();
	} else if (strings_match(instruction, "set")) {
		if (strings_match(name, "default") || theme_is_in_db(name))
			set_theme(name);
	} else {
		sw_assert_not_reached();
	}

	free(dcopy);
	clean_up(url, db_path);
}
