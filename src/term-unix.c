/* Copyright (C) 2012-2022 Markus Uhlin. All rights reserved. */

#include "common.h"

#include <termios.h>
#include <unistd.h>

#include "assertAPI.h"
#include "errHand.h"
#include "main.h"
#include "strHand.h"
#include "term-unix.h"

static stringarray_t known_brands = {
	"xterm",
	"xterm-256color",
	"rxvt-unicode",
	"rxvt-unicode-256color",
};

struct winsize
term_get_size(void)
{
	struct winsize size = { 0 };

	if (ioctl(fileno(stdin), TIOCGWINSZ, &size) == -1)
		err_sys("term_get_size: ioctl: TIOCGWINSZ");
	return size;
}

void
term_restore_title(void)
{
	term_set_title("Terminal");
}

void
term_set_title(const char *fmt, ...)
{
	char	*var_data;
	char	 term_brand[100] = { '\0' };

	if ((var_data = getenv("TERM")) == NULL || sw_strcpy(term_brand,
	    var_data, sizeof term_brand) != 0)
		return;

	for (const char **ppcc = &known_brands[0];
	    ppcc < &known_brands[ARRAY_SIZE(known_brands)];
	    ppcc++) {

		if (strings_match(*ppcc, term_brand)) {
			char cmd[600] = { '\0' };
			va_list ap;

			(void) sw_strcpy(cmd, "\033]2;", sizeof cmd);
			va_start(ap, fmt);
			(void) vsnprintf(&cmd[strlen(cmd)],
			    (sizeof cmd - strlen(cmd)), fmt, ap);
			va_end(ap);
			if (sw_strcat(cmd, "\a", sizeof cmd) != 0)
				return;
			(void) fputs(cmd, stdout);
			(void) fflush(stdout);

			return;
		}

	} /* for */
}

void
term_toggle_echo(on_off_t state)
{
	struct termios attrs = { 0 };

	errno = 0;

	if (tcgetattr(STDIN_FILENO, &attrs) == -1) {
		err_log(errno, "term_toggle_echo: tcgetattr");
		return;
	}

	switch (state) {
	case ON:
		if (!(attrs.c_lflag & ECHO)) {
			attrs.c_lflag |= ECHO;
			if (tcsetattr(STDIN_FILENO, TCSANOW, &attrs) != 0)
				err_log(errno, "term_toggle_echo: tcsetattr");
		}
		break;
	case OFF:
		if (attrs.c_lflag & ECHO) {
			attrs.c_lflag &= ~ECHO;
			if (tcsetattr(STDIN_FILENO, TCSANOW, &attrs) != 0)
				err_log(errno, "term_toggle_echo: tcsetattr");
		}
		break;
	default:
		sw_assert_not_reached();
		break;
	}
}
