#ifndef WHOIS_H
#define WHOIS_H

__SWIRC_BEGIN_DECLS
void	event_whoReply(struct irc_message_compo *);
void	event_whois_acc(struct irc_message_compo *);
void	event_whois_away(struct irc_message_compo *);
void	event_whois_bot(struct irc_message_compo *);
void	event_whois_cert(struct irc_message_compo *);
void	event_whois_channels(struct irc_message_compo *);
void	event_whois_conn(struct irc_message_compo *);
void	event_whois_host(struct irc_message_compo *);
void	event_whois_idle(struct irc_message_compo *);
void	event_whois_ircOp(struct irc_message_compo *);
void	event_whois_modes(struct irc_message_compo *);
void	event_whois_server(struct irc_message_compo *);
void	event_whois_service(struct irc_message_compo *);
void	event_whois_ssl(struct irc_message_compo *);
void	event_whois_user(struct irc_message_compo *);
__SWIRC_END_DECLS

#endif
