/* Connect and Disconnect commands
   Copyright (C) 2016-2020 Markus Uhlin. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

   - Neither the name of the author nor the names of its contributors may be
     used to endorse or promote products derived from this software without
     specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

#include "common.h"

#include <openssl/crypto.h> /* OPENSSL_cleanse() */

#include "../assertAPI.h"
#include "../config.h"
#include "../curses-funcs.h"
#include "../dataClassify.h"
#include "../libUtils.h"
#include "../main.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "connect.h"

static bool quit_reconnecting = false;
static bool reconnecting = false;
static bool secure_connection = false;

static const char *afternet_servers[] = {
    "irc.afternet.org",
    NULL
};

static const char *anonops_servers[] = {
    "anonops532vcpz6z.onion",
    "irc.anonops.com",
    NULL
};

static const char *blitzed_servers[] = {
    "irc.blitzed.org",
    NULL
};

static const char *efnet_servers[] = {
    "efnet.port80.se",
    "efnet.portlane.se",
    "irc.choopa.net",
    "irc.efnet.nl",
    "irc.efnet.org",
    "irc.inet.tele.dk",
    "irc.mzima.net",
    "irc.servercentral.net",
    "irc.underworld.no",
    NULL
};

static const char *freenode_servers[] = {
    "card.freenode.net",
    "chat.freenode.net",
    "chat.ipv6.freenode.net",
    "cherryh.freenode.net",
    "freenodeok2gncmy.onion",
    "hobana.freenode.net",
    "leguin.acc.umu.se",
    "orwell.freenode.net",
    "verne.freenode.net",
    "weber.freenode.net",
    NULL
};

static const char *ircnet_servers[] = {
    "eris.us.ircnet.net",
    "irc.atw-inter.net",
    "irc.nlnog.net",
    "irc.portlane.se",
    "open.ircnet.net",
    NULL
};

static const char *quakenet_servers[] = {
    "ServerCentral.IL.US.Quakenet.Org",
    "irc.quakenet.org",
    "portlane.se.quakenet.org",
    "underworld1.no.quakenet.org",
    "underworld2.no.quakenet.org",
    NULL
};

static const char *undernet_servers[] = {
    "Ashburn.Va.Us.UnderNet.org",
    "irc.undernet.org",
    "undernet.rethemhosting.net",
    NULL
};

static const char *test_servers[] = {
    "misc-services.exbit.io",
    "testnet.inspircd.org",
    "testnet.oragono.io",
    NULL
};

static bool
shouldConnectUsingPassword()
{
    char answer[20] = "";
    int c = EOF;

    while (BZERO(answer, sizeof answer), true) {
	printf("Connect using password? [Y/n]: ");
	fflush(stdout);

	if (fgets(answer, sizeof answer, stdin) == NULL) {
	    putchar('\n');
	    continue;
	} else if (strchr(answer, '\n') == NULL) {
	    puts("input too big");
	    while (c = getchar(), c != '\n' && c != EOF)
		/* discard */;
	} else if (strings_match(trim(answer), "") ||
		   strings_match(answer, "y") ||
		   strings_match(answer, "Y")) {
	    break;
	} else if (strings_match(answer, "n") || strings_match(answer, "N")) {
	    return false;
	} else {
	    continue;
	}
    }

    return true;
}

/*lint -sem(get_password, r_null) */
static char *
get_password()
{
    static char pass[400] = "";
    int c = EOF;

    escape_curses();

    if (!shouldConnectUsingPassword()) {
	resume_curses();
	return NULL;
    }

    while (true) {
	printf("Password (will echo): ");
	fflush(stdout);

	const bool fgets_error = fgets(pass, ARRAY_SIZE(pass), stdin) == NULL;

	if (fgets_error) {
	    putchar('\n');
	    continue;
	} else if (strchr(pass, '\n') == NULL) {
	    puts("input too big");
	    while (c = getchar(), c != '\n' && c != EOF)
		/* discard */;
	} else if (strings_match(trim(pass), "")) {
	    continue;
	} else {
	    break;
	}
    }

    resume_curses();
    return (&pass[0]);
}

static const char *
get_server(const char *ar[], const char *msg)
{
    char ans[20] = "";
    int c = EOF;
    int i = 0;
    int srvno = 0;

    escape_curses();
    puts(msg);

    for (; ar[i] != NULL; i++)
	printf("    (%d) %s\n", i, ar[i]);
    if (i > 0)
	i--;
    while (BZERO(ans, sizeof ans), true) {
	printf("Your choice (0-%d): ", i);
	fflush(stdout);

/*
 * sscanf() is safe in this context
 */
#if WIN32
#pragma warning(disable: 4996)
#endif
	if (fgets(ans, sizeof ans, stdin) == NULL) {
	    putchar('\n');
	    continue;
	} else if (strchr(ans, '\n') == NULL) {
	    puts("input too big");
	    while (c = getchar(), c != '\n' && c != EOF)
		/* discard */;
	} else if (sscanf(ans, "%d", &srvno) != 1 || srvno < 0 || srvno > i) {
	    ;
	} else {
	    break;
	}
/*
 * Reset warning behavior to its default value
 */
#if WIN32
#pragma warning(default: 4996)
#endif
    }

    resume_curses();
    return (ar[srvno]);
}

static void
reconnect_begin()
{
    quit_reconnecting = false;
    reconnecting = true;
}

static void
reconnect_end()
{
    reconnecting = false;
}

static void
turn_icb_mode_on()
{
    g_icb_mode = true;
}

void
do_connect(const char *server, const char *port, const char *pass)
{
    PRINTTEXT_CONTEXT ptext_ctx;
    struct network_connect_context conn_ctx = {
	.server   = (char *) server,
	.port     = (char *) port,
	.password = (char *) pass,
	.username = "",
	.rl_name  = "",
	.nickname = "",
    };

    printtext_context_init(&ptext_ctx, g_status_window, TYPE_SPEC1_FAILURE,
	true);

    if (g_cmdline_opts->username) {
	conn_ctx.username = g_cmdline_opts->username;
    } else if (Config_mod("username")) {
	conn_ctx.username = Config_mod("username");
    } else {
	printtext(&ptext_ctx, "Unable to connect: No username");
	return;
    }

    if (g_cmdline_opts->rl_name) {
	conn_ctx.rl_name = g_cmdline_opts->rl_name;
    } else if (Config_mod("real_name")) {
	conn_ctx.rl_name = Config_mod("real_name");
    } else {
	printtext(&ptext_ctx, "Unable to connect: No real name");
	return;
    }

    if (g_cmdline_opts->nickname) {
	conn_ctx.nickname = g_cmdline_opts->nickname;
    } else if (Config_mod("nickname")) {
	conn_ctx.nickname = Config_mod("nickname");
    } else {
	printtext(&ptext_ctx, "Unable to connect: No nickname");
	return;
    }

    if (!is_valid_username(conn_ctx.username)) {
	printtext(&ptext_ctx, "Unable to connect: Invalid username: \"%s\"",
		  conn_ctx.username);
	return;
    } else if (!is_valid_real_name(conn_ctx.rl_name)) {
	printtext(&ptext_ctx, "Unable to connect: Invalid real name: \"%s\"",
		  conn_ctx.rl_name);
	return;
    } else if (!is_valid_nickname(conn_ctx.nickname)) {
	printtext(&ptext_ctx, "Unable to connect: Invalid nickname: \"%s\"",
		  conn_ctx.nickname);
	return;
    } else {
	/*
	 * The value of sleep_time_seconds assigned below isn't intent
	 * to be used. net_connect() is responsible for giving it an
	 * initial value as well as changing/updating it between
	 * reconnect attempts to reflect the delay.
	 */
	long int sleep_time_seconds = 10;

	if (!g_icb_mode && strings_match(conn_ctx.port, ICB_PORT))
	    turn_icb_mode_on();
	else if (!ssl_is_enabled() && strings_match(conn_ctx.port, SSL_PORT))
	    set_ssl_on();

	ptext_ctx.spec_type = TYPE_SPEC2;
	reconnect_begin();

	while (net_connect(&conn_ctx, &sleep_time_seconds) ==
	    SHOULD_RETRY_TO_CONNECT) {

	    printtext(&ptext_ctx, "Next reconnect attempt in %ld seconds...",
		      sleep_time_seconds);

	    for (long int secs = 0; secs < sleep_time_seconds; secs++) {
		napms(1000);

		if (quit_reconnecting) {
		    net_kill_connection();
		    net_connect_clean_up();
		    printtext(&ptext_ctx, "Reconnection aborted!");
		    goto out_of_both_loops;
		}
	    }
	}

      out_of_both_loops:

	reconnect_end();
    }
}

void
set_ssl_on(void)
{
    secure_connection = true;
}

void
set_ssl_off(void)
{
    secure_connection = false;
}

bool
ssl_is_enabled(void)
{
    return secure_connection;
}

/* usage: /connect [-tls] <server[:port]> */
void
cmd_connect(const char *data)
{
    char *dcopy = sw_strdup(data);
    char *server, *port;
    char *state = "";
    int feeds_written = 0;

    set_ssl_off();

    if (strings_match(dcopy, "") || is_whiteSpace(dcopy)) {
	print_and_free("/connect: missing arguments", dcopy);
	return;
    } else if ((feeds_written = strFeed(dcopy, 1)) == 1) {
	char *token;

	token = strtok_r(dcopy, "\n:", &state);
	sw_assert(token != NULL);
	if (strings_match(token, "-tls") || strings_match(token, "-ssl"))
	    set_ssl_on();

	server = strtok_r(NULL, "\n:", &state);
	sw_assert(server != NULL);

	if ((port = strtok_r(NULL, "\n:", &state)) == NULL) {
	    if (ssl_is_enabled())
		port = SSL_PORT;
	    else
		port = IRC_PORT;
	}
    } else if (feeds_written == 0) {
	server = strtok_r(dcopy, "\n:", &state);
	sw_assert(server != NULL);

	if ((port = strtok_r(NULL, "\n:", &state)) == NULL)
	    port = IRC_PORT;
    } else {
	sw_assert_not_reached();
    }

    if (atomic_load_bool(&g_connection_in_progress)) {
	print_and_free("/connect: connection in progress", dcopy);
	return;
    } else if (reconnecting) {
	print_and_free("/connect: reconnecting... /disconnect ?", dcopy);
	return;
    } else if (g_on_air) {
	print_and_free("/connect: already connected!", dcopy);
	return;
    } else if (strtok_r(NULL, "\n:", &state) != NULL) {
	print_and_free("/connect: implicit trailing data", dcopy);
	return;
    } else if (!is_valid_hostname(server)) {
	print_and_free("/connect: bogus server name", dcopy);
	return;
    } else if (!is_numeric(port)) {
	print_and_free("/connect: bogus port number", dcopy);
	return;
    } else {
	if (strings_match_ignore_case(server, "afternet"))
	    IRC_CONNECT(get_server(afternet_servers, "AfterNET IRC network"), port);
	else if (strings_match_ignore_case(server, "anonops"))
	    IRC_CONNECT(get_server(anonops_servers, "AnonOps IRC network"), port);
	else if (strings_match_ignore_case(server, "blitzed"))
	    IRC_CONNECT(get_server(blitzed_servers, "Blitzed IRC network"), port);
	else if (strings_match_ignore_case(server, "efnet"))
	    IRC_CONNECT(get_server(efnet_servers, "EFnet servers"), port);
	else if (strings_match_ignore_case(server, "freenode"))
	    IRC_CONNECT(get_server(freenode_servers, "freenode servers"), port);
	else if (strings_match_ignore_case(server, "ircnet"))
	    IRC_CONNECT(get_server(ircnet_servers, "IRCnet servers"), port);
	else if (strings_match_ignore_case(server, "quakenet"))
	    IRC_CONNECT(get_server(quakenet_servers, "QuakeNet servers"), port);
	else if (strings_match_ignore_case(server, "undernet"))
	    IRC_CONNECT(get_server(undernet_servers, "undernet servers"), port);
	else if (strings_match_ignore_case(server, "test"))
	    IRC_CONNECT(get_server(test_servers, "test servers"), port);
	else
	    IRC_CONNECT(server, port);
	free(dcopy);
    }
}

/* usage: /disconnect [message] */
void
cmd_disconnect(const char *data)
{
    const bool has_message = !strings_match(data, "");
    extern void event_welcome_signalit(void);

    quit_reconnecting = true;

    if (g_on_air) {
	if (has_message)
	    net_send("QUIT :%s", data);
	else
	    net_send("QUIT :%s", Config("quit_message"));

	g_on_air = false;

	if (atomic_load_bool(&g_connection_in_progress))
	    event_welcome_signalit();
    }
}
