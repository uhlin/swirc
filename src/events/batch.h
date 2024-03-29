#ifndef GUARD_BATCH_H
#define GUARD_BATCH_H

typedef enum {
	BATCH_CHATHISTORY,
	BATCH_NETJOIN,
	BATCH_NETSPLIT,
	BATCH_ZNC_IN_PLAYBACK,
	BATCH_UNKNOWN
} batch_t;

__SWIRC_BEGIN_DECLS
void	event_batch(struct irc_message_compo *);
void	event_batch_add_irc_msgs(CSTRING, CSTRING);

void	event_batch_init(void);
void	event_batch_deinit(void);
__SWIRC_END_DECLS

#endif
