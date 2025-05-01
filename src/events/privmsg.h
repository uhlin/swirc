#ifndef PRIVMSG_H
#define PRIVMSG_H

#ifdef __cplusplus
#include <string>
#endif

__SWIRC_BEGIN_DECLS
void	event_privmsg(struct irc_message_compo *);
#ifdef __cplusplus
void	replace_signs(std::string &);
#endif
__SWIRC_END_DECLS

#endif
