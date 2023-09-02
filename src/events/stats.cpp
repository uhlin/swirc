#include "common.h"

#include "../irc.h"

#include "stats.h"

void
event_statskline(struct irc_message_compo *compo)
{
	irc_extract_msg(compo, g_active_window, 1, false);
}
