#ifndef GUARD_BANLIST_H
#define GUARD_BANLIST_H

#define event_inviteList event_banlist
#define event_exceptList event_banlist

#define event_eof_inviteList event_eof_banlist
#define event_eof_exceptList event_eof_banlist

__SWIRC_BEGIN_DECLS
void event_banlist     (struct irc_message_compo *);
void event_eof_banlist (struct irc_message_compo *);
__SWIRC_END_DECLS

#endif
