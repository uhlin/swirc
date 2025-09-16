/* IRC logs i.e. not logs for system messages
   Copyright (C) 2020-2025 Markus Uhlin. All rights reserved.

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

#include <stdio.h>
#include <time.h>

#include "assertAPI.h"
#include "dataClassify.h"
#include "i18n.h"
#include "libUtils.h"
#include "log.h"
#include "nestHome.h"
#include "printtext.h"
#include "readline.h"
#include "statusbar.h"
#include "strHand.h"
#include "window.h"

const char	g_log_filesuffix[5] = ".txt";

#if defined(UNIX)
const int	g_open_flags[4] = {
	[OPFL_APPEND] = (O_WRONLY|O_CREAT|O_APPEND),
	[OPFL_WRITE] = (O_WRONLY|O_CREAT|O_TRUNC),
	[OPFL_APLUS] = (O_RDWR|O_CREAT|O_APPEND),
	[OPFL_WPLUS] = (O_RDWR|O_CREAT|O_TRUNC),
};
const mode_t	g_open_modes = (S_IWUSR | S_IRUSR);
#elif defined(WIN32)
const int	g_open_flags[4] = {
	[OPFL_APPEND] = (_O_WRONLY|_O_CREAT|_O_APPEND),
	[OPFL_WRITE] = (_O_WRONLY|_O_CREAT|_O_TRUNC),
	[OPFL_APLUS] = (_O_RDWR|_O_CREAT|_O_APPEND),
	[OPFL_WPLUS] = (_O_RDWR|_O_CREAT|_O_TRUNC),
};
const mode_t	g_open_modes = (_S_IREAD | _S_IWRITE);
#endif

static const char *get_modified_server_host(const char *) NONNULL;
static const char *get_logtype(const char *) NONNULL;

static int
check_label(const char *label, int *ip)
{
	static const char legal_index[] =
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    "abcdefghijklmnopqrstuvwxyz"
	    "0123456789+-_";

	for (const char *cp = &label[0]; *cp != '\0'; cp++) {
		if (strchr(legal_index, *cp) == NULL) {
			*ip = *cp;
			return ERR;
		}
	}

	return OK;
}

static const char *
get_date(void)
{
	static char	buffer[200] = { '\0' };
	struct tm	items = { 0 };
	time_t		seconds = 0;

	if (time(&seconds) == g_time_error)
		return "";
#if defined(UNIX)
	if (localtime_r(&seconds, &items) == NULL)
		return "";
#elif defined(WIN32)
	if (localtime_s(&items, &seconds) != 0)
		return "";
#endif
	return ((strftime(buffer, ARRAY_SIZE(buffer), "%Y-%m-%d", &items) > 0)
	    ? &buffer[0] : "");
}

static const char *
get_modified_server_host(const char *orig)
{
	static char	array[256] = { '\0' };

	if (snprintf(array, sizeof array, "%s", orig) < 0)
		BZERO(array, sizeof array);
	else
		squeeze(array, "-.");

	return (&array[0]);
}

static const char *
get_logtype(const char *label)
{
	if (strings_match_ignore_case(label, g_status_window_label))
		return "1";
	else if (!is_irc_channel(label))
		return "2";
	else if (*label == '#')
		return "3";
	else if (*label == '&')
		return "4";
	else if (*label == '!')
		return "5";
	return "6";
}

char *
log_get_path(const char *server_host, const char *label)
{
	char			*cp, *label_copy, *path;
	int			 c = 'X';
	size_t			 num_illegal = 0;
	static const size_t	 maxlabel = 20;

	if (server_host == NULL || label == NULL || g_log_dir == NULL)
		return NULL;

	label_copy = sw_strdup(label);

	if (xstrnlen(label_copy, maxlabel + 1) > maxlabel)
		label_copy[maxlabel] = '\0';
	while (check_label(label_copy, &c) != OK) {
		if ((cp = strchr(label_copy, c)) == NULL)
			sw_assert_not_reached();
		*cp = 'X';
		num_illegal++;
	}
	if (num_illegal && num_illegal == strlen(label_copy)) {
		PRINTTEXT_CONTEXT	ctx;

		printtext_context_init(&ctx, g_status_window,
		    TYPE_SPEC1_FAILURE, true);
		printtext(&ctx, "%s: inappropriate filename", __func__);
		free(label_copy);
		return NULL;
	}

	path = sw_strdup(g_log_dir);

#if defined(UNIX)
	realloc_strcat(&path, "/");
#elif defined(WIN32)
	realloc_strcat(&path, "\\");
#endif

	realloc_strcat(&path, get_modified_server_host(server_host));
	realloc_strcat(&path, "-");
	realloc_strcat(&path, get_logtype(label));
	realloc_strcat(&path, "-");

	switch (atoi(get_logtype(label))) {
	case 1:
		realloc_strcat(&path, "console");
		break;
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		realloc_strcat(&path, label_copy);
		break;
	default:
		sw_assert_not_reached();
	}

	free(label_copy);
	realloc_strcat(&path, g_log_filesuffix);
	return (strToLower(path));
}

void
log_msg(const char *path, const char *text)
{
	FILE	*fp;
	int	 fd;

	if (path == NULL || text == NULL)
		return;

#if defined(UNIX)
	if ((fd = open(path, g_open_flags[OPFL_APPEND], g_open_modes)) < 0)
		return;
#elif defined(WIN32)
	if ((errno = _sopen_s(&fd, path, g_open_flags[OPFL_APPEND], _SH_DENYNO,
	    g_open_modes)) != 0)
		return;
#endif
	else if ((fp = fdopen(fd, "a")) != NULL) {
		char *text_copy;

		text_copy = sw_strdup(text);
#ifdef HAVE_BCI
		(void) fprintf_s(fp, "%s %s\n", get_date(),
		    squeeze_text_deco(text_copy));
#else
		(void) fprintf(fp, "%s %s\n", get_date(),
		    squeeze_text_deco(text_copy));
#endif
		(void) fclose(fp);
		free(text_copy);
	} else {
		(void) close(fd);
	}
}

void
log_toggle_on_off(void)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC_NONE, true);

	if (g_active_window->logging) {
		g_active_window->logging = false;
		ctx.spec_type = TYPE_SPEC1_WARN;
		printtext(&ctx, _("Logging for window (refnum: %d) now off"),
		    g_active_window->refnum);
	} else {
		ctx.spec_type = TYPE_SPEC1_SUCCESS;
		printtext(&ctx, _("Logging for window (refnum: %d) now on"),
		    g_active_window->refnum);
		g_active_window->logging = true;
	}

	statusbar_update();
	readline_top_panel();
}
