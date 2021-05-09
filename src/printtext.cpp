/* Prints and handles text
   Copyright (C) 2012-2021 Markus Uhlin. All rights reserved.

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

#ifdef UNIT_TESTING
#undef UNIT_TESTING
#include <setjmp.h>
#include <cmocka.h>
#define UNIT_TESTING 1
#endif

#include "assertAPI.h"
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
#include "strHand.h"
#include "strdup_printf.h"
#include "terminal.h"
#include "theme.h"

#define WADDCH(win, c)        ((void) waddch(win, c))
#define WATTR_OFF(win, attrs) ((void) wattr_off(win, attrs, NULL))
#define WATTR_ON(win, attrs)  ((void) wattr_on(win, attrs, NULL))
#define WCOLOR_SET(win, cpn)  ((void) wcolor_set(win, cpn, NULL))

#define STRLEN_CAST(string) strlen((char *) string)

/****************************************************************
*                                                               *
*  ------------------ Structure definitions ------------------  *
*                                                               *
****************************************************************/

struct message_components {
	char	*text;
	int	 indent;
};

struct text_decoration_bools {
	bool	 is_blink;
	bool	 is_bold;
	bool	 is_color;
	bool	 is_reverse;
	bool	 is_underline;

	text_decoration_bools();
};

text_decoration_bools::text_decoration_bools()
{
	this->is_blink     = false;
	this->is_bold      = false;
	this->is_color     = false;
	this->is_reverse   = false;
	this->is_underline = false;
}

struct case_default_context {
	WINDOW		*win;
	wchar_t		 wc;
	bool		 nextchar_empty;
	int		 indent;
	int		 max_lines;
	ptrdiff_t	 diff;

	case_default_context();
	case_default_context(WINDOW *, wchar_t, bool, int, int, ptrdiff_t);
};

case_default_context::case_default_context()
{
	this->win            = NULL;
	this->wc             = 0L;
	this->nextchar_empty = false;
	this->indent         = 0;
	this->max_lines      = 0;
	this->diff           = 0;
}

case_default_context::case_default_context(
	WINDOW		*win,
	wchar_t		 wc,
	bool		 nextchar_empty,
	int		 indent,
	int		 max_lines,
	ptrdiff_t	 diff)
{
	this->win            = win;
	this->wc             = wc;
	this->nextchar_empty = nextchar_empty;
	this->indent         = indent;
	this->max_lines      = max_lines;
	this->diff           = diff;
}

/****************************************************************
*                                                               *
*  -------------- Objects with external linkage --------------  *
*                                                               *
****************************************************************/

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
static pthread_once_t	vprinttext_init_done = PTHREAD_ONCE_INIT;
static pthread_once_t	puts_init_done = PTHREAD_ONCE_INIT;
static pthread_mutex_t	vprinttext_mutex;
#elif defined(WIN32)
static init_once_t	vprinttext_init_done = ONCE_INITIALIZER;
static init_once_t	puts_init_done = ONCE_INITIALIZER;
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

static void
addmbs(WINDOW *win, const unsigned char *mbs)
{
	chtype c = '\0';
	const unsigned char *p = mbs;

	while ((c = *p++) != '\0')
		WADDCH(win, c);
}

static void
append_newline(wchar_t **wc_buf)
{
	const size_t newsize = size_product(wcslen(*wc_buf) + sizeof "\n",
	    sizeof(wchar_t));

	*wc_buf = static_cast<wchar_t *>(xrealloc(*wc_buf, newsize));

	if ((errno = sw_wcscat(*wc_buf, L"\n", newsize)) != 0)
		err_sys("printtext: append_newline");
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
	if (! (*is_blink)) {
		//WATTR_ON(win, A_BLINK);
		*is_blink = true;
	} else {
		//WATTR_OFF(win, A_BLINK);
		*is_blink = false;
	}
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
	if (! (*is_bold)) {
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
 * @return The result
 */
static unsigned char *
convert_wc(wchar_t wc)
{
	const size_t	 size = MB_LEN_MAX + 1;
	mbstate_t	 ps;
	size_t		 bytes_written;
	unsigned char	*mbs = static_cast<unsigned char *>(xcalloc(size, 1));

	BZERO(&ps, sizeof(mbstate_t));

#ifdef HAVE_BCI
	if ((errno = wcrtomb_s(&bytes_written, reinterpret_cast<char *>(mbs),
	    size, wc, &ps)) != 0)
		err_log(errno, "printtext: convert_wc: wcrtomb_s");
#else
	if ((bytes_written = wcrtomb(reinterpret_cast<char *>(mbs), wc, &ps)) ==
	    g_conversion_failed)
		err_log(EILSEQ, "printtext: convert_wc: wcrtomb");
#endif

	(void) bytes_written; /* not used. provided for compatibility. */
	return mbs;
}

typedef enum {
	BUF_EOF,
	GO_ON,
	STOP_INTERPRETING
} cc_check_t;

/**
 * check for ^CN
 */
static cc_check_t
check_for_part1(wchar_t **bufp, char *fg)
{
	unsigned char *mbs;

	if (!*++(*bufp))
		return BUF_EOF;

	mbs = convert_wc(**bufp);

	if (STRLEN_CAST(mbs) != 1 || !sw_isdigit(*mbs)) {
		(*bufp)--;
		free(mbs);
		return STOP_INTERPRETING;
	}

	sw_snprintf(fg, 2, "%c", *mbs);
	free(mbs);
	return GO_ON;
}

/**
 * check for ^CNN or ^CN,
 */
static cc_check_t
check_for_part2(wchar_t **bufp, char *fg, bool *has_comma)
{
	unsigned char *mbs;

	if (!*++(*bufp))
		return BUF_EOF;

	mbs = convert_wc(**bufp);

	if (STRLEN_CAST(mbs) != 1 || (!sw_isdigit(*mbs) && *mbs != ',')) {
		(*bufp)--;
		free(mbs);
		return STOP_INTERPRETING;
	}

	if (sw_isdigit(*mbs))
		sw_snprintf(fg, 2, "%c", *mbs);
	else if (*mbs == ',')
		*has_comma = true;
	else
		sw_assert_not_reached();

	free(mbs);
	return GO_ON;
}

/**
 * check for ^CNN, or ^CN,N
 */
static cc_check_t
check_for_part3(wchar_t **bufp, bool *has_comma, bool fg_complete, char *bg)
{
	unsigned char *mbs;

	if (!*++(*bufp))
		return BUF_EOF;

	mbs = convert_wc(**bufp);

	if (STRLEN_CAST(mbs) != 1 || (*mbs != ',' && !sw_isdigit(*mbs))) {
		(*bufp)--;
		free(mbs);
		return STOP_INTERPRETING;
	} else if (*mbs == ',' && *has_comma) {
		(*bufp)--;
		free(mbs);
		return STOP_INTERPRETING;
	} else if (*mbs != ',' && fg_complete) {
		(*bufp)--;
		free(mbs);
		return STOP_INTERPRETING;
	}

	if (*mbs == ',')
		*has_comma = true;
	else if (sw_isdigit(*mbs))
		sw_snprintf(bg, 2, "%c", *mbs);
	else
		sw_assert_not_reached();

	free(mbs);
	return GO_ON;
}

/**
 * check for ^CNN,N or ^CN,NN
 */
static cc_check_t
check_for_part4(wchar_t **bufp, bool got_digit_bg, char *bg)
{
	unsigned char *mbs;

	if (!*++(*bufp))
		return BUF_EOF;

	mbs = convert_wc(**bufp);

	if (STRLEN_CAST(mbs) != 1 || !sw_isdigit(*mbs)) {
		(*bufp)--;
		free(mbs);
		return STOP_INTERPRETING;
	} else if (got_digit_bg) {
		sw_snprintf(++bg, 2, "%c", *mbs);
		free(mbs);
		return STOP_INTERPRETING;
	}

	sw_snprintf(bg, 2, "%c", *mbs);
	free(mbs);
	return GO_ON;
}

/**
 * check for ^CNN,NN
 */
static cc_check_t
check_for_part5(wchar_t **bufp, char *bg)
{
	unsigned char *mbs;

	if (!*++(*bufp))
		return BUF_EOF;

	mbs = convert_wc(**bufp);

	if (STRLEN_CAST(mbs) != 1 || !sw_isdigit(*mbs)) {
		(*bufp)--;
		free(mbs);
		return STOP_INTERPRETING;
	}

	sw_snprintf(bg, 2, "%c", *mbs);
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
	const short int num_colorMap_entries =
	    static_cast<short int>(COLORS >= 256
				   ? ARRAY_SIZE(ptext_colorMap)
				   : 16);
	short int fg, bg, resolved_pair;

	sw_assert(num1 >= 0);

	fg = ptext_colorMap[num1 % num_colorMap_entries].color;
	bg = (num2 < 0 ? -1 : ptext_colorMap[num2 % num_colorMap_entries].color);

	if (COLORS >= 16 && can_change_color()) {
		map_color(&fg, num1, num_colorMap_entries, &attr);
		map_color(&bg, num2, num_colorMap_entries, &attr);
	}

	if (attr != A_NORMAL)
		attr = ptext_colorMap[num1 % num_colorMap_entries].at;

	if ((resolved_pair = color_pair_find(fg, bg)) == -1) {
		WCOLOR_SET(win, 0);
		*is_color = false;
		return;
	}

	(void) wattr_set(win, attr, resolved_pair, NULL);
	*is_color = true;
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
	bool has_comma = false;
	char bg[10] = { 0 };
	char fg[10] = { 0 };
	short int num1 = -1;
	short int num2 = -1;
	struct integer_context intctx("term_background", 0, 15, 1);

	if (*is_color) {
		WCOLOR_SET(win, 0);
		*is_color = false;
	}

/***************************************************
 *
 * check for ^CN
 *
 ***************************************************/
	switch (check_for_part1(bufp, &fg[0])) {
	case BUF_EOF:
	case STOP_INTERPRETING:
		return;
	case GO_ON:
	default:
		break;
	}

/***************************************************
 *
 * check for ^CNN or ^CN,
 *
 ***************************************************/
	switch (check_for_part2(bufp, &fg[1], &has_comma)) {
	case BUF_EOF:
		return;
	case STOP_INTERPRETING:
		goto out;
	case GO_ON:
	default:
		break;
	}

/***************************************************
 *
 * check for ^CNN, or ^CN,N
 *
 ***************************************************/
	switch (check_for_part3(bufp, &has_comma, fg[1] != '\0', &bg[0])) {
	case BUF_EOF:
		return;
	case STOP_INTERPRETING:
		goto out;
	case GO_ON:
	default:
		break;
	}

/***************************************************
 *
 * check for ^CNN,N or ^CN,NN
 *
 ***************************************************/
	switch (check_for_part4(bufp, bg[0] != '\0', &bg[0])) {
	case BUF_EOF:
		return;
	case STOP_INTERPRETING:
		goto out;
	case GO_ON:
	default:
		break;
	}

/***************************************************
 *
 * check for ^CNN,NN
 *
 ***************************************************/
	switch (check_for_part5(bufp, &bg[1])) {
	case BUF_EOF:
		return;
	case STOP_INTERPRETING:
		goto out;
	case GO_ON:
	default:
		break;
	}

  out:
	num1 = static_cast<short int>(atoi(fg));

	if (!isEmpty(bg)) {
		num2 = static_cast<short int>(atoi(bg));
	} else if (isEmpty(bg) && theme_bool("term_use_default_colors", true)) {
		num2 = -1;
	} else {
		num2 = static_cast<short int>(theme_integer(&intctx));
	}

	printtext_set_color(win, is_color, num1, num2);

	if (has_comma && !(bg[0]))
		(*bufp)--;
}

/**
 * Do indent
 */
static void
do_indent(WINDOW *win, const int indent, int *insert_count)
{
	attr_t		attrs = 0;
	const chtype	blank = ' ';
	int		counter = 0;

	/* turn off all attributes during indentation */
	attrs = win->_attrs;
	(void) wattrset(win, A_NORMAL);

	while (counter++ != indent) {
		WADDCH(win, blank);
		(*insert_count)++;
	}

	/* restore attributes after indenting */
	(void) wattrset(win, attrs);
}

/**
 * Start on a new row?
 */
static bool
start_on_a_new_row(const ptrdiff_t sum, const WINDOW *win)
{
	return (sum < win->_maxx ? false : true);
}

/**
 * Handles switch default in printtext_puts()
 *
 * @param[in]     ctx          Context structure
 * @param[in,out] rep_count    "Represent" count
 * @param[out]    line_count   Line count
 * @param[out]    insert_count Insert count
 * @return Void
 */
static void
case_default(const struct case_default_context *ctx, int *rep_count,
	     int *line_count, int *insert_count)
{
	unsigned char *mbs;

	if (!iswprint(ctx->wc) && ctx->wc != L'\n')
		return;

	mbs = convert_wc(ctx->wc);

	if (!is_scrollok(ctx->win)) {
		addmbs(ctx->win, mbs);
		free(mbs);
		return;
	}

	/* -------------------------------------------------- */

	const bool care_about_indent = (ctx->indent > 0);
	const bool care_about_max_lines = (ctx->max_lines > 0);

	if (ctx->wc == L'\n') {
		WADDCH(ctx->win, '\n');
		*insert_count = 0;
		if (rep_count != NULL)
			(*rep_count) ++;
		if (care_about_max_lines &&
		    !(++ (*line_count) < ctx->max_lines)) {
			free(mbs);
			return;
		}
		if (! (ctx->nextchar_empty) && care_about_indent)
			do_indent(ctx->win, ctx->indent, insert_count);
	} else if (!start_on_a_new_row((*insert_count) + ctx->diff + 1,
		    ctx->win)) {
		addmbs(ctx->win, mbs);
		(*insert_count) ++;
	} else {
		/*
		 * Start on a new row
		 */
		WADDCH(ctx->win, '\n');
		*insert_count = 0;
		if (rep_count != NULL)
			(*rep_count) ++;
		if (care_about_max_lines &&
		    !(++ (*line_count) < ctx->max_lines)) {
			free(mbs);
			return;
		}
		if (care_about_indent)
			do_indent(ctx->win, ctx->indent, insert_count);
		if (ctx->wc != L' ') {
			addmbs(ctx->win, mbs);
			(*insert_count) ++;
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
	if (! (*is_reverse)) {
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
	if (! (*is_underline)) {
		WATTR_ON(win, A_UNDERLINE);
		*is_underline = true;
	} else {
		WATTR_OFF(win, A_UNDERLINE);
		*is_underline = false;
	}
}

/**
 * Get multibyte string length
 */
static size_t
get_mb_strlen(const char *s)
{
	const size_t ERR_CASE1 = static_cast<size_t>(-1);
	const size_t ERR_CASE2 = static_cast<size_t>(-2);
	size_t idx = 0;
	size_t len = 0;

	while (true) {
		const size_t ret = mbrlen(&s[idx], MB_CUR_MAX, NULL);

		if (ret == ERR_CASE1 || ret == ERR_CASE2) {
			return strlen(s);
		} else if (ret == 0) {
			break;
		} else {
			idx += ret;
			len += 1;
		}
	}

	return len;
}

static void
set_indent(int *indent, const char *fmt, ...)
{
	va_list ap;
	char *str = NULL;

	va_start(ap, fmt);
	str = strdup_vprintf(fmt, ap);
	va_end(ap);

	*indent = static_cast<int>(get_mb_strlen(squeeze_text_deco(str)));
	free(str);
}

/**
 * Get message components
 *
 * @param unproc_msg Unprocessed message
 * @param spec_type  "Specifier"
 * @param include_ts Include timestamp?
 * @param srv_time   Server time
 * @return Message components
 */
static struct message_components *
get_processed_out_message(
	const char *unproc_msg,
	enum message_specifier_type spec_type,
	bool include_ts,
	const char *srv_time)
{
	struct message_components *pout =
	    static_cast<struct message_components *>(xcalloc(sizeof *pout, 1));

	pout->text = NULL;
	pout->indent = 0;

	if (include_ts) {
		char *ts = NULL;

		if (srv_time)
			ts = sw_strdup(srv_time);
		else
			ts = sw_strdup(current_time(Theme("time_format")));

		switch (spec_type) {
		case TYPE_SPEC1:
			pout->text = strdup_printf("%s %s %s", ts, THE_SPEC1,
			    unproc_msg);
			set_indent(& (pout->indent), "%s %s ", ts, THE_SPEC1);
			break;
		case TYPE_SPEC2:
			pout->text = strdup_printf("%s %s %s", ts, THE_SPEC2,
			    unproc_msg);
			set_indent(& (pout->indent), "%s %s ", ts, THE_SPEC2);
			break;
		case TYPE_SPEC3:
			pout->text = strdup_printf("%s %s %s", ts, THE_SPEC3,
			    unproc_msg);
			set_indent(& (pout->indent), "%s %s ", ts, THE_SPEC3);
			break;
		case TYPE_SPEC1_SPEC2:
			pout->text = strdup_printf("%s %s %s %s", ts, THE_SPEC1,
			    THE_SPEC2, unproc_msg);
			set_indent(& (pout->indent), "%s %s %s ", ts, THE_SPEC1,
			    THE_SPEC2);
			break;
		case TYPE_SPEC1_FAILURE:
			pout->text = strdup_printf("%s %s %s %s", ts, THE_SPEC1,
			    GFX_FAILURE, unproc_msg);
			set_indent(& (pout->indent), "%s %s %s ", ts, THE_SPEC1,
			    GFX_FAILURE);
			break;
		case TYPE_SPEC1_SUCCESS:
			pout->text = strdup_printf("%s %s %s %s", ts, THE_SPEC1,
			    GFX_SUCCESS, unproc_msg);
			set_indent(& (pout->indent), "%s %s %s ", ts, THE_SPEC1,
			    GFX_SUCCESS);
			break;
		case TYPE_SPEC1_WARN:
			pout->text = strdup_printf("%s %s %s %s", ts, THE_SPEC1,
			    GFX_WARN, unproc_msg);
			set_indent(& (pout->indent), "%s %s %s ", ts, THE_SPEC1,
			    GFX_WARN);
			break;
		case TYPE_SPEC_NONE:
		default:
			pout->text = strdup_printf("%s %s", ts, unproc_msg);
			set_indent(& (pout->indent), "%s ", ts);
			break;
		}

		free(ts);
	} else if (!include_ts) {
		/*
		 * the same but no timestamp
		 */

		switch (spec_type) {
		case TYPE_SPEC1:
			pout->text = strdup_printf("%s %s", THE_SPEC1,
			    unproc_msg);
			set_indent(& (pout->indent), "%s ", THE_SPEC1);
			break;
		case TYPE_SPEC2:
			pout->text = strdup_printf("%s %s", THE_SPEC2,
			    unproc_msg);
			set_indent(& (pout->indent), "%s ", THE_SPEC2);
			break;
		case TYPE_SPEC3:
			pout->text = strdup_printf("%s %s", THE_SPEC3,
			    unproc_msg);
			set_indent(& (pout->indent), "%s ", THE_SPEC3);
			break;
		case TYPE_SPEC1_SPEC2:
			pout->text = strdup_printf("%s %s %s", THE_SPEC1,
			    THE_SPEC2, unproc_msg);
			set_indent(& (pout->indent), "%s %s ", THE_SPEC1,
			    THE_SPEC2);
			break;
		case TYPE_SPEC1_FAILURE:
			pout->text = strdup_printf("%s %s %s", THE_SPEC1,
			    GFX_FAILURE, unproc_msg);
			set_indent(& (pout->indent), "%s %s ", THE_SPEC1,
			    GFX_FAILURE);
			break;
		case TYPE_SPEC1_SUCCESS:
			pout->text = strdup_printf("%s %s %s", THE_SPEC1,
			    GFX_SUCCESS, unproc_msg);
			set_indent(& (pout->indent), "%s %s ", THE_SPEC1,
			    GFX_SUCCESS);
			break;
		case TYPE_SPEC1_WARN:
			pout->text = strdup_printf("%s %s %s", THE_SPEC1,
			    GFX_WARN, unproc_msg);
			set_indent(& (pout->indent), "%s %s ", THE_SPEC1,
			    GFX_WARN);
			break;
		case TYPE_SPEC_NONE:
		default:
			pout->text = sw_strdup(unproc_msg);
			pout->indent = 0;
			break;
		}
	} else {
		sw_assert_not_reached();
	}

	sw_assert(pout->text != NULL);

	if (g_no_colors)
		pout->text = squeeze_text_deco(pout->text);
	return pout;
}

/**
 * Helper function for squeeze_text_deco().
 */
static SW_INLINE void
handle_foo_situation(char **buffer, long int *i, long int *j,
		     const char *reject)
{
	if (!(*buffer)[*i]) {
		return;
	} else if ((*buffer)[*i] == COLOR) {
		(*i)--;
	} else if (strchr(reject, (*buffer)[*i]) == NULL) {
		(*buffer)[(*j)++] = (*buffer)[*i];
	}
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
/*lint -sem(windows_convert_to_utf8, r_null) */
static wchar_t *
windows_convert_to_utf8(const char *buf)
{
	const int size = static_cast<int>(strlen(buf) + 1);
	wchar_t *out = static_cast<wchar_t *>(xcalloc(size, sizeof *out));

	if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, buf, -1, out,
	    size) > 0)
		return out;
	free(out);
	return NULL;
}
#endif

/**
 * Attempt convert multibyte character string to wide-character string
 * by using a specific codeset. The storage is dynamically allocated.
 *
 * @param buf     Buffer to convert
 * @param codeset Codeset to use
 * @return A wide-character string, or NULL on error.
 */
/*lint -sem(try_convert_buf_with_cs, r_null) */
static wchar_t *
try_convert_buf_with_cs(const char *buf, const char *codeset)
{
	struct locale_info *li = get_locale_info(LC_CTYPE);
	char *original_locale = NULL;
	char *tmp_locale = NULL;
	wchar_t *out = NULL;

	try {
		size_t bytes_convert = 0;

		if (buf == NULL || codeset == NULL) {
			throw std::runtime_error("invalid arguments");
		} else if (li->lang_and_territory == NULL ||
			   li->codeset == NULL) {
			throw std::runtime_error("failed to get locale "
			    "information");
		}

		const size_t size = strlen(buf) + 1;

		original_locale = strdup_printf("%s.%s", li->lang_and_territory,
		    li->codeset);
		tmp_locale = strdup_printf("%s.%s", li->lang_and_territory,
		    codeset);
		out = static_cast<wchar_t *>(xcalloc(size, sizeof *out));

		if (setlocale(LC_CTYPE, tmp_locale) == NULL ||
		    (bytes_convert = xmbstowcs(out, buf, size - 1)) ==
		    g_conversion_failed) {
			if (setlocale(LC_CTYPE, original_locale) == NULL) {
				err_log(EPERM, "try_convert_buf_with_cs: "
				    "cannot restore original locale (%s)",
				    original_locale);
			}

			throw std::runtime_error("conversion failed");
		}

		if (bytes_convert == (size - 1))
			out[size - 1] = 0L;
		if (setlocale(LC_CTYPE, original_locale) == NULL) {
			err_log(EPERM, "try_convert_buf_with_cs: "
			    "cannot restore original locale (%s)",
			    original_locale);
		}

		free_locale_info(li);
		free(original_locale);
		free(tmp_locale);
		return out;
	} catch (std::runtime_error &e) {
		err_log(0, "try_convert_buf_with_cs: %s", e.what());
		free_locale_info(li);
		free(original_locale);
		free(tmp_locale);
		free(out);
		/* FALLTHROUGH */
	}

	return NULL;
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
	const char	*ar[] = {
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
	size_t		 size = 0;
	wchar_t		*out = NULL;

#if WIN32
	if ((out = windows_convert_to_utf8(*in_buf)) != NULL)
		return out;
#endif

	for (const char **ar_p = &ar[0]; ar_p < &ar[ARRAY_SIZE(ar)]; ar_p++) {
		if ((out = try_convert_buf_with_cs(*in_buf, *ar_p)) != NULL) {
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
		if (mbsrtowcs(&out[wcslen(out)], in_buf,
		    (size - wcslen(out)) - 1, &ps) == g_conversion_failed &&
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
		err_log(EILSEQ, "In perform_convert_buffer: characters lost");
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

	while ((wcp = wcspbrk(wc_buf, set)) != NULL)
		*wcp = L' ';
}

/**
 * Reset text-decoration bools
 *
 * @param booleans Context structure
 * @return Void
 */
static void
text_decoration_bools_reset(struct text_decoration_bools *booleans)
{
	booleans->is_blink     = false;
	booleans->is_bold      = false;
	booleans->is_color     = false;
	booleans->is_reverse   = false;
	booleans->is_underline = false;
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

	BZERO(ctx->server_time, ARRAY_SIZE(ctx->server_time));
	ctx->has_server_time = false;

	return ctx;
}

void
printtext_context_destroy(PPRINTTEXT_CONTEXT ctx)
{
	free(ctx);
}

void
printtext_context_init(
	PPRINTTEXT_CONTEXT ctx,
	PIRC_WINDOW window,
	enum message_specifier_type spec_type,
	bool include_ts)
{
	if (ctx == NULL) {
		err_log(EINVAL, "printtext_context_init");
		return;
	}

	ctx->window     = window;
	ctx->spec_type  = spec_type;
	ctx->include_ts = include_ts;

	BZERO(ctx->server_time, ARRAY_SIZE(ctx->server_time));
	ctx->has_server_time = false;
}

/**
 * Squeeze text-decoration from a buffer
 *
 * @param buffer Target buffer
 * @return The result
 */
char *
squeeze_text_deco(char *buffer)
{
	static const char reject[] =
	    TXT_BLINK
	    TXT_BOLD
	    TXT_NORMAL
	    TXT_REVERSE
	    TXT_UNDERLINE;
	long int	i, j;
	bool		has_comma;

	if (buffer == NULL)
		err_exit(EINVAL, "squeeze_text_deco");
	else if (strings_match(buffer, ""))
		return buffer;

	for (i = j = 0; buffer[i] != '\0'; i++) {
		switch (buffer[i]) {
		case COLOR:
		{
			/*
			 * check for ^CN
			 */
			if (!sw_isdigit(buffer[++i])) {
				handle_foo_situation(&buffer, &i, &j, reject);
				break;
			}

			/*
			 * check for ^CNN or ^CN,
			 */
			if (!sw_isdigit(buffer[++i]) && buffer[i] != ',') {
				handle_foo_situation(&buffer, &i, &j, reject);
				break;
			}

			has_comma = buffer[i++] == ',';

			/*
			 * check for ^CNN, or ^CN,N
			 */
			if (!has_comma && buffer[i] == ',') {
				has_comma = true;
			} else if (has_comma && sw_isdigit(buffer[i])) {
				/* ^CN,N */;
			} else if (has_comma && !sw_isdigit(buffer[i])) {
				i--;
				handle_foo_situation(&buffer, &i, &j, reject);
				break;
			} else {
				handle_foo_situation(&buffer, &i, &j, reject);
				break;
			}

			sw_assert(has_comma);

			/*
			 * check for ^CNN,N or ^CN,NN
			 */
			if (buffer[i] == ',') { /* ^CNN, */
				if (!sw_isdigit(buffer[++i])) {
					i--;
					handle_foo_situation(&buffer, &i, &j,
					    reject);
					break;
				}
			} else { /* ^CN,N */
				sw_assert(sw_isdigit(buffer[i]));
				if (sw_isdigit(buffer[++i])) /* we have ^CN,NN? */
					break;
				handle_foo_situation(&buffer, &i, &j, reject);
				break;
			}

			/*
			 * check for ^CNN,NN
			 */
			if (!sw_isdigit(buffer[++i])) {
				handle_foo_situation(&buffer, &i, &j, reject);
				break;
			}

			break;
		} /* case COLOR */
		default:
			if (strchr(reject, buffer[i]) == NULL)
				buffer[j++] = buffer[i];
			break;
		} /* switch block */
	}

	buffer[j] = '\0';
	return buffer;
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
print_and_free(const char *msg, char *cp)
{
	if (msg == NULL)
		err_exit(EINVAL, "print_and_free");

#ifdef UNIT_TESTING
	(void) fprintf(stderr, "** %s **\r\n", msg);
	free(cp);
	fail();
#else
	PRINTTEXT_CONTEXT ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_FAILURE, true);
	printtext(&ctx, "%s", msg);
	free(cp);
#endif
}

/**
 * Swirc messenger
 *
 * @param ctx Context structure
 * @param fmt Format control
 * @return Void
 */
void
printtext(PPRINTTEXT_CONTEXT ctx, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprinttext(ctx, fmt, ap);
	va_end(ap);
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
printtext_puts(WINDOW *pwin, const char *buf, int indent, int max_lines,
	       int *rep_count)
{
	int insert_count = 0;
	int line_count = 0;
	struct text_decoration_bools booleans; // calls constructor
	wchar_t *wc_buf = NULL;

#if defined(UNIX)
	if ((errno = pthread_once(&puts_init_done, puts_mutex_init)) != 0)
		err_sys("printtext_puts: pthread_once");
#elif defined(WIN32)
	if ((errno = init_once(&puts_init_done, puts_mutex_init)) != 0)
		err_sys("printtext_puts: init_once");
#endif

	if (rep_count)
		*rep_count = 0;
	if (pwin == NULL || buf == NULL)
		err_exit(EINVAL, "printtext_puts");
	else if (strings_match(buf, "") || term_is_too_small())
		return;

	mutex_lock(&g_puts_mutex);
	wc_buf = perform_convert_buffer(&buf);

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
			text_decoration_bools_reset(&booleans);
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
			ptrdiff_t	 diff = 0;
			wchar_t		*wcp = NULL;

			if (wc == L' ' && (wcp = wcschr(wc_bufp + 1, L' ')) !=
			    NULL)
				diff = (wcp - wc_bufp);
			struct case_default_context def_ctx(pwin, wc,
			    !wcscmp(wc_bufp + 1, L""), indent, max_lines, diff);
			case_default(&def_ctx, rep_count, &line_count,
			    &insert_count);
			break;
		} /* case default */
		} /* switch block */

		(void) wnoutrefresh(pwin);

		++ wc_bufp;

		if (is_scrollok(pwin) && max_lines > 0)
			max_lines_flagged = line_count >= max_lines;
	}

	free(wc_buf);
	wc_buf = NULL;

	(void) wattrset(pwin, A_NORMAL);

	update_panels();
	(void) doupdate();

	mutex_unlock(&g_puts_mutex);
}

#define B1 Theme("statusbar_leftBracket")
#define B2 Theme("statusbar_rightBracket")

#define SEP Theme("notice_sep")

void
set_timestamp(char *dest, size_t destsize,
	      const struct irc_message_compo *compo)
{
	const int ret = snprintf(dest, destsize, "%s%02d%s%02d%s%02d%s",
	    B1, compo->hour, SEP, compo->minute, SEP, compo->second, B2);

	if (ret < 0 || static_cast<size_t>(ret) >= destsize)
		debug("set_timestamp: snprintf: error");
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
vprinttext(PPRINTTEXT_CONTEXT ctx, const char *fmt, va_list ap)
{
	char *fmt_copy = NULL;
	const int tbszp1 = textBuf_size(ctx->window->buf) + 1;
	struct integer_context intctx("textbuffer_size_absolute", 350, 4700,
	    1000);
	struct message_components *pout = NULL;

#if defined(UNIX)
	if ((errno = pthread_once(&vprinttext_init_done, vprinttext_mutex_init))
	    != 0)
		err_sys("vprinttext: pthread_once");
#elif defined(WIN32)
	if ((errno = init_once(&vprinttext_init_done, vprinttext_mutex_init))
	    != 0)
		err_sys("vprinttext: init_once");
#endif

	mutex_lock(&vprinttext_mutex);
	fmt_copy = strdup_vprintf(fmt, ap);
	pout = get_processed_out_message(fmt_copy, ctx->spec_type,
	    ctx->include_ts, (ctx->has_server_time ? ctx->server_time : NULL));

	if (tbszp1 > config_integer(&intctx)) {
		/*
		 * Buffer full. Remove head...
		 */
		if ((errno = textBuf_remove(ctx->window->buf,
		    textBuf_head(ctx->window->buf))) != 0)
			err_sys("vprinttext: textBuf_remove");
	}

	if (textBuf_size(ctx->window->buf) == 0) {
		if ((errno = textBuf_ins_next(ctx->window->buf, NULL,
		    pout->text, pout->indent)) != 0)
			err_sys("vprinttext: textBuf_ins_next");
	} else {
		if ((errno = textBuf_ins_next(ctx->window->buf,
		    textBuf_tail(ctx->window->buf), pout->text,
		    pout->indent)) != 0)
			err_sys("vprinttext: textBuf_ins_next");
	}

	const bool shouldOutData = !(ctx->window->scroll_mode);

	if (shouldOutData) {
		if (g_on_air &&
		    is_irc_channel(ctx->window->label) &&
		    !g_icb_mode &&
		    (!ctx->window->received_names ||
		     ctx->window->nicklist.pan == NULL))
			/* no */;
		else {
			printtext_puts(panel_window(ctx->window->pan),
			    pout->text, pout->indent, -1, NULL);
		}
	}

	if (ctx->window->logging) {
		char *logpath = log_get_path(g_server_hostname,
		    ctx->window->label);

		if (logpath) {
			log_msg(logpath, pout->text);
			free(logpath);
		}
	}

	free(fmt_copy);
	free(pout->text);
	free(pout);
	mutex_unlock(&vprinttext_mutex);
}
