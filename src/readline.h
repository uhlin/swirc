#ifndef READLINE_H
#define READLINE_H

#if defined(PANEL_HDR)
#include PANEL_HDR
#elif UNIX
#include <panel.h>
#elif WIN32
#include "pdcurses/panel.h"
#else
#error "Cannot determine panel header file!"
#endif

#include <setjmp.h> /* want type jmp_buf */

#define READLINE_PROCESS 0
#define READLINE_RESTART 1
#define READLINE_TERMINATE 2

typedef enum {
    PANEL1_ACTIVE,
    PANEL2_ACTIVE
} rl_active_panel_t;

enum { /* custom, additional keys */
    MY_KEY_BS	  = '\010',	/* ^H */
    MY_KEY_DEL	  = '\177',	/* ^? */
    MY_KEY_EOT	  = '\004',	/* ^D */
    MY_KEY_ACK	  = '\006',	/* ^F */
    MY_KEY_STX	  = '\002',	/* ^B */
    MY_KEY_SO	  = '\016',	/* ^N */
    MY_KEY_DLE	  = '\020',	/* ^P */
    MY_KEY_RESIZE = '\033',
    CTRL_A = '\001',
    CTRL_E = '\005'
};

#define WINDOWS_KEY_ENTER 459

struct readline_session_context {
    wchar_t *buffer;
    int      bufpos;
    int      n_insert;
    bool     insert_mode;
    bool     no_bufspc;
    char    *prompt;
    int      prompt_size;
    WINDOW  *act;
};

extern bool	 g_readline_loop;
extern bool	 g_resize_requested;
extern jmp_buf	 g_readline_loc_info;

extern bool g_hist_next;
extern bool g_hist_prev;

#ifdef __cplusplus
extern "C" {
#endif

void readline_init(void);
void readline_deinit(void);

/*lint -sem(readline_get_active_pwin, r_null) */
/*lint -sem(readline, r_null) */

WINDOW	*readline_get_active_pwin (void);
char	*readline                 (const char *prompt);
void	 readline_recreate        (int rows, int cols);
void	 readline_top_panel       (void);

#ifdef __cplusplus
}
#endif

#endif
