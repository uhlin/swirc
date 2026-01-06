/* Prints and handles text
   Copyright (C) 2012-2026 Markus Uhlin. All rights reserved.

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

#include <clocale>
#include <cwctype>
#include <stdexcept>
#include <string>

#ifdef HAVE_LIBICONV
#include <iconv.h>
#endif

#include "assertAPI.h"
#include "atomicops.h"
#ifdef UNIT_TESTING
#undef UNIT_TESTING
#include "cmocka-c++.h"
#define UNIT_TESTING 1
#endif
#include "config.h"
#if defined(WIN32) && defined(PDC_EXP_EXTRAS)
#include "curses-funcs.h" /* is_scrollok() */
#endif
#include "cursesInit.h"
#include "dataClassify.h"
#include "errHand.h"
#include "libUtils.h"
#include "log.h"
#include "main.h"
#include "network.h"
#include "printtext.h"
#include "readline.h"
#include "strHand.h"
#include "strdup_printf.h"
#include "terminal.h"
#include "theme.h"

#define WADDCH(win, c)        ((void) waddch(win, c))
#define WATTR_OFF(win, attrs) ((void) wattr_off(win, attrs, nullptr))
#define WATTR_ON(win, attrs)  ((void) wattr_on(win, attrs, nullptr))
#define WCOLOR_SET(win, cpn)  ((void) wcolor_set(win, cpn, nullptr))

/****************************************************************
*                                                               *
*  ------------------ Structure definitions ------------------  *
*                                                               *
****************************************************************/

struct message_components {
	STRING	 text;
	int	 indent;

	message_components()
	    : text(nullptr)
	    , indent(0)
	{}
	message_components(STRING p_text, int p_indent)
	    : text(p_text)
	    , indent(p_indent)
	{}
	~message_components()
	{
		free(this->text);
		this->text = nullptr;
	}

	void get_msg(CSTRING, enum message_specifier_type, bool, CSTRING);
};

struct text_decoration_bools {
	bool	 is_blink;
	bool	 is_bold;
	bool	 is_color;
	bool	 is_reverse;
	bool	 is_underline;

	text_decoration_bools() : is_blink(false)
	    , is_bold(false)
	    , is_color(false)
	    , is_reverse(false)
	    , is_underline(false)
	{}

	void
	reset(void)
	{
		this->is_blink		= false;
		this->is_bold		= false;
		this->is_color		= false;
		this->is_reverse	= false;
		this->is_underline	= false;
	}
};

struct case_default_context {
	WINDOW		*win;
	wchar_t		 wc;
	bool		 nextchar_empty;
	int		 indent;
	int		 max_lines;
	ptrdiff_t	 diff;

	case_default_context() : win(nullptr)
	    , wc(L'\0')
	    , nextchar_empty(false)
	    , indent(0)
	    , max_lines(0)
	    , diff(0)
	{
		/* empty */;
	}

	case_default_context(WINDOW *p_win,
	    wchar_t p_wc,
	    bool p_nextchar_empty,
	    int p_indent,
	    int p_max_lines,
	    ptrdiff_t p_diff) : win(p_win)
	    , wc(p_wc)
	    , nextchar_empty(p_nextchar_empty)
	    , indent(p_indent)
	    , max_lines(p_max_lines)
	    , diff(p_diff)
	{
		/* empty */;
	}
};

/****************************************************************
*                                                               *
*  -------------- Objects with external linkage --------------  *
*                                                               *
****************************************************************/

const char g_ascii_soh = 0x01;

#if defined(UNIX)
pthread_mutex_t g_puts_mutex;
#elif defined(WIN32)
HANDLE g_puts_mutex;
#endif

const char g_textdeco_chars[] =
	TXT_BLINK
	TXT_BOLD
	TXT_COLOR
	TXT_NORMAL
	TXT_REVERSE
	TXT_UNDERLINE;

/****************************************************************
*                                                               *
*  -------------- Objects with internal linkage --------------  *
*                                                               *
****************************************************************/

#if defined(UNIX)
static pthread_mutex_t	vprinttext_mutex;
#elif defined(WIN32)
static HANDLE		vprinttext_mutex;
#endif

static struct ptext_colorMap_tag {
	short int	color;
	attr_t		at;
} ptext_colorMap[] = {
	{ COLOR_WHITE,   A_BOLD   },
	{ COLOR_BLACK,   A_NORMAL },
	{ COLOR_BLUE,    A_NORMAL },
	{ COLOR_GREEN,   A_NORMAL },
	{ COLOR_RED,     A_BOLD   },
	{ COLOR_RED,     A_NORMAL },
	{ COLOR_MAGENTA, A_NORMAL },
	{ COLOR_YELLOW,  A_NORMAL },
	{ COLOR_YELLOW,  A_BOLD   },
	{ COLOR_GREEN,   A_BOLD   },
	{ COLOR_CYAN,    A_NORMAL },
	{ COLOR_CYAN,    A_BOLD   },
	{ COLOR_BLUE,    A_BOLD   },
	{ COLOR_MAGENTA, A_BOLD   },
	{ COLOR_BLACK,   A_BOLD   },
	{ COLOR_WHITE,   A_NORMAL },

	/* 16-27 */
	{ 52,  0 }, { 94,  0 }, { 100, 0 },
	{ 58,  0 }, { 22,  0 }, { 29,  0 },
	{ 23,  0 }, { 24,  0 }, { 17,  0 },
	{ 54,  0 }, { 53,  0 }, { 89,  0 },

	/* 28-39 */
	{ 88,  0 }, { 130, 0 }, { 142, 0 },
	{ 64,  0 }, { 28,  0 }, { 35,  0 },
	{ 30,  0 }, { 25,  0 }, { 18,  0 },
	{ 91,  0 }, { 90,  0 }, { 125, 0 },

	/* 40-51 */
	{ 124, 0 }, { 166, 0 }, { 184, 0 },
	{ 106, 0 }, { 34,  0 }, { 49,  0 },
	{ 37,  0 }, { 33,  0 }, { 19,  0 },
	{ 129, 0 }, { 127, 0 }, { 161, 0 },

	/* 52-63 */
	{ 196, 0 }, { 208, 0 }, { 226, 0 },
	{ 154, 0 }, { 46,  0 }, { 86,  0 },
	{ 51,  0 }, { 75,  0 }, { 21,  0 },
	{ 171, 0 }, { 201, 0 }, { 198, 0 },

	/* 64-75 */
	{ 203, 0 }, { 215, 0 }, { 227, 0 },
	{ 191, 0 }, { 83,  0 }, { 122, 0 },
	{ 87,  0 }, { 111, 0 }, { 63,  0 },
	{ 177, 0 }, { 207, 0 }, { 205, 0 },

	/* 76-87 */
	{ 217, 0 }, { 223, 0 }, { 229, 0 },
	{ 193, 0 }, { 157, 0 }, { 158, 0 },
	{ 159, 0 }, { 153, 0 }, { 147, 0 },
	{ 183, 0 }, { 219, 0 }, { 212, 0 },

	/* 88-99 */
	{ 16,  0 }, { 233, 0 }, { 235, 0 },
	{ 237, 0 }, { 239, 0 }, { 241, 0 },
	{ 244, 0 }, { 247, 0 }, { 250, 0 },
	{ 254, 0 }, { 231, 0 }, { -1,  0 },
};

/****************************************************************
*                                                               *
*  ---------------------    Functions    ---------------------  *
*                                                               *
****************************************************************/

static char *get_buffer(CSTRING) NONNULL;

static void
addmbs(WINDOW *win, const uint8_t *mbs)
{
#if defined(UNIX)
	chtype c;
	const uint8_t *p = mbs;

	while ((c = *p++) != '\0')
		WADDCH(win, c);
#elif defined(WIN32)
	(void) waddnstr(win, reinterpret_cast<CSTRING>(mbs), -1);
#endif
}

static void
append_newline(wchar_t **wc_buf)
{
	const size_t newsize = size_product(wcslen(*wc_buf) + sizeof "\n",
	    sizeof(wchar_t));

	*wc_buf = static_cast<wchar_t *>(xrealloc(*wc_buf, newsize));

	if ((errno = sw_wcscat(*wc_buf, L"\n", newsize)) != 0)
		err_sys("%s", __func__);
}

/**
 * Toggle blink ON/OFF. Don't actually use A_BLINK because it's
 * annoying.
 *
 * @param[in]     win      Target window.
 * @param[in,out] is_blink Is blink state.
 * @return Void
 */
static void
case_blink(WINDOW *win, bool *is_blink)
{
	UNUSED_PARAM(win);

	if (!*is_blink)
		*is_blink = true;
	else
		*is_blink = false;
}

/**
 * Toggle bold ON/OFF
 *
 * @param[in]     win     Target window
 * @param[in,out] is_bold Is bold state
 * @return Void
 */
static void
case_bold(WINDOW *win, bool *is_bold)
{
	if (!*is_bold) {
		WATTR_ON(win, A_BOLD);
		*is_bold = true;
	} else {
		WATTR_OFF(win, A_BOLD);
		*is_bold = false;
	}
}

/**
 * Convert a wide-character to a multibyte sequence. The storage for
 * the multibyte sequence is allocated on the heap and must be
 * free()'d.
 *
 * @param wc Wide-character
 * @param bytes_out How many bytes is the wc?
 * @return The result
 */
static UCHARPTR
convert_wc(wchar_t wc, size_t &bytes_out)
{
	const size_t	 size = MB_LEN_MAX;
	mbstate_t	 ps;
	size_t		 bytes_written = 0;
	UCHARPTR	 mbs = static_cast<UCHARPTR>(xmalloc(size + 1));

	BZERO(&ps, sizeof(mbstate_t));
	mbs[size] = '\0';

#ifdef HAVE_BCI
	if (wc == L'\0' ||
	    (errno = wcrtomb_s(&bytes_written, reinterpret_cast<STRING>(mbs),
	    size, wc, &ps)) != 0 ||
	    bytes_written == g_conversion_failed ||
	    bytes_written > size) {
		if (wc != L'\0' && errno != 0)
			err_log(errno, "printtext: %s: wcrtomb_s", __func__);
		*mbs = '\0';
		bytes_out = 0;
		return mbs;
	}
#else
	if (wc == L'\0' ||
	    (bytes_written = wcrtomb(reinterpret_cast<STRING>(mbs), wc, &ps)) ==
	    g_conversion_failed ||
	    bytes_written > size) {
		if (wc != L'\0')
			err_log(EILSEQ, "printtext: %s: wcrtomb", __func__);
		*mbs = '\0';
		bytes_out = 0;
		return mbs;
	}
#endif

	mbs[bytes_written] = '\0';
	bytes_out = bytes_written;
	return static_cast<UCHARPTR>(xrealloc(mbs, bytes_written + 1));
}

static inline void
putbyte(char *cp, const uint8_t b)
{
	cp[0] = static_cast<char>(b);
	cp[1] = '\0';
}

/***************************************************
 *
 * check for ^CN
 *
 ***************************************************/
static cc_check_t
check_for_part1(wchar_t **bufp, char *fg)
{
	size_t		 bytes_out = 0;
	UCHARPTR	 mbs;

	if (!*++(*bufp))
		return BUF_EOF;

	mbs = convert_wc(**bufp, bytes_out);

	if (bytes_out != 1 || !sw_isdigit(*mbs)) {
		--(*bufp);
		free(mbs);
		return STOP_INTERPRETING;
	}

	fg[0] = static_cast<char>(*mbs);
	fg[1] = '\0';
	free(mbs);
	return GO_ON;
}

/***************************************************
 *
 * check for ^CNN or ^CN,
 *
 ***************************************************/
static cc_check_t
check_for_part2(wchar_t **bufp, char *fg, bool *has_comma)
{
	size_t		 bytes_out = 0;
	UCHARPTR	 mbs;

	if (!*++(*bufp))
		return BUF_EOF;

	mbs = convert_wc(**bufp, bytes_out);

	if (bytes_out != 1 || (!sw_isdigit(*mbs) && *mbs != ',')) {
		--(*bufp);
		free(mbs);
		return STOP_INTERPRETING;
	}

	if (sw_isdigit(*mbs))
		putbyte(fg, *mbs);
	else if (*mbs == ',')
		*has_comma = true;
	else
		sw_assert_not_reached();

	free(mbs);
	return GO_ON;
}

/***************************************************
 *
 * check for ^CNN, or ^CN,N
 *
 ***************************************************/
static cc_check_t
check_for_part3(wchar_t **bufp, bool *has_comma, bool fg_complete, char *bg)
{
	size_t		 bytes_out = 0;
	UCHARPTR	 mbs;

	if (!*++(*bufp))
		return BUF_EOF;

	mbs = convert_wc(**bufp, bytes_out);

	if (bytes_out != 1 || (*mbs != ',' && !sw_isdigit(*mbs))) {
		--(*bufp);
		free(mbs);
		return STOP_INTERPRETING;
	} else if (*mbs == ',' && *has_comma) {
		--(*bufp);
		free(mbs);
		return STOP_INTERPRETING;
	} else if (*mbs != ',' && fg_complete) {
		--(*bufp);
		free(mbs);
		return STOP_INTERPRETING;
	}

	if (*mbs == ',')
		*has_comma = true;
	else if (sw_isdigit(*mbs))
		putbyte(bg, *mbs);
	else
		sw_assert_not_reached();

	free(mbs);
	return GO_ON;
}

/***************************************************
 *
 * check for ^CNN,N or ^CN,NN
 *
 ***************************************************/
static cc_check_t
check_for_part4(wchar_t **bufp, bool got_digit_bg, char *bg)
{
	size_t		 bytes_out = 0;
	UCHARPTR	 mbs;

	if (!*++(*bufp))
		return BUF_EOF;

	mbs = convert_wc(**bufp, bytes_out);

	if (bytes_out != 1 || !sw_isdigit(*mbs)) {
		--(*bufp);
		free(mbs);
		return STOP_INTERPRETING;
	} else if (got_digit_bg) {
		putbyte(++bg, *mbs);
		free(mbs);
		return STOP_INTERPRETING;
	}

	putbyte(bg, *mbs);
	free(mbs);
	return GO_ON;
}

/***************************************************
 *
 * check for ^CNN,NN
 *
 ***************************************************/
static cc_check_t
check_for_part5(wchar_t **bufp, char *bg)
{
	size_t		 bytes_out = 0;
	UCHARPTR	 mbs;

	if (!*++(*bufp))
		return BUF_EOF;

	mbs = convert_wc(**bufp, bytes_out);

	if (bytes_out != 1 || !sw_isdigit(*mbs)) {
		--(*bufp);
		free(mbs);
		return STOP_INTERPRETING;
	}

	putbyte(bg, *mbs);
	free(mbs);
	return GO_ON;
}

static void
map_color(short int *inout, const short int i, const short int colorMap_size,
    attr_t *attr_out)
{
	switch (*inout) {
	case COLOR_BLACK:
		if (ptext_colorMap[i % colorMap_size].at == A_BOLD) {
			*inout = GREY;
			*attr_out = A_NORMAL;
		}
		break;
	case COLOR_WHITE:
		if (ptext_colorMap[i % colorMap_size].at == A_NORMAL) {
			*inout = LIGHT_GREY;
			*attr_out = A_NORMAL;
		}
		break;
	default:
		/* Not black/white. */
		break;
	}
}

/**
 * Set color for output in a window.
 *
 * @param[in]  win      Window
 * @param[out] is_color Is color state
 * @param[in]  num1     Number for foreground
 * @param[in]  num2     Number for background
 * @return Void
 */
void
printtext_set_color(WINDOW *win, bool *is_color, short int num1, short int num2)
{
	attr_t attr = 0xff;
	const short int num_colorMap_entries = static_cast<short int>
	    (COLORS >= 256 ? ARRAY_SIZE(ptext_colorMap) : 16);
	short int fg, bg, resolved_pair;

	sw_assert(num1 >= 0);

	fg = ptext_colorMap[num1 % num_colorMap_entries].color;
	if (num2 < 0)
		bg = -1;
	else
		bg = ptext_colorMap[num2 % num_colorMap_entries].color;

	if (COLORS >= 16 && can_change_color()) {
		map_color(&fg, num1, num_colorMap_entries, &attr);
		if (num2 >= 0)
			map_color(&bg, num2, num_colorMap_entries, &attr);
	}

	if (attr != A_NORMAL)
		attr = ptext_colorMap[num1 % num_colorMap_entries].at;

	if ((resolved_pair = color_pair_find(fg, bg)) == -1) {
		WCOLOR_SET(win, 0);
		*is_color = false;
		return;
	}

	(void) wattr_set(win, attr, resolved_pair, nullptr);
	*is_color = true;
}

static void
init_numbers(CSTRING fg, CSTRING bg, short int &num1, short int &num2)
{
	struct integer_context intctx("term_background", 0, 15, 1);

	num1 = static_cast<short int>(atoi(fg));

	if (!isEmpty(bg))
		num2 = static_cast<short int>(atoi(bg));
	else if (isEmpty(bg) && theme_bool("term_use_default_colors", true))
		num2 = -1;
	else
		num2 = static_cast<short int>(theme_integer(&intctx));
}

/**
 * Handle and interpret color codes.
 *
 * @param win      Window
 * @param is_color Is color state
 * @param bufp     Buffer pointer
 * @return Void
 */
static void
case_color(WINDOW *win, bool *is_color, wchar_t **bufp)
{
	bool		has_comma = false;
	cc_check_t	res;
	char		bg[10] = { 0 };
	char		fg[10] = { 0 };
	short int	num1 = -1;
	short int	num2 = -1;

	if (*is_color) {
		WCOLOR_SET(win, 0);
		*is_color = false;
	}

	if (check_for_part1(bufp, &fg[0]) != GO_ON)
		return;
	else if ((res = check_for_part2(bufp, &fg[1], &has_comma)) == BUF_EOF)
		return;
	else if (res == STOP_INTERPRETING)
		/* null */;
	else if ((res = check_for_part3(bufp, &has_comma, fg[1] != '\0',
	    &bg[0])) == BUF_EOF)
		return;
	else if (res == STOP_INTERPRETING)
		/* null */;
	else if ((res = check_for_part4(bufp, bg[0] != '\0', &bg[0])) ==
	    BUF_EOF)
		return;
	else if (res == STOP_INTERPRETING)
		/* null */;
	else if (check_for_part5(bufp, &bg[1]) == BUF_EOF)
		return;

	init_numbers(&fg[0], &bg[0], num1, num2);
	printtext_set_color(win, is_color, num1, num2);

	if (has_comma && !(bg[0]))
		--(*bufp);
}

/**
 * Do indent
 */
static void
do_indent(WINDOW *win, const int indent, int *insert_count)
{
	attr_t		 attrs = 0;
	short int	 pair = 0;

	(void) wattr_get(win, &attrs, &pair, nullptr);

	/* turn off all attributes during indentation */
	(void) wattr_set(win, A_NORMAL, 0, nullptr);

	for (int i = 0; i < indent; i++) {
		WADDCH(win, ' ');
		(*insert_count)++;
	}

	/* restore attributes after indenting */
	(void) wattr_set(win, attrs, pair, nullptr);
}

static void
new_row(WINDOW *win, int *insert_count, int *rep_count)
{
	WADDCH(win, '\n');
	*insert_count = 0;

	if (isValid(rep_count))
		(*rep_count)++; // NOLINT: false positive
}

/**
 * Start on a new row?
 */
static bool
start_on_a_new_row(const ptrdiff_t sum, WINDOW *win)
{
	return (sum < getmaxx(win) ? false : true);
}

/**
 * Handles switch default in printtext_puts()
 *
 * @param[in]     ctx          Context structure
 * @param[in,out] rep_count    "Represent" count
 * @param[out]    lines_count  Lines count
 * @param[out]    insert_count Insert count
 * @return Void
 */
static void
case_default(const struct case_default_context *ctx, int *rep_count,
    int *lines_count, int *insert_count)
{
	size_t		 dummy;
	UCHARPTR	 mbs;

	if (!iswprint(ctx->wc) && ctx->wc != L'\n')
		return;

	mbs = convert_wc(ctx->wc, dummy);
	UNUSED_VAR(dummy);

	if (!is_scrollok(ctx->win)) {
		addmbs(ctx->win, mbs);
		free(mbs);
		return;
	}

	/* -------------------------------------------------- */

	const bool care_about_indent = (ctx->indent > 0);
	const bool care_about_max_lines = (ctx->max_lines > 0);

	if (ctx->wc == L'\n') {
		new_row(ctx->win, insert_count, rep_count);

		if (care_about_max_lines &&
		    !(++ (*lines_count) < ctx->max_lines)) {
			free(mbs);
			return;
		}

		if (!ctx->nextchar_empty && care_about_indent)
			do_indent(ctx->win, ctx->indent, insert_count);
	} else if (!start_on_a_new_row((*insert_count) + ctx->diff + 1,
		    ctx->win)) {
		addmbs(ctx->win, mbs);
		(*insert_count) += xwcwidth(ctx->wc, 2); // XXX
	} else {
		/*
		 * Start on a new row
		 */

		new_row(ctx->win, insert_count, rep_count);

		if (care_about_max_lines &&
		    !(++ (*lines_count) < ctx->max_lines)) {
			free(mbs);
			return;
		}

		if (care_about_indent)
			do_indent(ctx->win, ctx->indent, insert_count);

		if (ctx->wc != L' ') {
			addmbs(ctx->win, mbs);
			(*insert_count) += xwcwidth(ctx->wc, 2);
		}
	}

	free(mbs);
}

/**
 * Toggle reverse ON/OFF
 *
 * @param[in]     win        Target window
 * @param[in,out] is_reverse Is reverse state
 * @return Void
 */
static void
case_reverse(WINDOW *win, bool *is_reverse)
{
	if (!*is_reverse) {
		WATTR_ON(win, A_REVERSE);
		*is_reverse = true;
	} else {
		WATTR_OFF(win, A_REVERSE);
		*is_reverse = false;
	}
}

/**
 * Toggle underline ON/OFF
 *
 * @param[in]     win          Target window
 * @param[in,out] is_underline Is underline state
 * @return Void
 */
static void
case_underline(WINDOW *win, bool *is_underline)
{
	if (!*is_underline) {
		WATTR_ON(win, A_UNDERLINE);
		*is_underline = true;
	} else {
		WATTR_OFF(win, A_UNDERLINE);
		*is_underline = false;
	}
}

static void
set_indent(int *indent, CSTRING fmt, ...)
{
	STRING		 str = nullptr;
	size_t		 bytes_convert;
	va_list		 ap;
	wchar_t		 wcs[400] = { L'\0' };

	va_start(ap, fmt);
	str = strdup_vprintf(fmt, ap);
	va_end(ap);

	bytes_convert = xmbstowcs(addrof(wcs[0]), squeeze_text_deco(str),
	    ARRAY_SIZE(wcs) - 1);
	wcs[ARRAY_SIZE(wcs) - 1] = L'\0';
	free(str);
	if (bytes_convert == g_conversion_failed) {
		*indent = 0;
		return;
	}
	*indent = xwcswidth(addrof(wcs[0]), 2);
}

/**
 * Get message components
 *
 * @param unproc_msg Unprocessed message
 * @param spec_type  "Specifier"
 * @param include_ts Include timestamp?
 * @param srv_time   Server time
 */
void
message_components::get_msg(CSTRING unproc_msg,
    enum message_specifier_type spec_type,
    bool include_ts,
    CSTRING srv_time)
{
	if (include_ts) {
		STRING ts = nullptr;

		if (srv_time)
			ts = sw_strdup(srv_time);
		else
			ts = sw_strdup(current_time(Theme("time_format")));

		switch (spec_type) {
		case TYPE_SPEC1:
			this->text = strdup_printf("%s %s %s", ts, THE_SPEC1,
			    unproc_msg);
			set_indent(& (this->indent), "%s %s ", ts, THE_SPEC1);
			break;
		case TYPE_SPEC2:
			this->text = strdup_printf("%s %s %s", ts, THE_SPEC2,
			    unproc_msg);
			set_indent(& (this->indent), "%s %s ", ts, THE_SPEC2);
			break;
		case TYPE_SPEC3:
			this->text = strdup_printf("%s %s %s", ts, THE_SPEC3,
			    unproc_msg);
			set_indent(& (this->indent), "%s %s ", ts, THE_SPEC3);
			break;
		case TYPE_SPEC1_SPEC2:
			this->text = strdup_printf("%s %s %s %s", ts, THE_SPEC1,
			    THE_SPEC2, unproc_msg);
			set_indent(& (this->indent), "%s %s %s ", ts, THE_SPEC1,
			    THE_SPEC2);
			break;
		case TYPE_SPEC1_FAILURE:
			this->text = strdup_printf("%s %s %s %s", ts, THE_SPEC1,
			    GFX_FAILURE, unproc_msg);
			set_indent(& (this->indent), "%s %s %s ", ts, THE_SPEC1,
			    GFX_FAILURE);
			break;
		case TYPE_SPEC1_SUCCESS:
			this->text = strdup_printf("%s %s %s %s", ts, THE_SPEC1,
			    GFX_SUCCESS, unproc_msg);
			set_indent(& (this->indent), "%s %s %s ", ts, THE_SPEC1,
			    GFX_SUCCESS);
			break;
		case TYPE_SPEC1_WARN:
			this->text = strdup_printf("%s %s %s %s", ts, THE_SPEC1,
			    GFX_WARN, unproc_msg);
			set_indent(& (this->indent), "%s %s %s ", ts, THE_SPEC1,
			    GFX_WARN);
			break;
		case TYPE_SPEC_NONE:
		default:
			this->text = strdup_printf("%s %s", ts, unproc_msg);
			set_indent(& (this->indent), "%s ", ts);
			break;
		}

		free(ts);
	} else if (!include_ts) {
		/*
		 * the same but no timestamp
		 */

		switch (spec_type) {
		case TYPE_SPEC1:
			this->text = strdup_printf("%s %s", THE_SPEC1,
			    unproc_msg);
			set_indent(& (this->indent), "%s ", THE_SPEC1);
			break;
		case TYPE_SPEC2:
			this->text = strdup_printf("%s %s", THE_SPEC2,
			    unproc_msg);
			set_indent(& (this->indent), "%s ", THE_SPEC2);
			break;
		case TYPE_SPEC3:
			this->text = strdup_printf("%s %s", THE_SPEC3,
			    unproc_msg);
			set_indent(& (this->indent), "%s ", THE_SPEC3);
			break;
		case TYPE_SPEC1_SPEC2:
			this->text = strdup_printf("%s %s %s", THE_SPEC1,
			    THE_SPEC2, unproc_msg);
			set_indent(& (this->indent), "%s %s ", THE_SPEC1,
			    THE_SPEC2);
			break;
		case TYPE_SPEC1_FAILURE:
			this->text = strdup_printf("%s %s %s", THE_SPEC1,
			    GFX_FAILURE, unproc_msg);
			set_indent(& (this->indent), "%s %s ", THE_SPEC1,
			    GFX_FAILURE);
			break;
		case TYPE_SPEC1_SUCCESS:
			this->text = strdup_printf("%s %s %s", THE_SPEC1,
			    GFX_SUCCESS, unproc_msg);
			set_indent(& (this->indent), "%s %s ", THE_SPEC1,
			    GFX_SUCCESS);
			break;
		case TYPE_SPEC1_WARN:
			this->text = strdup_printf("%s %s %s", THE_SPEC1,
			    GFX_WARN, unproc_msg);
			set_indent(& (this->indent), "%s %s ", THE_SPEC1,
			    GFX_WARN);
			break;
		case TYPE_SPEC_NONE:
		default:
			this->text = sw_strdup(unproc_msg);
			this->indent = 0;
			break;
		}
	} else {
		sw_assert_not_reached();
	}

	sw_assert(this->text != nullptr);

	if (g_no_colors)
		this->text = squeeze_text_deco(this->text);
}

/**
 * WIN32 specific: attempt to convert multibyte character string to
 * wide-character string by using UTF-8. The storage is obtained with
 * xcalloc().
 *
 * @param buf Buffer to convert.
 * @return A wide-character string, or NULL on error.
 */
#if WIN32
static wchar_t *
windows_convert_to_utf8(CSTRING buf)
{
	const int	 size = static_cast<int>(strlen(buf) + 1);
	wchar_t		*out = static_cast<wchar_t *>(xcalloc(size,
			     sizeof *out));

	if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, buf, -1, out,
	    size) > 0)
		return out;
	free(out);
	return nullptr;
}
#endif

static void
restore_original(CSTRING locale)
{
	if (xsetlocale(LC_CTYPE, locale) == nullptr) {
		err_log(0, "%s: cannot restore original locale (%s)", __func__,
		    locale);
	}
}

static void
try_convert_clean_up(struct locale_info *li, STRING orig, STRING tmp)
{
	free_locale_info(li);
	free(orig);
	free(tmp);
}

/**
 * Attempt convert multibyte character string to wide-character string
 * by using a specific codeset. The storage is dynamically allocated.
 *
 * @param buf     Buffer to convert
 * @param codeset Codeset to use
 * @return A wide-character string, or NULL on error.
 */
static wchar_t *
try_convert_buf_with_cs(CSTRING buf, CSTRING codeset)
{
	STRING			 original_locale = nullptr;
	STRING			 tmp_locale = nullptr;
	struct locale_info	*li = nullptr;
	wchar_t			*out = nullptr;

	try {
		size_t bytes_convert = 0;

		li = get_locale_info(LC_CTYPE);

		if (buf == nullptr || codeset == nullptr) {
			throw std::runtime_error("invalid arguments");
		} else if (li->lang_and_territory == nullptr ||
			   li->codeset == nullptr) {
			throw std::runtime_error("failed to get locale "
			    "information");
		}

		original_locale = strdup_printf("%s.%s", li->lang_and_territory,
		    li->codeset);
		tmp_locale = strdup_printf("%s.%s", li->lang_and_territory,
		    codeset);

		const size_t size = strlen(buf) + 1;
		out = static_cast<wchar_t *>(xcalloc(size, sizeof *out));

		if (xsetlocale(LC_CTYPE, tmp_locale) == nullptr ||
		    (bytes_convert = xmbstowcs(out, buf, size - 1)) ==
		    g_conversion_failed) {
			restore_original(original_locale);
			throw std::runtime_error("conversion failed");
		}

		if (bytes_convert >= (size - 1))
			out[size - 1] = 0L;
		restore_original(original_locale);

		try_convert_clean_up(li, original_locale, tmp_locale);
		return out;
	} catch (const std::bad_alloc &e) {
		err_exit(ENOMEM, "%s: %s", __func__, e.what());
	} catch (const std::runtime_error &e) {
		debug("%s: %s", __func__, e.what());
		try_convert_clean_up(li, original_locale, tmp_locale);
		free(out);
	} catch (...) {
		sw_assert_not_reached();
	}

	return nullptr;
}

/**
 * Convert multibyte character string to wide-character string, using
 * different encodings. The storage is dynamically allocated.
 *
 * @param in_buf In buffer.
 * @return A wide-character string.
 */
static wchar_t *
perform_convert_buffer(const char **in_buf)
{
	bool		 chars_lost = false;
	stringarray_t	 ar = {
#if defined(UNIX)
		"UTF-8", "utf8",
		"ISO-8859-1", "ISO8859-1", "iso88591",
		"ISO-8859-15", "ISO8859-15", "iso885915",
#elif defined(WIN32)
		"65001", /* UTF-8 */
		"28591", /* ISO 8859-1 Latin 1 */
		"28605", /* ISO 8859-15 Latin 9 */
#endif
	};
	mbstate_t	 ps;
	size_t		 outlen = 0;
	size_t		 size = 0;
	wchar_t		*out = nullptr;

#if WIN32
	if ((out = windows_convert_to_utf8(*in_buf)) != nullptr)
		return out;
#endif

	for (const char **ar_p = &ar[0]; ar_p < &ar[ARRAY_SIZE(ar)]; ar_p++) {
		if ((out = try_convert_buf_with_cs(*in_buf, *ar_p)) !=
		    nullptr) {
			/*
			 * success
			 */
			return out;
		}
	}

	/* fallback solution... */
	size = strlen(*in_buf) + 1;
	out = static_cast<wchar_t *>(xcalloc(size, sizeof *out));
	BZERO(&ps, sizeof(mbstate_t));

/*
 * mbsrtowcs() is complex enough to use
 */
#if WIN32
#pragma warning(disable: 4996)
#endif

	while (errno = 0, true) {
		outlen = wcslen(out);

		if (mbsrtowcs(&out[outlen], in_buf, (size - outlen) - 1, &ps) ==
		    g_conversion_failed &&
		    errno == EILSEQ) {
			chars_lost = true;
			(*in_buf)++;
		} else
			break;
	}

/*
 * Reset warning behavior to its default value
 */
#if WIN32
#pragma warning(default: 4996)
#endif

	out[size - 1] = 0L;

	if (chars_lost)
		err_log(EILSEQ, "In %s: characters lost", __func__);
	return out;
}

/**
 * Create mutex "g_puts_mutex".
 */
static void
puts_mutex_init(void)
{
	mutex_new(&g_puts_mutex);
}

static void
replace_characters_with_spaces(wchar_t *wc_buf, const wchar_t *set)
{
	wchar_t *wcp;

	while ((wcp = wcspbrk(wc_buf, set)) != nullptr)
		*wcp = L' ';
}

/**
 * Create mutex "vprinttext_mutex".
 */
static void
vprinttext_mutex_init(void)
{
	mutex_new(&vprinttext_mutex);
}

PPRINTTEXT_CONTEXT
printtext_context_new(PIRC_WINDOW window, enum message_specifier_type spec_type,
    bool include_ts)
{
	PPRINTTEXT_CONTEXT ctx =
	    static_cast<PPRINTTEXT_CONTEXT>(xcalloc(sizeof *ctx, 1));

	ctx->window     = window;
	ctx->spec_type  = spec_type;
	ctx->include_ts = include_ts;

	BZERO(ctx->server_time, sizeof ctx->server_time);
	ctx->has_server_time = false;

	return ctx;
}

void
printtext_context_destroy(PPRINTTEXT_CONTEXT ctx)
{
	free(ctx);
}

void
printtext_context_init(PPRINTTEXT_CONTEXT ctx, PIRC_WINDOW window,
    enum message_specifier_type spec_type, bool include_ts)
{
	if (ctx == nullptr) {
		err_log(EINVAL, "%s", __func__);
		return;
	}

	ctx->window     = window;
	ctx->spec_type  = spec_type;
	ctx->include_ts = include_ts;

	BZERO(ctx->server_time, sizeof ctx->server_time);
	ctx->has_server_time = false;
}

/**
 * Search for a color pair with given foreground/background.
 *
 * @param fg Foreground
 * @param bg Background
 * @return A color pair number, or -1 if not found.
 */
short int
color_pair_find(short int fg, short int bg)
{
	const short int gipp1 = g_initialized_pairs + 1;
	short int x, y;

	for (short int pnum = 1; pnum < gipp1; pnum++) {
		if (pair_content(pnum, &x, &y) == ERR)
			return -1;
		else if (x == fg && y == bg) /* found match */
			return pnum;
	}

	return -1;
}

/**
 * Print an error message to the active window and free the memory
 * space pointed to by @cp
 */
void
print_and_free(CSTRING msg, char *cp)
{
	PRINTTEXT_CONTEXT ctx;

	if (msg == nullptr)
		err_exit(EINVAL, "%s", __func__);

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_FAILURE, true);
	printtext(&ctx, "%s", msg);
	free(cp);
}

void
printf_and_free(char *cp, CSTRING fmt, ...)
{
	PPRINTTEXT_CONTEXT	ctx;
	va_list			ap;

	ctx = printtext_context_new(g_active_window, TYPE_SPEC1_FAILURE, true);

	va_start(ap, fmt);
	vprinttext(ctx, fmt, ap);
	va_end(ap);

	printtext_context_destroy(ctx);
	free(cp);
}

/**
 * Swirc messenger
 *
 * @param ctx Context structure
 * @param fmt Format control
 * @return Void
 */
void
printtext(PPRINTTEXT_CONTEXT ctx, CSTRING fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprinttext(ctx, fmt, ap);
	va_end(ap);
}

#ifdef UNIT_TESTING
void
printtext_convert_wc_test1(void **state)
{
	const wchar_t array[] =
	    L"0123456789"
	    L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    L"abcdefghijklmnopqrstuvwxyz";
	const size_t array_len = wcslen(array);

	for (size_t i = 0; i < array_len; i++) {
		size_t		 bytes_out = 0;
		UCHARPTR	 mbs;

		mbs = convert_wc(array[i], bytes_out);
		assert_non_null(mbs);
		assert_true(bytes_out == 1);
		free(mbs);
	}
	UNUSED_PARAM(state);
}

void
printtext_convert_wc_test2(void **state)
{
	const wchar_t array[] = {L'Å',L'Ä',L'Ö',L'å',L'ä',L'ö'};

	for (size_t i = 0; i < ARRAY_SIZE(array); i++) {
		size_t		 bytes_out = 0;
		UCHARPTR	 mbs;

		mbs = convert_wc(array[i], bytes_out);
		assert_non_null(mbs);
		if (!*mbs) {
			assert_true(bytes_out == 0);
		} else {
			assert_true(bytes_out > 1);
			print_message("%s: in=%lc out=%s\r\n", __func__,
			    array[i], mbs);
		}
		free(mbs);
	}
	UNUSED_PARAM(state);
}
#endif // UNIT_TESTING

void
printtext_print(CSTRING what, CSTRING fmt, ...)
{
	PPRINTTEXT_CONTEXT	ctx;
	va_list			ap;

	if (fmt == nullptr)
		err_exit(EINVAL, "%s", __func__);
	else if (g_active_window == nullptr)
		return;

	ctx = printtext_context_new(g_active_window, TYPE_SPEC1, true);

	if (what == nullptr)
		/* null */;
	else if (strings_match(what, "sp1"))
		ctx->spec_type = TYPE_SPEC1;
	else if (strings_match(what, "sp2"))
		ctx->spec_type = TYPE_SPEC2;
	else if (strings_match(what, "sp3"))
		ctx->spec_type = TYPE_SPEC3;
	else if (strings_match(what, "err"))
		ctx->spec_type = TYPE_SPEC1_FAILURE;
	else if (strings_match(what, "success"))
		ctx->spec_type = TYPE_SPEC1_SUCCESS;
	else if (strings_match(what, "warn"))
		ctx->spec_type = TYPE_SPEC1_WARN;
	else if (strings_match(what, "none"))
		ctx->spec_type = TYPE_SPEC_NONE;

	va_start(ap, fmt);
	vprinttext(ctx, fmt, ap);
	va_end(ap);

	printtext_context_destroy(ctx);
}

static char *
get_buffer(CSTRING orig)
{
#ifdef HAVE_LIBICONV
#define UTF8_MAXBYTE 4
	static stringarray_t fromcode = {
		"UTF-8",
		"ISO-8859-1",
		"ISO-8859-15",
		"WINDOWS-1251",
		"WINDOWS-1252",
	};

	if (!config_bool("iconv_conversion", true))
		return sw_strdup(orig);

	for (immutable_cp_t str : fromcode) {
		char		*in, *orig_copy, *out, *out_p;
		iconv_t		 cd;
		size_t		 inbytes, outbytes, outsize;

		if ((cd = iconv_open("UTF-8", str)) == reinterpret_cast
		    <iconv_t>(-1))
			continue;
		orig_copy = sw_strdup(orig);
		in = addrof(orig_copy[0]);
		inbytes = strlen(in);
		outbytes = outsize = size_product(inbytes, UTF8_MAXBYTE);
		out = static_cast<char *>(xmalloc(outbytes + 1));
		out[outbytes] = '\0';
		out_p = addrof(out[0]);
		errno = 0;
		if (iconv(cd, &in, &inbytes, &out_p, &outbytes) == static_cast
		    <size_t>(-1)) {
			free(orig_copy);
			free(out);
			(void) iconv_close(cd);
			continue;
		} else if (inbytes == 0 && errno == 0) {
			out[outsize - outbytes] = '\0';
			free(orig_copy);
			(void) iconv_close(cd);
			return static_cast<char *>(xrealloc(out,
			    strlen(out) + 1));
		}
		free(orig_copy);
		free(out);
		(void) iconv_close(cd);
	}
	return sw_strdup(orig);
#else
	/*
	 * !HAVE_LIBICONV
	 */
	return sw_strdup(orig);
#endif
}

static void
puts_mutex_init_doit()
{
#if defined(UNIX)
	static pthread_once_t init_done = PTHREAD_ONCE_INIT;

	if ((errno = pthread_once(&init_done, puts_mutex_init)) != 0)
		err_sys("%s: pthread_once", __func__);
#elif defined(WIN32)
	static init_once_t init_done = ONCE_INITIALIZER;

	if ((errno = init_once(&init_done, puts_mutex_init)) != 0)
		err_sys("%s: init_once", __func__);
#endif
}

/**
 * Output data to window
 *
 * @param[in]  pwin      Panel window where the output is to be displayed.
 * @param[in]  buf       A buffer that should contain the data to be written to
 *                       'pwin'.
 * @param[in]  indent    If >0 indent text with this number of blanks.
 * @param[in]  max_lines If >0 write at most this number of lines.
 * @param[out] rep_count "Represent count". How many actual lines does this
 *                       contribution represent in the output window?
 *                       (Passing NULL is ok.)
 * @return Void
 */
void
printtext_puts(WINDOW *pwin, CSTRING buf, int indent, int max_lines,
    int *rep_count)
{
	char		*tmpbuf = nullptr;
	const char	*tmpbuf_p = nullptr;
	int		 insert_count = 0;
	int		 lines_count = 0;
	struct text_decoration_bools
			 booleans; // calls constructor
	wchar_t		*wc_buf = nullptr;

	puts_mutex_init_doit();

	if (isValid(rep_count))
		*rep_count = 0;
	if (pwin == nullptr || buf == nullptr)
		err_exit(EINVAL, "%s", __func__);
	else if (strings_match(buf, "") || term_is_too_small())
		return;

	mutex_lock(&g_puts_mutex);
	tmpbuf = get_buffer(buf);
	tmpbuf_p = addrof(tmpbuf[0]);
	wc_buf = perform_convert_buffer(&tmpbuf_p);
	free(tmpbuf);

	if (is_scrollok(pwin))
		append_newline(&wc_buf);

	replace_characters_with_spaces(wc_buf, L"\f\t\v");

	wchar_t *wc_bufp = &wc_buf[0];
	bool max_lines_flagged = false;

	while (*wc_bufp && !max_lines_flagged) {
		wchar_t wc = *wc_bufp;

		switch (wc) {
		case BLINK:
			case_blink(pwin, &booleans.is_blink);
			break;
		case BOLD:
			case_bold(pwin, &booleans.is_bold);
			break;
		case COLOR:
			case_color(pwin, &booleans.is_color, &wc_bufp);
			break;
		case NORMAL:
			booleans.reset();
			(void) wattrset(pwin, A_NORMAL);
			break;
		case REVERSE:
			case_reverse(pwin, &booleans.is_reverse);
			break;
		case UNDERLINE:
			case_underline(pwin, &booleans.is_underline);
			break;
		default:
		{
			const wchar_t	*wcp = nullptr;
			ptrdiff_t	 diff = 0;

			if (wc == L' ' && (wcp = wcschr(wc_bufp + 1, L' ')) !=
			    nullptr) {
				static wchar_t		str[4096] = { L'\0' };
				static const ptrdiff_t	ARSZ = static_cast
				    <ptrdiff_t>(ARRAY_SIZE(str));

				if ((diff = (wcp - wc_bufp)) > ARSZ - 1)
					diff = ARSZ - 1;
				sw_assert(diff > 0);
#if defined(UNIX)
				(void) wcsncpy(str, wc_bufp, diff);
				str[diff] = L'\0';
#elif defined(WIN32)
				switch (wcsncpy_s(str, ARSZ, wc_bufp, diff)) {
				case 0:
					/* copying ok */
					break;
				case STRUNCATE:
					err_log(0, "%s: wcsncpy_s: truncated",
					    __func__);
					break;
				default:
					sw_assert_not_reached();
					break;
				}
#endif
				if ((diff = xwcswidth(wcspbrk(str,
				    L"\035\002\003\017\026\037") ?
				    squeeze_text_deco_wide(str) : str, 2)) < 0)
					diff = 0;
			}
			struct case_default_context def_ctx(pwin, wc,
			    !wcscmp(wc_bufp + 1, L""), indent, max_lines, diff);
			case_default(&def_ctx, rep_count, &lines_count,
			    &insert_count);
			break;
		} /* case default */
		} /* switch block */

		++ wc_bufp;

		if (is_scrollok(pwin) && max_lines > 0)
			max_lines_flagged = lines_count >= max_lines;
	}

	free(wc_buf);
	wc_buf = nullptr;

	if (!atomic_load_bool(&g_redrawing_window) &&
	    !atomic_load_bool(&g_resizing_term)) {
		update_panels();
		(void) doupdate();
	}

	(void) wattrset(pwin, A_NORMAL);

	mutex_unlock(&g_puts_mutex);
}

void
set_timestamp(char *dest, size_t destsize,
	      const struct irc_message_compo *compo)
{
	immutable_cp_t	b1  = Theme("statusbar_leftBracket");
	immutable_cp_t	b2  = Theme("statusbar_rightBracket");
	immutable_cp_t	sep = Theme("notice_sep");
	int		ret;

	ret = snprintf(dest, destsize, "%s%02d%s%02d%s%02d%s",
	    b1, compo->hour, sep, compo->minute, sep, compo->second, b2);

	if (ret < 0 || static_cast<size_t>(ret) >= destsize)
		debug("%s: snprintf: error", __func__);
}

static void
vprinttext_mutex_init_doit()
{
#if defined(UNIX)
	static pthread_once_t init_done = PTHREAD_ONCE_INIT;

	if ((errno = pthread_once(&init_done, vprinttext_mutex_init)) != 0)
		err_sys("%s: pthread_once", __func__);
#elif defined(WIN32)
	static init_once_t init_done = ONCE_INITIALIZER;

	if ((errno = init_once(&init_done, vprinttext_mutex_init)) != 0)
		err_sys("%s: init_once", __func__);
#endif
}

/**
 * Variable argument list version of Swirc messenger
 *
 * @param ctx Context structure
 * @param fmt Format
 * @param ap  va_list object
 * @return Void
 */
void
vprinttext(PPRINTTEXT_CONTEXT ctx, CSTRING fmt, va_list ap)
{
	STRING			 fmt_copy;
	const int		 tbszp1 = textBuf_size(ctx->window->buf) + 1;
	struct integer_context	 intctx("textbuffer_size_absolute", 350, 4700,
				     1000);
	struct message_components pout;

	vprinttext_mutex_init_doit();
	mutex_lock(&vprinttext_mutex);
	fmt_copy = strdup_vprintf(fmt, ap);
	pout.get_msg(fmt_copy, ctx->spec_type, ctx->include_ts,
	    (ctx->has_server_time ? ctx->server_time : nullptr));
	free(fmt_copy);

	if (tbszp1 > config_integer(&intctx)) /* Buffer full... */
		textBuf_pop_head(__func__, ctx->window->buf);

	textBuf_emplace_back(__func__, ctx->window->buf, pout.text,
	    pout.indent);

	const bool shouldOutData = !(ctx->window->scroll_mode);

	if (shouldOutData) {
		if (atomic_load_bool(&g_on_air) &&
		    is_irc_channel(ctx->window->label) &&
		    !g_icb_mode &&
		    (!ctx->window->received_names ||
		     ctx->window->nicklist.pan == nullptr))
			/* no */;
		else {
			printtext_puts(panel_window(ctx->window->pan),
			    pout.text, pout.indent, -1, nullptr);
		}
	}

	if (ctx->window->logging && !ctx->window->is_logwin) {
		STRING logpath;

		if ((logpath = log_get_path(g_server_hostname,
		    ctx->window->label)) != nullptr) {
			log_msg(logpath, pout.text);
			free(logpath);
		}
	}

	mutex_unlock(&vprinttext_mutex);
}
