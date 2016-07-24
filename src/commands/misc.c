#include "common.h"

#include "../config.h"
#include "../dataClassify.h"
#include "../errHand.h"
#include "../io-loop.h"
#include "../network.h"
#include "../printtext.h"
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
	    (void) net_send("QUIT :%s", data);
	else
	    (void) net_send("QUIT :%s", Config("quit_message"));
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
	if (net_send("WHOIS %s %s", data, data) < 0)
	    g_on_air = false;
    }
}

/* usage: /query [nick] */
void
cmd_query(const char *data)
{
    ptext_ctx.window = g_active_window;

    if (Strings_match(data, "")) {
	if (is_valid_nickname(g_active_window->label)) {
	    switch (destroy_chat_window(g_active_window->label)) {
	    case EINVAL:
		err_exit(EINVAL, "destroy_chat_window");
	    case ENOENT:
		printtext(&ptext_ctx, "/query: cannot find window!");
		break;
	    }
	} else {
	    printtext(&ptext_ctx, "/query: missing arguments");
	}
    } else if (!is_valid_nickname(data)) {
	printtext(&ptext_ctx, "/query: bogus nickname");
    } else {
	switch (spawn_chat_window(data, "")) {
	case EINVAL:
	    err_exit(EINVAL, "spawn_chat_window");
	case ENOSPC:
	    printtext(&ptext_ctx, "/query: too many windows open!");
	    break;
	}
    }
}

/* usage: /n [channel] */
void
cmd_names(const char *data)
{
    extern int event_names_print_all(const char *channel);

    ptext_ctx.window = g_active_window;

    if (Strings_match(data, "")) {
	if (is_irc_channel(g_active_window->label)) {
	    event_names_print_all(g_active_window->label);
	} else {
	    printtext(&ptext_ctx, "/n: missing arguments");
	}
    } else if (!is_irc_channel(data)) {
	printtext(&ptext_ctx, "/n: bogus irc channel");
    } else {
	event_names_print_all(data);
    }
}

/* usage: /mode <modes> [...] */
void
cmd_mode(const char *data)
{
    ptext_ctx.window = g_active_window;

    if (Strings_match(data, "")) {
	printtext(&ptext_ctx, "/mode: missing arguments");
    } else {
	if (net_send("MODE %s", data) < 0)
	    g_on_air = false;
    }
}
