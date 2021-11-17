#ifndef CAP_H
#define CAP_H

__SWIRC_BEGIN_DECLS
bool		 is_sasl_mechanism_supported(const char *);
const char	*get_sasl_mechanism(void);
void		 event_cap(struct irc_message_compo *);
__SWIRC_END_DECLS

#endif
