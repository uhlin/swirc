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

/*
 * usage: /op <nick>
 */
void
cmd_op(const char *data)
{
	static const char cmd[] = "/op";

	if (strings_match(data, "")) {
		printtext_print("err", "%s: %s", cmd, MISSING_ARGS);
		return;
	} else if (!is_valid_nickname(data)) {
		printtext_print("err", "%s: %s", cmd, INVALID_NICK);
		return;
	} else if (!is_irc_channel(ACTWINLABEL)) {
		printtext_print("err", "%s: %s", cmd, NOT_AN_IRC_CHANNEL);
		return;
	}

	(void) net_send("MODE %s +o %s", ACTWINLABEL, data);
}

/*
 * usage: /deop <nick>
 */
void
cmd_deop(const char *data)
{
	static const char cmd[] = "/deop";

	if (strings_match(data, "")) {
		printtext_print("err", "%s: %s", cmd, MISSING_ARGS);
		return;
	} else if (!is_valid_nickname(data)) {
		printtext_print("err", "%s: %s", cmd, INVALID_NICK);
		return;
	} else if (!is_irc_channel(ACTWINLABEL)) {
		printtext_print("err", "%s: %s", cmd, NOT_AN_IRC_CHANNEL);
		return;
	}

	(void) net_send("MODE %s -o %s", ACTWINLABEL, data);
}
