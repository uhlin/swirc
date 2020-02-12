#include "common.h"

#include <stdio.h>
#include <time.h>

#include "assertAPI.h"
#include "dataClassify.h"
#include "libUtils.h"
#include "log.h"
#include "nestHome.h"
#include "printtext.h"
#include "strHand.h"
#include "window.h"

const char g_log_filesuffix[] = ".txt";

static const char *get_modified_server_host(const char *) PTR_ARGS_NONNULL;
static const char *get_logtype(const char *) PTR_ARGS_NONNULL;

static const char *
get_date()
{
    static char		buffer[200] = "";
    struct tm		items = { 0 };
    time_t		seconds = 0;
    const time_t	unsuccessful = ((time_t) -1);

    if (time(&seconds) == unsuccessful) {
	return "";
    }

#if defined(UNIX)
    if (localtime_r(&seconds, &items) == NULL) {
	return "";
    }
#elif defined(WIN32)
    if (localtime_s(&items, &seconds) != 0) {
	return "";
    }
#endif

    return ((strftime(buffer, ARRAY_SIZE(buffer), "%Y-%m-%d", &items) > 0)
	    ? &buffer[0]
	    : "");
}

static const char *
get_modified_server_host(const char *orig)
{
    static char array[256] = { '\0' };

    memset(array, 0, ARRAY_SIZE(array));
    snprintf(array, ARRAY_SIZE(array), "%s", orig);
    squeeze(array, "-.");

    return (&array[0]);
}

static const char *
get_logtype(const char *label)
{
    if (strings_match_ignore_case(label, g_status_window_label))
	return "1";
    else if (is_irc_channel(label))
	return "2";
    return "3";
}

char *
log_get_path(const char *server_host, const char *label)
{
    if (server_host == NULL || label == NULL || g_log_dir == NULL)
	return NULL;

    char *path = sw_strdup(g_log_dir);

#if defined(UNIX)
    realloc_strcat(&path, "/");
#elif defined(WIN32)
    realloc_strcat(&path, "\\");
#endif

    realloc_strcat(&path, get_modified_server_host(server_host));
    realloc_strcat(&path, "-");
    realloc_strcat(&path, get_logtype(label));
    realloc_strcat(&path, ".");

    switch (atoi(get_logtype(label))) {
    case 1:
	realloc_strcat(&path, "console");
	break;
    case 2:
    case 3:
	realloc_strcat(&path, label);
	break;
    default:
	sw_assert_not_reached();
    }

    realloc_strcat(&path, g_log_filesuffix);
    return (strToLower(path));
}

void
log_msg(const char *path, const char *text)
{
    if (path == NULL || text == NULL)
	return;

    FILE *fp = xfopen(path, "a");

    if (fp != NULL) {
	char *text_copy = sw_strdup(text);

	(void) fprintf(fp, "%s %s\n", get_date(), squeeze_text_deco(text_copy));
	(void) fclose(fp);

	free(text_copy);
    }
}

void
log_toggle_on_off(void)
{
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC_NONE, true);

    if (g_active_window->logging) {
	g_active_window->logging = false;
	ctx.spec_type = TYPE_SPEC1_WARN;
	printtext(&ctx, "Logging for window (refnum: %d) now off",
		  g_active_window->refnum);
    } else {
	ctx.spec_type = TYPE_SPEC1_SUCCESS;
	printtext(&ctx, "Logging for window (refnum: %d) now on",
		  g_active_window->refnum);
	g_active_window->logging = true;
    }
}
