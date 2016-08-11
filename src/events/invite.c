#include "common.h"

#include "../irc.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "invite.h"

/* event_inviting: 341 (RPL_INVITING)

   Example:
     :irc.server.com 341 <my nick> <targ_nick> <channel> */
void
event_inviting(struct irc_message_compo *compo)
{
    struct printtext_context ctx = {
	.window	    = g_active_window,
	.spec_type  = TYPE_SPEC1_SUCCESS,
	.include_ts = true,
    };

    (void) compo;

    printtext(&ctx, "Invitation passed onto the end client");
}

/* event_invite

   Example:
     :<nick>!<user>@<host> INVITE <target> :<channel> */
void
event_invite(struct irc_message_compo *compo)
{
    char	*nick, *user, *host;
    char	*prefix = &compo->prefix[1];
    char	*state1, *state2;
    char	*target, *channel;
    struct printtext_context ctx = {
	.window	    = g_active_window,
	.spec_type  = TYPE_SPEC1,
	.include_ts = true,
    };

    state1 = state2 = "";

    if ((nick = strtok_r(prefix, "!@", &state1)) == NULL
	|| (user = strtok_r(NULL, "!@", &state1)) == NULL
	|| (host = strtok_r(NULL, "!@", &state1)) == NULL) {
	return;
    }

    if (Strfeed(compo->params, 1) != 1)
	return;

    if ((target = strtok_r(compo->params, "\n", &state2)) == NULL ||
	(channel = strtok_r(NULL, "\n", &state2)) == NULL)
	return;

    if (*channel == ':')
	channel++;

    if (Strings_match_ignore_case(target, g_my_nickname)) {
	printtext(&ctx, "%c%s%c %s%s@%s%s invites you to %c%s%c",
		  BOLD, nick, BOLD, LEFT_BRKT, user, host, RIGHT_BRKT,
		  BOLD, channel, BOLD);
    }
}
