#ifndef CAP_H
#define CAP_H

bool		 is_sasl_mechanism_supported(const char *mechanism);
const char	*get_sasl_mechanism(void);
void		 event_cap(struct irc_message_compo *);

#endif
