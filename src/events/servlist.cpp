#include "common.h"

#include <stdexcept>

#include "../dataClassify.h"
#include "../irc.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "servlist.h"

/* event_servlist: 234 (RPL_SERVLIST)

   Examples:
     :irc.server.com 234 <me> <name> <server> <mask> <type> <hopcount> <info>
     :irc.server.com 234 <me> Alis@hub.uk hub.uk * 0xD000 1 :[...]
     :irc.server.com 234 <me> Clis@hub.uk hub.uk * 0xD000 1 :[...] */
void
event_servlist(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

    try {
	char *state = const_cast<char *>("");

	if (strFeed(compo->params, 6) != 6)
	    throw std::runtime_error("strFeed");

	(void) strtok_r(compo->params, "\n", &state); /* me */
	char	*name	  = strtok_r(NULL, "\n", &state);
	char	*server	  = strtok_r(NULL, "\n", &state);
	char	*mask	  = strtok_r(NULL, "\n", &state);
	char	*type	  = strtok_r(NULL, "\n", &state);
	char	*hopcount = strtok_r(NULL, "\n", &state);
	char	*info	  = strtok_r(NULL, "\n", &state);

	if (isNull(name) || isNull(server) || isNull(mask) || isNull(type) ||
	    isNull(hopcount) || isNull(info))
	    throw std::runtime_error("unable to retrieve event components");

	if (*info == ':')
	    info++;

	printtext(&ctx, "%s%s%c%s%s%s: %s",
	    COLOR1, name, NORMAL,
	    Theme("notice_inner_b1"), mask, Theme("notice_inner_b2"),
	    info);
    } catch (const std::runtime_error &e) {
	ctx.window    = g_status_window;
	ctx.spec_type = TYPE_SPEC1_FAILURE;
	printtext(&ctx, "event_servlist(%s): error: %s", compo->command,
	    e.what());
    }
}

/* event_servlistEnd: 235 (RPL_SERVLISTEND)

   Example:
     :irc.server.com 235 <me> <mask> <type> :End of service listing */
void
event_servlistEnd(struct irc_message_compo *compo)
{
    (void) compo;
}
