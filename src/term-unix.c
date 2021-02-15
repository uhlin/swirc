/* Copyright (C) 2012-2021 Markus Uhlin. All rights reserved. */

#include "common.h"
#include "errHand.h"
#include "strHand.h"
#include "term-unix.h"

void
term_set_title(const char *fmt, ...)
{
    char term_brand[80] = { '\0' };
    char *var_data = NULL;
    const char *known_brands[] = {
	"xterm",
	"xterm-256color",
	"rxvt-unicode",
	"rxvt-unicode-256color",
    };
    const size_t ar_sz = ARRAY_SIZE(known_brands);

    if ((var_data = getenv("TERM")) == NULL ||
	sw_strcpy(term_brand, var_data, sizeof term_brand) != 0)
	return;

    for (const char **ppcc = &known_brands[0]; ppcc < &known_brands[ar_sz];
	 ppcc++) {
	if (strings_match(*ppcc, term_brand)) {
	    char os_cmd[1100] = { '\0' };
	    va_list ap;

	    (void) sw_strcpy(os_cmd, "\033]2;", sizeof os_cmd);
	    va_start(ap, fmt);
	    (void) vsnprintf(&os_cmd[strlen(os_cmd)],
		sizeof os_cmd - strlen(os_cmd), fmt, ap);
	    va_end(ap);
	    if (sw_strcat(os_cmd, "\a", sizeof os_cmd) != 0)
		break;
	    (void) fputs(os_cmd, stdout);
	    (void) fflush(stdout);
	    break;
	}
    }
}

void
term_restore_title(void)
{
    term_set_title("Terminal");
}

struct winsize
term_get_size(void)
{
    struct winsize size = { 0 };

    if (ioctl(fileno(stdin), TIOCGWINSZ, &size) == -1)
	err_sys("term_get_size: ioctl: TIOCGWINSZ");

    return size;
}
