#ifndef CHANNEL_H
#define CHANNEL_H

typedef enum {
	STATE_PLUS,
	STATE_MINUS,
	STATE_NEITHER_PM
} plus_minus_state_t;

__SWIRC_BEGIN_DECLS
void	event_chan_hp(struct irc_message_compo *);
void	event_join(struct irc_message_compo *);
void	event_kick(struct irc_message_compo *);
void	event_mode(struct irc_message_compo *);
void	event_nick(struct irc_message_compo *);
void	event_part(struct irc_message_compo *);
void	event_quit(struct irc_message_compo *);
void	event_topic(struct irc_message_compo *);
void	event_topic_chg(struct irc_message_compo *);
void	event_topic_creator(struct irc_message_compo *);
__SWIRC_END_DECLS

#endif
