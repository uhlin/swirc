/* connect and disconnect commands
   Copyright (C) 2016-2018 Markus Uhlin. All rights reserved.

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
#include "../errHand.h"
#include "../libUtils.h"
#include "../main.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "connect.h"

#define SSL_PORT "6697"

static bool secure_connection = false;

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
};

static const char *freenode_servers[] = {
    "card.freenode.net",
    "chat.freenode.net",
    "cherryh.freenode.net",
    "freenodeok2gncmy.onion",
    "hobana.freenode.net",
    "leguin.acc.umu.se",
    "orwell.freenode.net",
    "verne.freenode.net",
    "weber.freenode.net",
};

static const char *ircnet_servers[] = {
    "eris.us.ircnet.net",
    "irc.atw-inter.net",
    "irc.nlnog.net",
    "irc.portlane.se",
    "open.ircnet.net",
};

static const char *quakenet_servers[] = {
    "ServerCentral.IL.US.Quakenet.Org",
    "irc.quakenet.org",
    "portlane.se.quakenet.org",
    "underworld1.no.quakenet.org",
    "underworld2.no.quakenet.org",
};

static const char *undernet_servers[] = {
    "Ashburn.Va.Us.UnderNet.org",
    "irc.undernet.org",
    "undernet.rethemhosting.net",
};

/*lint -sem(get_password, r_null) */
static char *
get_password()
{
    char  answer[20] = "";
    int   c          = EOF;
    char *pass       = NULL;

    escape_curses();

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
	    resume_curses();
	    return NULL;
	} else {
	    continue;
	}
    }

#define PASSWORD_SIZE 100
    pass = xmalloc(PASSWORD_SIZE);

    while (true) {
	printf("Password (will echo): ");
	fflush(stdout);

	const bool fgets_error = fgets(pass, PASSWORD_SIZE, stdin) == NULL;

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
    return (pass);
}

void
do_connect(const char *server, const char *port)
{
    PRINTTEXT_CONTEXT ptext_ctx;
    struct network_connect_context conn_ctx = {
	.server   = (char *) server,
	.port     = (char *) port,
	.password = NULL,
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
	conn_ctx.password = (g_connection_password ? get_password() : NULL);

	if (!is_ssl_enabled() && strings_match(conn_ctx.port, SSL_PORT))
	    set_ssl_on();

	net_connect(&conn_ctx);

	if (conn_ctx.password) {
	    OPENSSL_cleanse(conn_ctx.password, PASSWORD_SIZE);
	    free(conn_ctx.password);
	    conn_ctx.password = NULL;
	}
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
is_ssl_enabled(void)
{
    return secure_connection;
}

static const char *
get_server(const char *ar[], const size_t ar_sz, const char *msg)
{
    char ans[20] = "";
    const char **ppcc = &ar[0];
    int c = EOF;
    int i = 0;
    int srvno = 0;

    escape_curses();
    puts(msg);

    while (ppcc < &ar[ar_sz]) {
	printf("    (%d) %s\n", i, *ppcc);
	ppcc++, i++;
    }

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

	if ((port = strtok_r(NULL, "\n:", &state)) == NULL)
	    port = is_ssl_enabled() ? SSL_PORT : "6667";
    } else if (feeds_written == 0) {
	server = strtok_r(dcopy, "\n:", &state);
	sw_assert(server != NULL);

	if ((port = strtok_r(NULL, "\n:", &state)) == NULL)
	    port = "6667";
    } else {
	sw_assert_not_reached();
    }

    if (g_connection_in_progress) {
	print_and_free("/connect: connection in progress", dcopy);
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
	if (strings_match_ignore_case(server, "efnet"))
	    do_connect(get_server(efnet_servers,
		ARRAY_SIZE(efnet_servers), "EFnet servers"), port);
	else if (strings_match_ignore_case(server, "freenode"))
	    do_connect(get_server(freenode_servers,
		ARRAY_SIZE(freenode_servers), "freenode servers"), port);
	else if (strings_match_ignore_case(server, "ircnet"))
	    do_connect(get_server(ircnet_servers,
		ARRAY_SIZE(ircnet_servers), "IRCnet servers"), port);
	else if (strings_match_ignore_case(server, "quakenet"))
	    do_connect(get_server(quakenet_servers,
		ARRAY_SIZE(quakenet_servers), "QuakeNet servers"), port);
	else if (strings_match_ignore_case(server, "undernet"))
	    do_connect(get_server(undernet_servers,
		ARRAY_SIZE(undernet_servers), "undernet servers"), port);
	else
	    do_connect(server, port);
	free(dcopy);
    }
}

/* usage: /disconnect [message] */
void
cmd_disconnect(const char *data)
{
    const bool has_message = !strings_match(data, "");

    if (g_on_air) {
	if (has_message)
	    net_send("QUIT :%s", data);
	else
	    net_send("QUIT :%s", Config("quit_message"));
	g_on_air = false;
	net_listenThread_join();
    }
}
