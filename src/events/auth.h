#ifndef AUTH_H
#define AUTH_H

void event_authenticate(struct irc_message_compo *);
void handle_sasl_auth_fail(struct irc_message_compo *);

#endif
