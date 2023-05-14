#ifndef NAMES_H
#define NAMES_H

#include "../textBuffer.h"
#include "names-htbl-modify.h"

__SWIRC_BEGIN_DECLS

/*lint -sem(get_list_of_matching_channel_users, r_null) */

void		cmd_stats(CSTRING);
PTEXTBUF	get_list_of_matching_channel_users(CSTRING chan,
		    CSTRING search_var);

void	event_names_init(void);
void	event_names_deinit(void);

/*lint -sem(event_names_htbl_lookup, r_null) */

PNAMES	event_names_htbl_lookup(CSTRING nick, CSTRING channel);
int	event_names_htbl_insert(CSTRING nick, CSTRING channel);
int	event_names_htbl_remove(CSTRING nick, CSTRING channel);
void	event_eof_names(struct irc_message_compo *);
void	event_names(struct irc_message_compo *);
void	event_names_htbl_remove_all(PIRC_WINDOW);

__SWIRC_END_DECLS

#endif
