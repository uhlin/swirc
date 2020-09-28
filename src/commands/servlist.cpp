#include "common.h"

#include "../network.h"
#include "../strHand.h"

#include "servlist.h"

/*
 * usage: /servlist [<mask> [<type>]]
 */
void
cmd_servlist(const char *data)
{
    if (strings_match(data, ""))
	(void) net_send("SERVLIST");
    else
	(void) net_send("SERVLIST %s", data);
}
