#ifndef NOTICE_H
#define NOTICE_H

__SWIRC_BEGIN_DECLS
void	event_notice(struct irc_message_compo *);
STRING	get_notice(CSTRING, CSTRING, CSTRING);
__SWIRC_END_DECLS

#endif
