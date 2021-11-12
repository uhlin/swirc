#include "common.h"

#include "../errHand.h"
#include "../irc.h"

#include "pong.h"

void
event_pong(struct irc_message_compo *compo)
{
	debug("Got event %s", compo->command);
}
