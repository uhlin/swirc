#include "common.h"

#include "../dataClassify.h"
#include "../irc.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "account.h"
#include "names.h"

/* event_account

   Examples:
     :nick!user@host ACCOUNT <accountname> (logged into a new account)
     :nick!user@host ACCOUNT *             (logged out of their account) */
void
event_account(struct irc_message_compo *compo)
{
    char *accountname = & (compo->params[0]);
    char *last = (char *) "";
    char *nick, *user, *host;
    char *prefix = & (compo->prefix[1]);
    struct printtext_context ctx(NULL, TYPE_SPEC1_SPEC2, true);

    if ((nick = strtok_r(prefix, "!@", &last)) == NULL ||
	(user = strtok_r(NULL, "!@", &last)) == NULL ||
	(host = strtok_r(NULL, "!@", &last)) == NULL)
	return;

    if (*accountname == ':')
	accountname++;

    const bool logged_out = strings_match(accountname, "*");

    for (int i = 1; i <= g_ntotal_windows; i++) {
	PIRC_WINDOW window = window_by_refnum(i);

	if (window && is_irc_channel(window->label) &&
	    event_names_htbl_lookup(nick, window->label) != NULL) {
	    ctx.window = window;

	    if (logged_out) {
		printtext(&ctx, "%s%s%c %s%s@%s%s has logged out "
		    "of their account",
		    COLOR2, nick, NORMAL,
		    LEFT_BRKT, user, host, RIGHT_BRKT);
	    } else {
		/**
		 * Logged in
		 */

		printtext(&ctx, "%s%s%c %s%s@%s%s has logged into "
		    "a new account %s%s%c",
		    COLOR1, nick, NORMAL,
		    LEFT_BRKT, user, host, RIGHT_BRKT,
		    COLOR4, accountname, NORMAL);
	    }
	}
    }
}
