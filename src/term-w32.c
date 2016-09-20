/* Copyright (C) 2012-2014, 2016 Markus Uhlin. All rights reserved. */

#include "common.h"
#include "errHand.h"
#include "term-w32.h"

void
term_set_title(const char *fmt, ...)
{
    va_list ap;
    char title[900];

    va_start(ap, fmt);
    vsnprintf_s(title, sizeof title, _TRUNCATE, fmt, ap);
    va_end(ap);

    SetConsoleTitle(title);
}

void
term_restore_title(void)
{
    term_set_title("Windows Console");
}

struct winsize
term_get_size(void)
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    struct winsize size;

    if (!GetConsoleScreenBufferInfo(hOut, &info)) {
	err_quit("GetConsoleScreenBufferInfo error 0x%lx",
		 (unsigned long int) GetLastError());
    }

    size.ws_row = (info.srWindow.Bottom - info.srWindow.Top) + 1;
    size.ws_col = (info.srWindow.Right - info.srWindow.Left) + 1;

    return size;
}
