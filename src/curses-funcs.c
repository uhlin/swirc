/* Additional functions for the Ncurses library
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
#include "curses-funcs.h"

bool            g_cursesMode  = false;
PTR_TO_ENDWIN   g_endwin_fn   = NULL;
PTR_TO_DOUPDATE g_doupdate_fn = NULL;

void
escape_curses(void)
{
	if (g_cursesMode && g_endwin_fn != NULL && g_endwin_fn() != ERR)
		g_cursesMode = false;
}

void
resume_curses(void)
{
	if (!g_cursesMode && g_doupdate_fn != NULL && g_doupdate_fn() != ERR)
		g_cursesMode = true;
}

#if defined(WIN32) && defined(PDC_EXP_EXTRAS)
bool
is_cleared(const WINDOW *win)
{
	return (win != NULL && win->_clear);
}

#if PDC_BUILD < 3900
bool
is_leaveok(const WINDOW *win)
{
	return (win != NULL && win->_leaveit);
}
#endif

bool
is_scrollok(const WINDOW *win)
{
	return (win != NULL && win->_scroll);
}

bool
is_nodelay(const WINDOW *win)
{
	return (win != NULL && win->_nodelay);
}

bool
is_immedok(const WINDOW *win)
{
	return (win != NULL && win->_immed);
}

bool
is_syncok(const WINDOW *win)
{
	return (win != NULL && win->_sync);
}

#if PDC_BUILD < 3900
bool
is_keypad(const WINDOW *win)
{
	return (win != NULL && win->_use_keypad);
}
#endif
#endif
