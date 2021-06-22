#ifndef IRC_H
#define IRC_H

#include "window.h"

struct irc_message_compo {
	int year;
	int month;
	int day;

	int hour;
	int minute;
	int second;
	int precision;

	char *prefix;
	char *command;
	char *params;
};

typedef void (*event_handler_fn)(struct irc_message_compo *);

enum to_window {
	STATUS_WINDOW,
	ACTIVE_WINDOW,
	NO_WINDOW
};

enum message_concat_state {
	CONCAT_BUFFER_IS_EMPTY,
	CONCAT_BUFFER_CONTAIN_DATA
};

#define UNKNOWN_EVENT_DISPLAY_EXTENDED_INFO 1

__SWIRC_BEGIN_DECLS
extern const char g_forbidden_chan_name_chars[];

extern bool	 g_alt_nick_tested;
extern bool	 g_am_irc_op;
extern bool	 g_is_away;

extern char	*g_my_nickname;
extern char	*g_server_hostname;

void	irc_init(void);
void	irc_deinit(void);

bool	has_server_time(const struct irc_message_compo *);
void	irc_extract_msg(struct irc_message_compo *, PIRC_WINDOW, int ext_bits,
	    bool is_error);
void	irc_handle_interpret_events(char *recvbuffer, char **message_concat,
	    enum message_concat_state *);
void	irc_set_my_nickname(const char *);
void	irc_set_server_hostname(const char *);
__SWIRC_END_DECLS

#endif
