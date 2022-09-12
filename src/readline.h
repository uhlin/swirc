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
#include "textBuffer.h"

#define READLINE_PROCESS	0
#define READLINE_RESTART	1
#define READLINE_TERMINATE	2

typedef enum {
	PANEL1_ACTIVE,
	PANEL2_ACTIVE
} rl_active_panel_t;

enum { /* custom, additional keys */
	MY_KEY_BS     = '\010', /* ^H */
	MY_KEY_DEL    = '\177', /* ^? */
	MY_KEY_EOT    = '\004', /* ^D */
	MY_KEY_ACK    = '\006', /* ^F */
	MY_KEY_STX    = '\002', /* ^B */
	MY_KEY_SO     = '\016', /* ^N */
	MY_KEY_DLE    = '\020', /* ^P */
	MY_KEY_RESIZE = '\033',
	CTRL_A = '\001',
	CTRL_E = '\005',
	CTRL_L = '\014'
};

#define WINDOWS_KEY_ENTER 459

typedef struct tagREADLINE_POS {
	int x;
	int y;
} READLINE_POS, *PREADLINE_POS;

typedef struct tagTAB_COMPLETION {
	char	search_var[64];
	bool	isInCirculationModeForHelp;
	bool	isInCirculationModeForQuery;
	bool	isInCirculationModeForSettings;
	bool	isInCirculationModeForWhois;
	bool	isInCirculationModeForZncCmds;
	bool	isInCirculationModeForCmds;
	bool	isInCirculationModeForChanUsers;
	PTEXTBUF	matches;
	PTEXTBUF_ELMT	elmt;
} TAB_COMPLETION, *PTAB_COMPLETION;

struct readline_session_context {
	PTAB_COMPLETION  tc;
	WINDOW          *act;
	bool             insert_mode;
	bool             no_bufspc;
	char            *prompt;
	int              bufpos;
	int              n_insert;
	int              prompt_size;
	wchar_t         *buffer;
};

__SWIRC_BEGIN_DECLS
extern PREADLINE_POS g_readline_pos;

extern bool	g_readline_loop;
extern bool	g_resize_requested;
extern jmp_buf	g_readline_loc_info;

extern bool	g_hist_next;
extern bool	g_hist_prev;

void readline_init(void);
void readline_deinit(void);

/*lint -sem(readline_get_active_pwin, r_null) */
/*lint -sem(readline, r_null) */

WINDOW	*readline_get_active_pwin(void);
char	*readline(const char *prompt);
char	*readline_finalize_out_string_exported(const wchar_t *);
void	 readline_handle_backspace(volatile struct readline_session_context *);
void	 readline_handle_key_exported(volatile struct readline_session_context *,
	     wint_t);
void	 readline_mouse_init(void);
void	 readline_recreate(int rows, int cols);
void	 readline_top_panel(void);
__SWIRC_END_DECLS

#endif
