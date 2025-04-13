/* Copyright (c) 2019 Markus Uhlin <markus.uhlin@icloud.com>
   All rights reserved.

   Permission to use, copy, modify, and distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
   AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
   DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
   PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
   PERFORMANCE OF THIS SOFTWARE. */

#ifndef _COLORS_H_
#define _COLORS_H_

/*
 * Reset all attributes to their defaults
 */
#define NORMAL		"\x1b[0m"

#define UNDERLINE_ON	"\x1b[4m"
#define UNDERLINE_OFF	"\x1b[24m"

#define BLINK_ON	"\x1b[5m"
#define BLINK_OFF	"\x1b[25m"

/*
 * Reverse video
 */
#define REVVID_ON	"\x1b[7m"
#define REVVID_OFF	"\x1b[27m"

#define BLACK		"\x1b[30m"
#define RED		"\x1b[31m"
#define GREEN		"\x1b[32m"
#define BROWN		"\x1b[33m"
#define BLUE		"\x1b[34m"
#define MAGENTA		"\x1b[35m"
#define CYAN		"\x1b[36m"
#define WHITE		"\x1b[37m"

#define BOLDBLACK	"\x1b[1;30m"
#define BOLDRED		"\x1b[1;31m"
#define BOLDGREEN	"\x1b[1;32m"
#define BOLDBROWN	"\x1b[1;33m"
#define BOLDBLUE	"\x1b[1;34m"
#define BOLDMAGENTA	"\x1b[1;35m"
#define BOLDCYAN	"\x1b[1;36m"
#define BOLDWHITE	"\x1b[1;37m"

#define UNDERSCORE_ON	"\x1b[38m"
#define UNDERSCORE_OFF	"\x1b[39m"

#define BGBLACK		"\x1b[40m"
#define BGRED		"\x1b[41m"
#define BGGREEN		"\x1b[42m"
#define BGBROWN		"\x1b[43m"
#define BGBLUE		"\x1b[44m"
#define BGMAGENTA	"\x1b[45m"
#define BGCYAN		"\x1b[46m"
#define BGWHITE		"\x1b[47m"
#define BGDEFAULT	"\x1b[49m"

#if defined(_WIN32) && defined(ENABLE_VIRTUAL_TERMINAL_PROCESSING)
static void
VirtualTerminalProcessing(void)
{
	HANDLE	 output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD	 modes = 0;

	GetConsoleMode(output_handle, &modes);
	modes |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(output_handle, modes);
}
#endif

#endif
