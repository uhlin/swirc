#ifndef WINDOW_H
#define WINDOW_H

#if defined(PANEL_HDR)
#include PANEL_HDR
#elif UNIX
#include <panel.h>
#elif WIN32
#include "pdcurses/panel.h"
#else
#error "Cannot determine panel header file!"
#endif

#include "textBuffer.h"

#define ACTWINLABEL g_active_window->label
#define NAMES_HASH_TABLE_SIZE 4500

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagNAMES {
    char	*nick;
    bool	 is_owner;
    bool	 is_superop;
    bool	 is_op;
    bool	 is_halfop;
    bool	 is_voice;
    struct tagNAMES *next;
} NAMES, *PNAMES;

typedef struct tagIRC_WINDOW {
    char	*label;		/* Should not be case-sensitive */
    char	*title;
    PANEL	*pan;
    int		 refnum;
    PTEXTBUF	 buf;
    int		 saved_size;
    int		 scroll_count;
    bool	 scroll_mode;
    PNAMES	 names_hash[NAMES_HASH_TABLE_SIZE];
    bool	 received_names;
    int		 num_owners;
    int		 num_superops;
    int		 num_ops;
    int		 num_halfops;
    int		 num_voices;
    int		 num_normal;
    int		 num_total;
    char chanmodes[256];
    bool received_chanmodes;
    bool received_chancreated;
    struct tagIRC_WINDOW *next;
} IRC_WINDOW, *PIRC_WINDOW;

extern const char       g_status_window_label[];
extern PIRC_WINDOW	g_status_window;
extern PIRC_WINDOW	g_active_window;
extern int              g_ntotal_windows;

void windowSystem_init(void);
void windowSystem_deinit(void);

/*lint -sem(window_by_label, r_null) */
/*lint -sem(window_by_refnum, r_null) */

PIRC_WINDOW	window_by_label(const char *);
PIRC_WINDOW	window_by_refnum(int);
int		changeWindow_by_label(const char *);
int		changeWindow_by_refnum(int);
int		destroy_chat_window(const char *label);
int		spawn_chat_window(const char *label, const char *title);
void		new_window_title(const char *label, const char *title);
void		window_close_all_priv_conv(void);
void		window_foreach_destroy_names(void);
void		window_foreach_rejoin_all_channels(void);
void		window_scroll_down(PIRC_WINDOW);
void		window_scroll_up(PIRC_WINDOW);
void		window_select_next(void);
void		window_select_prev(void);
void		windows_recreate_all(int rows, int cols);

#ifdef __cplusplus
}
#endif

#endif
