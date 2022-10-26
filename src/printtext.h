#ifndef PRINTTEXT_H
#define PRINTTEXT_H

#include "irc.h"
#include "mutex.h"
#include "window.h"

/* text decoration */
enum {
	BLINK     = '\035',
	BOLD      = '\002',
	COLOR     = '\003',
	NORMAL    = '\017',
	REVERSE   = '\026',
	UNDERLINE = '\037'
};

#define TXT_BLINK     "\035"
#define TXT_BOLD      "\002"
#define TXT_COLOR     "\003"
#define TXT_NORMAL    "\017"
#define TXT_REVERSE   "\026"
#define TXT_UNDERLINE "\037"

enum message_specifier_type {
	TYPE_SPEC1,
	TYPE_SPEC2,
	TYPE_SPEC3,
	TYPE_SPEC1_SPEC2,
	TYPE_SPEC1_FAILURE,
	TYPE_SPEC1_SUCCESS,
	TYPE_SPEC1_WARN,
	TYPE_SPEC_NONE
};

typedef struct tagPRINTTEXT_CONTEXT {
	PIRC_WINDOW window;
	enum message_specifier_type spec_type;
	bool include_ts;

	char server_time[64];
	bool has_server_time;
} PRINTTEXT_CONTEXT, *PPRINTTEXT_CONTEXT;

__SWIRC_BEGIN_DECLS
#if defined(UNIX)
extern pthread_mutex_t g_puts_mutex;
#elif defined(WIN32)
extern HANDLE g_puts_mutex;
#endif

extern const char g_textdeco_chars[];
__SWIRC_END_DECLS

/* ----------------------------------------------------------------- */

/*lint -sem(windows_convert_to_utf8, r_null) */
/*lint -sem(try_convert_buf_with_cs, r_null) */

__SWIRC_BEGIN_DECLS
PPRINTTEXT_CONTEXT
	printtext_context_new(PIRC_WINDOW, enum message_specifier_type,
	    bool include_ts);
void	printtext_context_destroy(PPRINTTEXT_CONTEXT);
void	printtext_context_init(PPRINTTEXT_CONTEXT, PIRC_WINDOW,
	    enum message_specifier_type, bool include_ts);

/*lint -printf(2, printf_and_free) */
/*lint -printf(2, printtext) */
/*lint -printf(2, printtext_print) */

char	*squeeze_text_deco(char *);
short int
	 color_pair_find(short int foreground, short int background);
void	 print_and_free(const char *msg, char *);
void	 printf_and_free(char *, const char *, ...) PRINTFLIKE(2);
void	 printtext(PPRINTTEXT_CONTEXT, const char *, ...) PRINTFLIKE(2);
void	 printtext_print(const char *what, const char *, ...) PRINTFLIKE(2);
void	 printtext_puts(WINDOW *, const char *buf, int indent, int, int *);
void	 printtext_set_color(WINDOW *, bool *, short int, short int);
void	 set_timestamp(char *dest, size_t destsize,
	     const struct irc_message_compo *) PTR_ARGS_NONNULL;
void	 vprinttext(PPRINTTEXT_CONTEXT, const char *format, va_list);
__SWIRC_END_DECLS

#endif
