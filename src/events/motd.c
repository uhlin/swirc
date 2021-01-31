#include "common.h"

#include "../config.h"
#include "../irc.h"

#include "motd.h"

void
event_motd(struct irc_message_compo *compo)
{
    if (config_bool("skip_motd", false)) {
	return;
    }

    irc_extract_msg(compo, g_status_window, 1, false);
}
