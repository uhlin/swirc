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

static const char	err1[] = "missing arguments";
static const char	err2[] = "invalid nickname";
static const char	err3[] = "active window isn't an irc channel";

/*
 * usage: /op <nick>
 */
void
cmd_op(const char *data)
{
	static const char cmd[] = "/op";

	if (strings_match(data, "")) {
		printtext_print("err", "%s: %s", cmd, err1);
		return;
	} else if (!is_valid_nickname(data)) {
		printtext_print("err", "%s: %s", cmd, err2);
		return;
	} else if (!is_irc_channel(ACTWINLABEL)) {
		printtext_print("err", "%s: %s", cmd, err3);
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
		printtext_print("err", "%s: %s", cmd, err1);
		return;
	} else if (!is_valid_nickname(data)) {
		printtext_print("err", "%s: %s", cmd, err2);
		return;
	} else if (!is_irc_channel(ACTWINLABEL)) {
		printtext_print("err", "%s: %s", cmd, err3);
		return;
	}

	(void) net_send("MODE %s -o %s", ACTWINLABEL, data);
}
