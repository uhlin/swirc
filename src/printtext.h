#ifndef PRINTTEXT_H
#define PRINTTEXT_H

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

struct printtext_context {
    PIRC_WINDOW			window;
    enum message_specifier_type spec_type;
    bool                        include_ts;
};

#if defined(UNIX)
extern pthread_mutex_t g_puts_mutex;
#elif defined(WIN32)
extern HANDLE g_puts_mutex;
#endif

/*lint -printf(2, printtext, swirc_wprintw) */

char		*squeeze_text_deco (char *buffer);
short int	 color_pair_find   (short int fg, short int bg);
void		 printtext         (struct printtext_context *, const char *fmt, ...) PRINTFLIKE(2);
void		 printtext_puts    (WINDOW *, const char *buf, int indent, int max_lines, int *rep_count);
void		 swirc_wprintw     (WINDOW *, const char *fmt, ...) PRINTFLIKE(2);
void		 vprinttext        (struct printtext_context *, const char *fmt, va_list);

#define print_and_free PrintAndFree
void PrintAndFree(const char *msg, char *cp);

#endif
