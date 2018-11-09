#ifndef WELCOME_H
#define WELCOME_H

#if defined(UNIX)
#include "welcome-unix.h"
#elif defined(WIN32)
#include "welcome-w32.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void	event_welcome(struct irc_message_compo *);

#ifdef __cplusplus
}
#endif

#endif
