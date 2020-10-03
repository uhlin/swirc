#include "common.h"

#include "../irc.h"
#include "../printtext.h"

#include "servlist.h"

/* event_servlist: 234 (RPL_SERVLIST)

   Examples:
     :irc.server.com 234 <me> <name> <server> <mask> <type> <hopcount> <info>
     :irc.server.com 234 <me> Alis@hub.uk hub.uk * 0xD000 1 :[...]
     :irc.server.com 234 <me> Clis@hub.uk hub.uk * 0xD000 1 :[...] */
void
event_servlist(struct irc_message_compo *compo)
{
    (void) compo;
}

/* event_servlistEnd: 235 (RPL_SERVLISTEND)

   Example:
     :irc.server.com 235 <me> <mask> <type> :End of service listing */
void
event_servlistEnd(struct irc_message_compo *compo)
{
    (void) compo;
}
