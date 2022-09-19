/* Copyright (C) 2012-2022 Markus Uhlin. All rights reserved. */

#include "common.h"

#include "assertAPI.h"
#include "errHand.h"
#include "term-w32.h"

bool
is_term_resized(int rows, int cols)
{
	UNUSED_PARAM(rows);
	UNUSED_PARAM(cols);
	return is_termresized();
}

struct winsize
term_get_size(void)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	struct winsize size = { 0 };

	if (!GetConsoleScreenBufferInfo(hOut, &info)) {
		err_quit("term_get_size: GetConsoleScreenBufferInfo: %s",
		    errdesc_by_last_err());
	}

	size.ws_row = (info.srWindow.Bottom - info.srWindow.Top) + 1;
	size.ws_col = (info.srWindow.Right - info.srWindow.Left) + 1;

	return size;
}

void
term_restore_title(void)
{
	term_set_title("Windows Console");
}

void
term_set_title(const char *fmt, ...)
{
	char title[900] = { '\0' };
	va_list ap;

	va_start(ap, fmt);
	vsnprintf_s(title, sizeof title, _TRUNCATE, fmt, ap);
	va_end(ap);

	SetConsoleTitle(title);
}

void
term_toggle_echo(on_off_t state)
{
	DWORD	modes;
	HANDLE	stdin_handle;

	if ((stdin_handle = GetStdHandle(STD_INPUT_HANDLE)) ==
	    INVALID_HANDLE_VALUE) {
		err_log(0, "term_toggle_echo: GetStdHandle: %s",
		    errdesc_by_last_err());
		return;
	} else if (!GetConsoleMode(stdin_handle, &modes)) {
		err_log(0, "term_toggle_echo: GetConsoleMode: %s",
		    errdesc_by_last_err());
		return;
	}

	switch (state) {
	case ON:
		if (!(modes & ENABLE_ECHO_INPUT)) {
			modes |= ENABLE_ECHO_INPUT;
			if (!SetConsoleMode(stdin_handle, modes)) {
				err_log(0, "term_toggle_echo: SetConsoleMode",
				    errdesc_by_last_err());
			}
		}
		break;
	case OFF:
		if (modes & ENABLE_ECHO_INPUT) {
			modes &= ~ENABLE_ECHO_INPUT;
			if (!SetConsoleMode(stdin_handle, modes)) {
				err_log(0, "term_toggle_echo: SetConsoleMode",
				    errdesc_by_last_err());
			}
		}
		break;
	default:
		sw_assert_not_reached();
		break;
	}
}
