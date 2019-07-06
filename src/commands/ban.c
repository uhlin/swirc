#include "common.h"

#include "../dataClassify.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "ban.h"

#ifdef UNIT_TESTING
#undef  ACTWINLABEL
#define ACTWINLABEL "#chatzone"
#endif

#define NO_MASK "no mask"
#define NOT_AN_IRC_CHANNEL "active window isn't an irc channel"

/* usage: /ban <mask> */
void
cmd_ban(const char *data)
{
    if (strings_match(data, "")) {
	print_and_free("/ban: " NO_MASK, NULL);
	return;
    } else if (!is_irc_channel(ACTWINLABEL)) {
	print_and_free("/ban: " NOT_AN_IRC_CHANNEL, NULL);
	return;
    }

    (void) net_send("MODE %s +b %s", ACTWINLABEL, data);
}

/* usage: /unban <mask> */
void
cmd_unban(const char *data)
{
    if (strings_match(data, "")) {
	print_and_free("/unban: " NO_MASK, NULL);
	return;
    } else if (!is_irc_channel(ACTWINLABEL)) {
	print_and_free("/unban: " NOT_AN_IRC_CHANNEL, NULL);
	return;
    }

    (void) net_send("MODE %s -b %s", ACTWINLABEL, data);
}
