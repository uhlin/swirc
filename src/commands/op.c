#include "common.h"

#include "../dataClassify.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "op.h"

#ifdef UNIT_TESTING
#undef  ACTWINLABEL
#define ACTWINLABEL "#chatzone"
#endif

#define MISSING_ARGS		"missing arguments"
#define INVALID_NICK		"invalid nickname"
#define NOT_AN_IRC_CHANNEL	"active window isn't an irc channel"

/* usage: /op <nick> */
void
cmd_op(const char *data)
{
    if (strings_match(data, "")) {
	print_and_free("/op: " MISSING_ARGS, NULL);
	return;
    } else if (!is_valid_nickname(data)) {
	print_and_free("/op: " INVALID_NICK, NULL);
	return;
    } else if (!is_irc_channel(ACTWINLABEL)) {
	print_and_free("/op: " NOT_AN_IRC_CHANNEL, NULL);
	return;
    }

    (void) net_send("MODE %s +o %s", ACTWINLABEL, data);
}

/* usage: /deop <nick> */
void
cmd_deop(const char *data)
{
    if (strings_match(data, "")) {
	print_and_free("/deop: " MISSING_ARGS, NULL);
	return;
    } else if (!is_valid_nickname(data)) {
	print_and_free("/deop: " INVALID_NICK, NULL);
	return;
    } else if (!is_irc_channel(ACTWINLABEL)) {
	print_and_free("/deop: " NOT_AN_IRC_CHANNEL, NULL);
	return;
    }

    (void) net_send("MODE %s -o %s", ACTWINLABEL, data);
}
