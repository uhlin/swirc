/* Prints and handles text
   Copyright (C) 2012-2016 Markus Uhlin. All rights reserved.

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

   THIS SOFTWARE IS PROVIDED THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

#include "common.h"

#include <locale.h>
#include <wctype.h>

#include "assertAPI.h"
#include "config.h"
#if defined(WIN32) && defined(PDC_EXP_EXTRAS)
#include "curses-funcs.h" /* is_scrollok() */
#endif
#include "cursesInit.h"
#include "dataClassify.h"
#include "errHand.h"
#include "libUtils.h"
#include "main.h"
#include "printtext.h"
#include "strHand.h"
#include "strdup_printf.h"
#include "terminal.h"
#include "theme.h"

#define WADDCH(win, c)        ((void) waddch(win, c))
#define WATTR_OFF(win, attrs) ((void) wattr_off(win, attrs, 0))
#define WATTR_ON(win, attrs)  ((void) wattr_on(win, attrs, 0))
#define WCOLOR_SET(win, cpn)  ((void) wcolor_set(win, cpn, 0))

/* Structure definitions
   ===================== */

struct message_components {
    char *text;
    int   indent;
};

struct text_decoration_bools {
    bool is_blink;
    bool is_bold;
    bool is_color;
    bool is_reverse;
    bool is_underline;
};

struct case_default_context {
    WINDOW    *win;
    wchar_t    wc;
    int        nextchar_empty;
    int        indent;
    int        max_lines;
    ptrdiff_t  diff;
};

/* Objects with external linkage
   ============================= */

#if defined(UNIX)
pthread_mutex_t g_puts_mutex;
#elif defined(WIN32)
HANDLE g_puts_mutex;
#endif

/* Objects with internal linkage
   ============================= */

#if defined(UNIX)
static pthread_once_t  vprinttext_init_done = PTHREAD_ONCE_INIT;
static pthread_once_t  puts_init_done       = PTHREAD_ONCE_INIT;
static pthread_mutex_t vprinttext_mutex;
#elif defined(WIN32)
static init_once_t vprinttext_init_done = ONCE_INITIALIZER;
static init_once_t puts_init_done       = ONCE_INITIALIZER;
static HANDLE      vprinttext_mutex;
#endif

static struct ptext_colorMap_tag {
    short int color;
#if defined(UNIX)
    attr_t at;
#elif defined(WIN32)
    chtype at;
#endif
} ptext_colorMap[] = {
    { COLOR_WHITE,   A_BOLD },
    { COLOR_BLACK,   A_DIM  },
    { COLOR_BLUE,    A_DIM  },
    { COLOR_GREEN,   A_DIM  },
    { COLOR_RED,     A_BOLD },
    { COLOR_RED,     A_DIM  },
    { COLOR_MAGENTA, A_DIM  },
    { COLOR_YELLOW,  A_DIM  },
    { COLOR_YELLOW,  A_BOLD },
    { COLOR_GREEN,   A_BOLD },
    { COLOR_CYAN,    A_DIM  },
    { COLOR_CYAN,    A_BOLD },
    { COLOR_BLUE,    A_BOLD },
    { COLOR_MAGENTA, A_BOLD },
    { COLOR_BLACK,   A_BOLD },
    { COLOR_WHITE,   A_DIM  },
};

/* Static function declarations
   ============================ */

static SW_INLINE void
handle_foo_situation(char **buffer, long int *i, long int *j, const char *reject);

static struct message_components *
get_processed_out_message(const char *unproc_msg, enum message_specifier_type, bool include_ts);

/*lint -sem(try_convert_buf_with_cs, r_null) */
/*lint -sem(windows_convert_to_utf8, r_null) */

static unsigned char	*convert_wc                  (wchar_t);
static void		 case_blink                  (WINDOW *, bool *is_blink);
static void		 case_bold                   (WINDOW *, bool *is_bold);
static void		 case_color                  (WINDOW *, bool *is_color, wchar_t **bufp);
static void		 case_default                (struct case_default_context *,
						      int *rep_count, int *line_count, int *insert_count);
static void		 case_reverse                (WINDOW *, bool *is_reverse);
static void		 case_underline              (WINDOW *, bool *is_underline);
static void		 printtext_set_color         (WINDOW *, bool *is_color, short int num1, short int num2);
static void		 puts_mutex_init             (void);
static void		 text_decoration_bools_reset (struct text_decoration_bools *);
static void		 vprinttext_mutex_init       (void);
static wchar_t		*perform_convert_buffer      (const char **in_buf);
static wchar_t		*try_convert_buf_with_cs     (const char *buf, const char *codeset) PTR_ARGS_NONNULL;
#if WIN32
static wchar_t		*windows_convert_to_utf8     (const char *buf);
#endif

void
vprinttext(struct printtext_context *ctx, const char *fmt, va_list ap)
{
    int error;
    char *fmt_copy;
    struct message_components *pout;
    struct integer_unparse_context unparse_ctx = {
	.setting_name     = "textbuffer_size_absolute",
	.fallback_default = 1000,
	.lo_limit         = 350,
	.hi_limit         = 4700,
    };
    const int tbszp1 = textBuf_size(ctx->window->buf) + 1;

#if defined(UNIX)
    if ((error = pthread_once(&vprinttext_init_done, vprinttext_mutex_init)) != 0) {
	err_exit(error, "pthread_once error");
    }
#elif defined(WIN32)
    if ((error = init_once(&vprinttext_init_done, vprinttext_mutex_init)) != 0) {
	err_exit(error, "init_once error");
    }
#endif

    mutex_lock(&vprinttext_mutex);

    fmt_copy = Strdup_vprintf(fmt, ap);
    pout     = get_processed_out_message(fmt_copy, ctx->spec_type, ctx->include_ts);

    if (tbszp1 > config_integer_unparse(&unparse_ctx)) {
	/* Buffer full. Remove head... */

	if ((error = textBuf_remove( ctx->window->buf, textBuf_head(ctx->window->buf) )) != 0) {
	    err_exit(error, "textBuf_remove error");
	}
    }

    if (textBuf_size(ctx->window->buf) == 0) {
	if ((error = textBuf_ins_next(ctx->window->buf, NULL, pout->text, pout->indent)) != 0) {
	    err_exit(error, "textBuf_ins_next error");
	}
    } else {
	if ((error = textBuf_ins_next(
		 ctx->window->buf, textBuf_tail(ctx->window->buf), pout->text, pout->indent)) != 0) {
	    err_exit(error, "textBuf_ins_next error");
	}
    }

    if (! (ctx->window->scroll_mode))
	printtext_puts(panel_window(ctx->window->pan), pout->text, pout->indent, -1, NULL);

    free_not_null(fmt_copy);
    free_not_null(pout->text);
    free_not_null(pout);

    mutex_unlock(&vprinttext_mutex);
}

static void
vprinttext_mutex_init(void)
{
    mutex_new(&vprinttext_mutex);
}

static struct message_components *
get_processed_out_message(const char *unproc_msg,
			  enum message_specifier_type spec_type,
			  bool include_ts)
{
#define STRLEN_SQUEEZE(string) ((int) strlen(squeeze_text_deco(string)))
    struct message_components *pout = xcalloc(sizeof *pout, 1);
    char *tmp = NULL;

    pout->text = NULL;
    pout->indent = 0;

    if (include_ts) {
	char *ts = sw_strdup( current_time(Theme("time_format")) );

	switch (spec_type) {
	case TYPE_SPEC1:
	    pout->text   = Strdup_printf("%s %s %s", ts, THE_SPEC1, unproc_msg);
	    tmp          = Strdup_printf("%s %s ", ts, THE_SPEC1);
	    pout->indent = STRLEN_SQUEEZE(tmp);
	    break;
	case TYPE_SPEC2:
	    pout->text   = Strdup_printf("%s %s %s", ts, THE_SPEC2, unproc_msg);
	    tmp          = Strdup_printf("%s %s ", ts, THE_SPEC2);
	    pout->indent = STRLEN_SQUEEZE(tmp);
	    break;
	case TYPE_SPEC3:
	    pout->text   = Strdup_printf("%s %s %s", ts, THE_SPEC3, unproc_msg);
	    tmp          = Strdup_printf("%s %s ", ts, THE_SPEC3);
	    pout->indent = STRLEN_SQUEEZE(tmp);
	    break;
	case TYPE_SPEC1_SPEC2:
	    pout->text   = Strdup_printf("%s %s %s %s", ts, THE_SPEC1, THE_SPEC2, unproc_msg);
	    tmp          = Strdup_printf("%s %s %s ", ts, THE_SPEC1, THE_SPEC2);
	    pout->indent = STRLEN_SQUEEZE(tmp);
	    break;
	case TYPE_SPEC1_FAILURE:
	    pout->text   = Strdup_printf("%s %s %s %s", ts, THE_SPEC1, GFX_FAILURE, unproc_msg);
	    tmp          = Strdup_printf("%s %s %s ", ts, THE_SPEC1, GFX_FAILURE);
	    pout->indent = STRLEN_SQUEEZE(tmp);
	    break;
	case TYPE_SPEC1_SUCCESS:
	    pout->text   = Strdup_printf("%s %s %s %s", ts, THE_SPEC1, GFX_SUCCESS, unproc_msg);
	    tmp          = Strdup_printf("%s %s %s ", ts, THE_SPEC1, GFX_SUCCESS);
	    pout->indent = STRLEN_SQUEEZE(tmp);
	    break;
	case TYPE_SPEC1_WARN:
	    pout->text   = Strdup_printf("%s %s %s %s", ts, THE_SPEC1, GFX_WARN, unproc_msg);
	    tmp          = Strdup_printf("%s %s %s ", ts, THE_SPEC1, GFX_WARN);
	    pout->indent = STRLEN_SQUEEZE(tmp);
	    break;
	case TYPE_SPEC_NONE: default:
	    pout->text   = Strdup_printf("%s %s", ts, unproc_msg);
	    tmp          = Strdup_printf("%s ", ts);
	    pout->indent = STRLEN_SQUEEZE(tmp);
	    break;
	}

	free(ts);
    } else if (!include_ts) { /* the same but no timestamp */
	switch (spec_type) {
	case TYPE_SPEC1:
	    pout->text   = Strdup_printf("%s %s", THE_SPEC1, unproc_msg);
	    tmp          = Strdup_printf("%s ", THE_SPEC1);
	    pout->indent = STRLEN_SQUEEZE(tmp);
	    break;
	case TYPE_SPEC2:
	    pout->text   = Strdup_printf("%s %s", THE_SPEC2, unproc_msg);
	    tmp          = Strdup_printf("%s ", THE_SPEC2);
	    pout->indent = STRLEN_SQUEEZE(tmp);
	    break;
	case TYPE_SPEC3:
	    pout->text   = Strdup_printf("%s %s", THE_SPEC3, unproc_msg);
	    tmp          = Strdup_printf("%s ", THE_SPEC3);
	    pout->indent = STRLEN_SQUEEZE(tmp);
	    break;
	case TYPE_SPEC1_SPEC2:
	    pout->text   = Strdup_printf("%s %s %s", THE_SPEC1, THE_SPEC2, unproc_msg);
	    tmp          = Strdup_printf("%s %s ", THE_SPEC1, THE_SPEC2);
	    pout->indent = STRLEN_SQUEEZE(tmp);
	    break;
	case TYPE_SPEC1_FAILURE:
	    pout->text   = Strdup_printf("%s %s %s", THE_SPEC1, GFX_FAILURE, unproc_msg);
	    tmp          = Strdup_printf("%s %s ", THE_SPEC1, GFX_FAILURE);
	    pout->indent = STRLEN_SQUEEZE(tmp);
	    break;
	case TYPE_SPEC1_SUCCESS:
	    pout->text   = Strdup_printf("%s %s %s", THE_SPEC1, GFX_SUCCESS, unproc_msg);
	    tmp          = Strdup_printf("%s %s ", THE_SPEC1, GFX_SUCCESS);
	    pout->indent = STRLEN_SQUEEZE(tmp);
	    break;
	case TYPE_SPEC1_WARN:
	    pout->text   = Strdup_printf("%s %s %s", THE_SPEC1, GFX_WARN, unproc_msg);
	    tmp          = Strdup_printf("%s %s ", THE_SPEC1, GFX_WARN);
	    pout->indent = STRLEN_SQUEEZE(tmp);
	    break;
	case TYPE_SPEC_NONE: default:
	    pout->text   = sw_strdup(unproc_msg);
	    pout->indent = 0;
	    break;
	}
    } else {
	sw_assert_not_reached();
    }

    free_not_null(tmp);
    sw_assert(pout->text != NULL);

    if (g_no_colors) {
	pout->text = squeeze_text_deco(pout->text);
    }

    return (pout);
}

static SW_INLINE void
handle_foo_situation(char **buffer, long int *i, long int *j, const char *reject)
{
    if (!(*buffer)[*i]) {
	return;
    } else if ((*buffer)[*i] == COLOR) {
	(*i)--;
    } else if (strchr(reject, (*buffer)[*i]) == NULL) {
	(*buffer)[(*j)++] = (*buffer)[*i];
    } else {
	;
    }
}

char *
squeeze_text_deco(char *buffer)
{
    char *reject;
    long int i, j;

    if (buffer == NULL) {
	err_exit(EINVAL, "squeeze_text_deco error");
    } else if (*buffer == '\0') {
	return (buffer);
    } else {
	;
    }

    reject = Strdup_printf(
	"%c%c%c%c%c", BLINK, BOLD, NORMAL, REVERSE, UNDERLINE);

    for (i = j = 0; buffer[i] != '\0'; i++) {
	switch (buffer[i]) {
	case COLOR:
	{
	    /* check for ^CN */
	    if (!sw_isdigit(buffer[++i])) {
		handle_foo_situation(&buffer, &i, &j, reject);
		break;
	    }

	    /* check for ^CNN or ^CN, */
	    if (!sw_isdigit(buffer[++i]) && buffer[i] != ',') {
		handle_foo_situation(&buffer, &i, &j, reject);
		break;
	    }

	    /* check for ^CNN, or ^CN,N */
	    if (buffer[++i] != ',' && !sw_isdigit(buffer[i])) {
		handle_foo_situation(&buffer, &i, &j, reject);
		break;
	    }

	    /* check for ^CNN,N or ^CN,NN */
	    if (!sw_isdigit(buffer[++i])) {
		handle_foo_situation(&buffer, &i, &j, reject);
		break;
	    } else if (sw_isdigit(buffer[i - 1])) {	/* we have ^CN,NN? */
		break;					/* end switch */
	    } else {
		;
	    }

	    /* check for ^CNN,NN */
	    if (!sw_isdigit(buffer[++i])) {
		handle_foo_situation(&buffer, &i, &j, reject);
		break;
	    }

	    break;
	} /* case COLOR */
	default:
	    if (strchr(reject, buffer[i]) == NULL) {
		buffer[j++] = buffer[i];
	    }
	    break;
	} /* switch block */
    }

    free(reject);
    buffer[j] = '\0';

#ifndef NDEBUG
    {
	char *cp;

	reject = Strdup_printf(
	    "%c%c%c%c%c%c", BLINK, BOLD, COLOR, NORMAL, REVERSE, UNDERLINE);

	for (cp = &reject[0]; *cp != '\0'; cp++) {
	    if (strchr(buffer, *cp) != NULL) {
		err_quit("squeeze_text_deco debug error\n"
			 "still decoration in buf!");
	    }
	}

	free(reject);
    }
#endif

    return (buffer);
}

/* Description of parameters for printtext_puts()
   ==============================================

   [in] pwin
   Panel window where the output is to be displayed

   [in] buf
   A buffer that should contain the data to be written to 'pwin'

   [in] indent
   If >0 indent text with this number of blanks

   [in] max_lines
   If >0 write at most this number of lines

   [out] rep_count
   How many actual lines does this contribution represent in the output window?
   (Passing NULL is ok) */
void
printtext_puts(WINDOW *pwin, const char *buf, int indent, int max_lines, int *rep_count)
{
    struct text_decoration_bools booleans;
    wchar_t	*wc_buf, *wc_bufp;
    const bool	 pwin_scrollable = is_scrollok(pwin);
    int		 max_lines_flagged;
    int		 line_count	 = 0;
    int		 insert_count	 = 0;

#if defined(UNIX)
    if ((errno = pthread_once(&puts_init_done, puts_mutex_init)) != 0) {
	err_sys("pthread_once error");
    }
#elif defined(WIN32)
    if ((errno = init_once(&puts_init_done, puts_mutex_init)) != 0) {
	err_sys("init_once error");
    }
#endif

    if (rep_count) {
	*rep_count = 0;
    }

    if (!buf) {
	err_exit(EINVAL, "printtext_puts error");
    } else if (*buf == '\0') {
	return;
    } else {
	;
    }

    mutex_lock(&g_puts_mutex);
    text_decoration_bools_reset(&booleans);
    wc_buf = perform_convert_buffer(&buf);

    if (pwin_scrollable) {
	const size_t newsize = size_product(wcslen(wc_buf) + sizeof "\n", sizeof (wchar_t));

	wc_buf = xrealloc(wc_buf, newsize);

	if ((errno = sw_wcscat(wc_buf, L"\n", newsize)) != 0)
	    err_sys("printtext_puts error");
    }

    {
	wchar_t       *wcp;
	const wchar_t  set[] = L"\f\t\v";

	while (wcp = wcspbrk(wc_buf, set), wcp != NULL) {
	    *wcp = btowc(' ');
	}
    }

    for (wc_bufp = &wc_buf[0], max_lines_flagged = 0; *wc_bufp && !max_lines_flagged; wc_bufp++) {
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
	    term_set_attr(pwin, A_NORMAL);
	    break;
	case REVERSE:
	    case_reverse(pwin, &booleans.is_reverse);
	    break;
	case UNDERLINE:
	    case_underline(pwin, &booleans.is_underline);
	    break;
	default:
	{
	    wchar_t   *wcp;
	    ptrdiff_t  diff = 0;
	    struct case_default_context def_ctx;

	    if (wc == L' ' && (wcp = wcschr(wc_bufp + 1, L' ')) != NULL) {
		diff = wcp - wc_bufp;
	    }

	    def_ctx.win            = pwin;
	    def_ctx.wc             = wc;
	    def_ctx.nextchar_empty = !wcscmp(wc_bufp + 1, L"");
	    def_ctx.indent         = indent;
	    def_ctx.max_lines      = max_lines;
	    def_ctx.diff           = diff;

	    case_default(&def_ctx, rep_count, &line_count, &insert_count);
	    break;
	} /* case default */
	} /* switch block */
	if (pwin_scrollable && max_lines > 0) {
	    max_lines_flagged = line_count >= max_lines;
	}
    }

    free(wc_buf);
    term_set_attr(pwin, A_NORMAL);
    update_panels();
    doupdate();
    mutex_unlock(&g_puts_mutex);
}

static void
puts_mutex_init(void)
{
    mutex_new(&g_puts_mutex);
}

static void
text_decoration_bools_reset(struct text_decoration_bools *booleans)
{
    booleans->is_blink     = false;
    booleans->is_bold      = false;
    booleans->is_color     = false;
    booleans->is_reverse   = false;
    booleans->is_underline = false;
}

#if WIN32
static wchar_t *
windows_convert_to_utf8(const char *buf)
{
    const int sz = (int) (strlen(buf) + 1);
    wchar_t *out = xcalloc(sz, sizeof (wchar_t));

    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, buf, -1, out, sz) > 0)
	return out;
    free(out);
    return NULL;
}
#endif

static wchar_t *
try_convert_buf_with_cs(const char *buf, const char *codeset)
{
    struct locale_info	*li		 = get_locale_info(LC_CTYPE);
    char		*original_locale = NULL;
    char		*tmp_locale	 = NULL;
    const size_t	 sz		 = strlen(buf) + 1;
    wchar_t		*out		 = NULL;
    size_t		 bytes_convert	 = 0;
    const size_t	 CONVERT_FAILED	 = (size_t) -1;

    if (li->lang_and_territory == NULL || li->codeset == NULL)
	goto err;

    original_locale = Strdup_printf("%s.%s", li->lang_and_territory, li->codeset);
    tmp_locale      = Strdup_printf("%s.%s", li->lang_and_territory, codeset);
    out             = xcalloc(sz, sizeof (wchar_t));

    if (setlocale(LC_CTYPE, tmp_locale) == NULL ||
	(bytes_convert = mbstowcs(out, buf, sz - 1)) == CONVERT_FAILED) {
	if (setlocale(LC_CTYPE, original_locale) == NULL)
	    err_log(EPERM, "In try_convert_buf_with_cs: cannot restore original locale (%s)", original_locale);
	goto err;
    }

    if (bytes_convert == sz - 1)
	out[sz - 1] = 0;

    if (setlocale(LC_CTYPE, original_locale) == NULL)
	err_log(EPERM, "In try_convert_buf_with_cs: cannot restore original locale (%s)", original_locale);
    free_locale_info(li);
    free_not_null(original_locale);
    free_not_null(tmp_locale);
    return out;

  err:
    free_locale_info(li);
    free_not_null(original_locale);
    free_not_null(tmp_locale);
    free_not_null(out);
    return NULL;
}

static wchar_t *
perform_convert_buffer(const char **in_buf)
{
    const char *ar[] = {
#if defined(UNIX)
	"UTF-8",       "utf8",
	"ISO-8859-1",  "ISO8859-1",  "iso88591",
	"ISO-8859-15", "ISO8859-15", "iso885915",
#elif defined(WIN32)
	"65001", /* UTF-8 */
	"28591", /* ISO 8859-1 Latin 1 */
	"28605", /* ISO 8859-15 Latin 9 */
#endif
    };
    const size_t	 ar_sz		= ARRAY_SIZE(ar);
    wchar_t		*out		= NULL;
    size_t		 sz		= 0;
    mbstate_t		 ps;
    const size_t	 CONVERT_FAILED = (size_t) -1;

#if WIN32
    if ((out = windows_convert_to_utf8(*in_buf)) != NULL)
	return (out);
#endif

    for (const char **ar_p = &ar[0]; ar_p < &ar[ar_sz]; ar_p++) {
	if ((out = try_convert_buf_with_cs(*in_buf, *ar_p)) != NULL) /* success */
	    return (out);
    }

    /* fallback solution... */
    sz  = strlen(*in_buf) + 1;
    out = xcalloc(sz, sizeof (wchar_t));

    BZERO(&ps, sizeof (mbstate_t));

    while (errno = 0, true) {
	if (mbsrtowcs(&out[wcslen(out)], in_buf, (sz - wcslen(out)) - 1, &ps) == CONVERT_FAILED && errno == EILSEQ) {
	    err_log(EILSEQ, "In perform_convert_buffer: characters lost");
	    (*in_buf)++;
	} else
	    break;
    }

    out[sz - 1] = 0;
    return (out);
}

/* XXX: Don't actually use A_BLINK because it's annoying. */
static void
case_blink(WINDOW *win, bool *is_blink)
{
    if (!*is_blink) {
	WATTR_ON(win, A_REVERSE);
	*is_blink = true;
    } else {
	WATTR_OFF(win, A_REVERSE);
	*is_blink = false;
    }
}

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

static void
case_color(WINDOW *win, bool *is_color, wchar_t **bufp)
{
#define STRLEN_CAST(string) strlen((char *) string)
    bool           has_comma = false;
    char           bg[10]    = { 0 };
    char           fg[10]    = { 0 };
    short int      num1      = -1;
    short int      num2      = -1;
    struct integer_unparse_context unparse_ctx = {
	.setting_name	  = "term_background",
	.fallback_default = 1,	/* black */
	.lo_limit	  = 0,
	.hi_limit	  = 15,
    };
    unsigned char *mbs = NULL;

    if (*is_color) {
	WCOLOR_SET(win, 0);
	*is_color = false;
    }

/***************************************************
 *
 * check for ^CN
 *
 ***************************************************/
    {
	if (!*++(*bufp)) {
	    return;
	}

	mbs = convert_wc(**bufp);
	if (STRLEN_CAST(mbs) != 1 || !sw_isdigit(*mbs)) {
	    (*bufp)--;
	    free(mbs);
	    return;
	}

	sw_snprintf(&fg[0], 2, "%c", *mbs);

	free(mbs);
    }

/***************************************************
 *
 * check for ^CNN or ^CN,
 *
 ***************************************************/
    {
	if (!*++(*bufp)) {
	    return;
	}

	mbs = convert_wc(**bufp);
	if (STRLEN_CAST(mbs) != 1 || (!sw_isdigit(*mbs) && *mbs != ',')) {
	    (*bufp)--;
	    free(mbs);
	    goto out;
	}

	if (sw_isdigit(*mbs)) {
	    sw_snprintf(&fg[1], 2, "%c", *mbs);
	} else if (*mbs == ',') {
	    has_comma = true;
	}

	free(mbs);
    }

/***************************************************
 *
 * check for ^CNN, or ^CN,N
 *
 ***************************************************/
    {
	if (!*++(*bufp)) {
	    return;
	}

	mbs = convert_wc(**bufp);
	if (STRLEN_CAST(mbs) != 1 || (!sw_isdigit(*mbs) && *mbs != ',') ||
	    (*mbs != ',' && fg[1]) || (has_comma && *mbs == ',')) {
	    (*bufp)--;
	    free(mbs);
	    goto out;
	}

	if (sw_isdigit(*mbs)) {
	    sw_snprintf(&bg[0], 2, "%c", *mbs);
	} else if (*mbs == ',') {
	    has_comma = true;
	}

	free(mbs);
    }

/***************************************************
 *
 * check for ^CNN,N or ^CN,NN
 *
 ***************************************************/
    {
	if (!*++(*bufp)) {
	    return;
	}

	mbs = convert_wc(**bufp);
	if (STRLEN_CAST(mbs) != 1 || !sw_isdigit(*mbs)) {
	    (*bufp)--;
	    free(mbs);
	    goto out;
	}

	if (bg[0]) {
	    sw_snprintf(&bg[1], 2, "%c", *mbs);
	    free(mbs);
	    goto out;
	}

	sw_snprintf(&bg[0], 2, "%c", *mbs);

	free(mbs);
    }

/***************************************************
 *
 * check for ^CNN,NN
 *
 ***************************************************/
    {
	if (!*++(*bufp)) {
	    return;
	}

	mbs = convert_wc(**bufp);
	if (STRLEN_CAST(mbs) != 1 || !sw_isdigit(*mbs)) {
	    (*bufp)--;
	    free(mbs);
	    goto out;
	}

	sw_snprintf(&bg[1], 2, "%c", *mbs);

	free(mbs);
    }

  out:
    num1 = (short int) atoi(fg);
    if (!isEmpty(bg)) {
	num2 = (short int) atoi(bg);
    } else if (isEmpty(bg) && theme_bool_unparse("term_use_default_colors", true)) {
	num2 = -1;
    } else {
	num2 = (short int) theme_integer_unparse(&unparse_ctx);
    }

    printtext_set_color(win, is_color, num1, num2);

    if (has_comma && !(bg[0]))
	(*bufp)--;
}

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
case_default(struct case_default_context *ctx,
	     int *rep_count, int *line_count, int *insert_count)
{
    unsigned char *mbs, *p;
    chtype c;

    if (!iswprint(ctx->wc) && ctx->wc != L'\n') {
	return;
    }

    mbs = convert_wc(ctx->wc);
    p = &mbs[0];

    if (is_scrollok(ctx->win)) {
	chtype new_line = '\n';

	if (ctx->wc == L'\n') {
	    WADDCH(ctx->win, new_line);
	    *insert_count = 0;

	    if (rep_count != NULL) {
		(*rep_count)++;
	    }

	    if (ctx->max_lines > 0) {
		if (!( ++(*line_count) < ctx->max_lines )) {
		    free(mbs);
		    return;
		}
	    }

	    if (!ctx->nextchar_empty && ctx->indent > 0) {
		int    counter = 0;
		chtype blank   = ' ';

		while (counter++ != ctx->indent) {
		    WADDCH(ctx->win, blank);
		    (*insert_count)++;
		}
	    }
	} else if ((*insert_count) + ctx->diff + 1 < COLS - 1) {
	    while ((c = *p++) != '\0') {
		WADDCH(ctx->win, c);
	    }

	    (*insert_count)++;
	} else {
	    WADDCH(ctx->win, new_line);
	    *insert_count = 0;

	    if (rep_count != NULL) {
		(*rep_count)++;
	    }

	    if (ctx->max_lines > 0) {
		if (!( ++(*line_count) < ctx->max_lines )) {
		    free(mbs);
		    return;
		}
	    }

	    if (ctx->indent > 0) {
		int    counter = 0;
		chtype blank   = ' ';

		while (counter++ != ctx->indent) {
		    WADDCH(ctx->win, blank);
		    (*insert_count)++;
		}
	    }

#if 1
	    if (ctx->diff && ctx->wc == L' ') {
		free(mbs);
		return;
	    }
#endif

	    while ((c = *p++) != '\0') {
		WADDCH(ctx->win, c);
	    }

	    (*insert_count)++;
	}
    } else { /* not scrollok */
	while ((c = *p++) != '\0') {
	    WADDCH(ctx->win, c);
	}
    }

    free(mbs);
}

static unsigned char *
convert_wc(wchar_t wc)
{
    mbstate_t ps;
#ifdef HAVE_BCI
    size_t bytes_written; /* not used */
#endif
    const size_t size = MB_LEN_MAX + 1;
    unsigned char *mbs = xcalloc(size, 1);

    BZERO(&ps, sizeof (mbstate_t));

#ifdef HAVE_BCI
    if ((errno = wcrtomb_s(&bytes_written, (char *) mbs, size, wc, &ps)) != 0) {
	err_sys("wcrtomb_s error");
    }
#else
    if (wcrtomb((char *) mbs, wc, &ps) == ((size_t) -1)) {
	err_sys("wcrtomb error");
    }
#endif

    return (mbs);
}

static void
printtext_set_color(WINDOW *win, bool *is_color, short int num1, short int num2)
{
    const short int num_colorMap_entries = (short int) ARRAY_SIZE(ptext_colorMap);
    short int fg, bg, resolved_pair;
#if defined(UNIX)
    attr_t attr;
#elif defined(WIN32)
    chtype attr;
#endif

    sw_assert(num1 >= 0); /* num1 shouldn't under any circumstances appear negative */

    fg = ptext_colorMap[num1 % num_colorMap_entries].color;
    bg = (num2 < 0 ? -1 : ptext_colorMap[num2 % num_colorMap_entries].color);

    if ((resolved_pair = color_pair_find(fg, bg)) == -1) {
	WCOLOR_SET(win, 0);
	*is_color = false;
	return;
    }

    attr = ptext_colorMap[num1 % num_colorMap_entries].at; /* attributes of fg */
    attr |= COLOR_PAIR(resolved_pair);
    term_set_attr(win, attr);
    *is_color = true;
}

short int
color_pair_find(short int fg, short int bg)
{
    const short int gipp1 = g_initialized_pairs + 1;
    short int pnum; /* pair number */
    short int x, y;

    for (pnum = 1; pnum < gipp1; pnum++) {
	if (pair_content(pnum, &x, &y) == ERR) {
	    return -1;
	} else if (x == fg && y == bg) { /* found match */
	    return pnum;
	} else {
	    ;
	}
    }

    return -1;
}

void
printtext(struct printtext_context *ctx, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprinttext(ctx, fmt, ap);
    va_end(ap);
}

void
swirc_wprintw(WINDOW *win, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vwprintw(win, fmt, ap);
    va_end(ap);
}

void
print_and_free(const char *msg, char *cp)
{
    struct printtext_context ptext_ctx = {
	.window	    = g_status_window,
	.spec_type  = TYPE_SPEC1_FAILURE,
	.include_ts = true,
    };

    printtext(&ptext_ctx, "%s", msg);
    if (cp) free(cp);
}
