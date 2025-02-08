#ifndef READLINE_H
#define READLINE_H

#if defined(PANEL_HDR)
#include PANEL_HDR
#elif UNIX
#include <panel.h>
#elif WIN32
#include "pdcurses/panel.h"
#else
#error Cannot determine Panel header file!
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
	MY_KEY_BS     = 0x08, /* ^H, octal: 010 */
	MY_KEY_DEL    = 0x7F, /* ^?, octal: 177 */
	MY_KEY_EOT    = 0x04, /* ^D, octal: 004 */
	MY_KEY_ACK    = 0x06, /* ^F, octal: 006 */
	MY_KEY_STX    = 0x02, /* ^B, octal: 002 */
	MY_KEY_SO     = 0x0E, /* ^N, octal: 016 */
	MY_KEY_DLE    = 0x10, /* ^P, octal: 020 */
	MY_KEY_RESIZE = 0x1B, /* ^[, octal: 033 */
	CTRL_A = 0x01, /* octal 001 */
	CTRL_E = 0x05, /* octal 005 */
	CTRL_L = 0x0C, /* octal 014 */
	CTRL_W = 0x17  /* octal 027 */
};

#define WINDOWS_KEY_ENTER 459

typedef struct tagREADLINE_POS {
	int x;
	int y;
} READLINE_POS, *PREADLINE_POS;

typedef struct tagTAB_COMPLETION {
	char	search_var[64];

	struct {
		bool ChanUsers;
		bool Cmds;
		bool Connect;
		bool Cs;
		bool Dcc;
		bool Deop;
		bool Devoice;
		bool Ftp;
		bool Help;
		bool Kick;
		bool Kickban;
		bool Mode;
		bool Msg;
		bool Notice;
		bool Ns;
		bool Op;
		bool Query;
		bool Sasl;
		bool Settings;
		bool Squery;
		bool Theme;
		bool Time;
		bool Version;
		bool Voice;
		bool Whois;
		bool ZncCmds;
	} isInCirculationModeFor;

	PTEXTBUF	matches;
	PTEXTBUF_ELMT	elmt;
} TAB_COMPLETION, *PTAB_COMPLETION;

struct readline_session_context {
	PTAB_COMPLETION  tc;
	STRING           prompt;
	WINDOW          *act;
	bool             insert_mode;
	bool             no_bufspc;
	int              bufpos;
	int              numins;
	int              prompt_size;
	int              vispos; // visual position
	wchar_t         *buffer;
};

__SWIRC_BEGIN_DECLS
extern PREADLINE_POS	 g_readline_pos;
extern const int	 g_readline_bufsize;

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
STRING	 readline(CSTRING prompt);
STRING	 readline_finalize_out_string_exported(const wchar_t *);
void	 readline_handle_backspace(volatile struct readline_session_context *);
void	 readline_handle_key_exported(volatile struct readline_session_context *,
	     wint_t);
void	 readline_mouse_init(void);
void	 readline_recreate(int rows, int cols);
void	 readline_top_panel(void);
__SWIRC_END_DECLS

#endif
