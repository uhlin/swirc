#include "common.h"

#include "../config.h"
#include "../io-loop.h"
#include "../network.h"
#include "../strHand.h"

#include "misc.h"

/* usage: /quit [message] */
void
cmd_quit(const char *data)
{
    const bool has_message = !Strings_match(data, "");

    if (g_on_air) {
	if (has_message)
	    net_send(g_socket, 0, "QUIT :%s", data);
	else
	    net_send(g_socket, 0, "QUIT :%s", Config("quit_message"));
	g_on_air = false;
	net_listenThread_join();
    }

    g_io_loop = false;
}
