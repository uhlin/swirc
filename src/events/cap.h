#ifndef CAP_H
#define CAP_H

#ifdef __cplusplus
extern "C" {
#endif

bool		 is_sasl_mechanism_supported(const char *mechanism);
const char	*get_sasl_mechanism(void);
void		 event_cap(struct irc_message_compo *);

#ifdef __cplusplus
}
#endif

#endif
