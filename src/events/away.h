#ifndef AWAY_H
#define AWAY_H

#ifdef __cplusplus
extern "C" {
#endif

void	event_away(struct irc_message_compo *);
void	event_unaway(struct irc_message_compo *);
void	event_nowAway(struct irc_message_compo *);

#ifdef __cplusplus
}
#endif

#endif
