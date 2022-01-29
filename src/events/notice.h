#ifndef NOTICE_H
#define NOTICE_H

__SWIRC_BEGIN_DECLS
void	 event_notice(struct irc_message_compo *);
char	*get_notice(const char *, const char *, const char *);
__SWIRC_END_DECLS

#endif
