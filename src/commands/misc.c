#include "common.h"

#include "../config.h"
#include "../dataClassify.h"
#include "../io-loop.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../strHand.h"

#include "misc.h"

static struct printtext_context ptext_ctx = {
    .window	= NULL,
    .spec_type	= TYPE_SPEC1_FAILURE,
    .include_ts = true,
};

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

/* usage: /whois <nick> */
void
cmd_whois(const char *data)
{
    ptext_ctx.window = g_active_window;

    if (Strings_match(data, "")) {
	printtext(&ptext_ctx, "/whois: missing arguments");
    } else if (!is_valid_nickname(data)) {
	printtext(&ptext_ctx, "/whois: bogus nickname");
    } else {
	net_send(g_socket, 0, "WHOIS %s %s", data, data);
    }
}
