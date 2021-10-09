/* Copyright (C) 2012-2021 Markus Uhlin. All rights reserved. */

#include "common.h"
#include "errHand.h"
#include "term-w32.h"

bool
is_term_resized(int rows, int cols)
{
    (void) rows;
    (void) cols;
    return is_termresized();
}

struct winsize
term_get_size(void)
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    struct winsize size;

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
    va_list ap;
    char title[900] = { '\0' };

    va_start(ap, fmt);
    vsnprintf_s(title, sizeof title, _TRUNCATE, fmt, ap);
    va_end(ap);

    SetConsoleTitle(title);
}
