#ifndef SRC_WINDOW_H_
#define SRC_WINDOW_H_

#if defined(PANEL_HDR)
#include PANEL_HDR
#elif UNIX
#include <panel.h>
#elif WIN32
#include "pdcurses/panel.h"
#else
#error Cannot determine Panel header file!
#endif

#include "atomicops.h"
#include "textBuffer.h"

#define ACTWINLABEL g_active_window->label
#define NAMES_HASH_TABLE_SIZE 4500

typedef enum {
	PLUS	= '+',
	MINUS	= '-'
} plus_minus_t;

typedef struct tagNAMES {
	char	*nick;
	char	*account;
	char	*rl_name;

	bool	 is_owner;
	bool	 is_superop;
	bool	 is_op;
	bool	 is_halfop;
	bool	 is_voice;
	struct tagNAMES *next;
} NAMES, *PNAMES;

typedef struct tagIRC_WINDOW {
	PANEL		*pan;
	PNAMES		 names_hash[NAMES_HASH_TABLE_SIZE];
	PTEXTBUF	 buf;
	bool		 logging;
	bool		 received_chancreated;
	bool		 received_chanmodes;
	bool		 received_names;
	bool		 scroll_mode;
	char		 chanmodes[256];
	STRING		 label; /* Should not be case-sensitive */
	STRING		 title;

	int	num_owners;
	int	num_superops;
	int	num_ops;
	int	num_halfops;
	int	num_voices;
	int	num_normal;
	int	num_total;

	struct {
		PANEL	*pan;
		int	 scroll_pos;
		int	 width;
	} nicklist;

	int	refnum;
	int	saved_size;
	int	scroll_count;
	struct tagIRC_WINDOW *next;
} IRC_WINDOW, *PIRC_WINDOW;

__SWIRC_BEGIN_DECLS
extern PIRC_WINDOW	g_active_window;
extern PIRC_WINDOW	g_status_window;
extern const char       g_status_window_label[10];
extern const int	g_scroll_amount;
extern _Atomic(int)	g_ntotal_windows;
extern volatile bool	g_redrawing_window;

void windowSystem_init(void);
void windowSystem_deinit(void);

/*lint -sem(window_by_label, r_null) */
/*lint -sem(window_by_refnum, r_null) */
/*lint -sem(get_list_of_matching_channels, r_null) */
/*lint -sem(get_list_of_matching_queries, r_null) */

PIRC_WINDOW	window_by_label(CSTRING);
PIRC_WINDOW	window_by_refnum(int);
PTEXTBUF	get_list_of_matching_channels(CSTRING search_var);
PTEXTBUF	get_list_of_matching_queries(CSTRING search_var);
errno_t		change_window_by_label(CSTRING);
errno_t		change_window_by_refnum(int);
errno_t		destroy_chat_window(CSTRING label);
errno_t		spawn_chat_window(CSTRING label, CSTRING title);
void		new_window_title(CSTRING label, CSTRING title);
void		window_close_all_priv_conv(void);
void		window_foreach_destroy_names(void);
void		window_foreach_rejoin_all_channels(void);
void		window_recreate_exported(PIRC_WINDOW, int rows, int cols);
void		window_scroll_down(PIRC_WINDOW, const int);
void		window_scroll_up(PIRC_WINDOW, const int);
void		window_select_next(void);
void		window_select_prev(void);
void		windows_recreate_all(int rows, int cols);
__SWIRC_END_DECLS

#endif
