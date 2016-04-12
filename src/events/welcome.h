#ifndef WELCOME_H
#define WELCOME_H

#if defined(UNIX)
#include "welcome-unix.h"
#elif defined(WIN32)
#include "welcome-w32.h"
#endif

void event_welcome(struct irc_message_compo *);

#endif
