/* Connect and Disconnect commands
   Copyright (C) 2016-2022 Markus Uhlin. All rights reserved.

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

#include "../irc.h"
#include "../events/welcome.h"

#include "../assertAPI.h"
#include "../config.h"
#include "../curses-funcs.h"
#include "../dataClassify.h"
#include "../io-loop.h"
#include "../libUtils.h"
#include "../main.h"
#include "../nestHome.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "connect.h"

bool	g_disconnect_wanted = false;

static bool	quit_reconnecting = false;
static bool	reconnecting = false;
static bool	secure_connection = false;

static IRC_SERVER afternet_servers[] = {
	{ "irc.afternet.org", "6667", "Default PLAIN" },
	{ "irc.afternet.org", "6697", "Default TLS" },
	{ NULL,               NULL,   NULL },
};

static IRC_SERVER alphachat_servers[] = {
	{ "irc.alphachat.net", "6667", "Default PLAIN" },
	{ "irc.alphachat.net", "6697", "Default TLS" },
	{ NULL,                NULL,   NULL },
};

static IRC_SERVER anonops_servers[] = {
	{ "irc.anonops.com", "6697", "Main server" },
	{ NULL,              NULL,   NULL },
};

static IRC_SERVER efnet_servers[] = {
	{ "efnet.port80.se",   "6697", "TLS" },
	{ "irc.efnet.nl",      "6697", "TLS" },
	{ "irc.efnet.org",     "6667", "PLAIN" },
	{ "irc.mzima.net",     "6697", "TLS" },
	{ "irc.underworld.no", "6697", "TLS" },
	{ NULL,                NULL,   NULL },
};

static IRC_SERVER freenode_servers[] = {
	{ "chat.freenode.net", "6667", "Default PLAIN" },
	{ "chat.freenode.net", "6697", "Default TLS" },
	{ NULL,                NULL,   NULL },
};

static IRC_SERVER ircnet_servers[] = {
	{ "open.ircnet.io",       "6667", "All open servers" },
	{ "ssl.ircnet.ovh",       "6697", "SSL/TLS support" },
	{ "irc.cloak.ircnet.ovh", "6667", "Cloak support" },
	{ "ssl.cloak.ircnet.ovh", "6697", "Cloak and SSL/TLS support" },

	{ "eu.irc6.net",    "6667", "IPv6 PLAIN" },
	{ "irc.cs.hut.fi",  "6667", "Helsinki Univ of Tech, CS Lab IRC server" },
	{ "irc.psychz.net", "6667", "US Open Server, Provided by Psychz Networks" },
	{ "irc.swipnet.se", "8080", "Tele2/SWIPNET" },
	{ NULL,             NULL,   NULL },
};

static IRC_SERVER ircnow_servers[] = {
	{ "irc.bsdforall.org",   "6697", "IPv4 TLS" },
	{ "irc.ircforever.org",  "6697", "IPv4 TLS" },
	{ "irc.ircnow.org",      "6667", "IPv4 PLAIN" },
	{ "irc.ircnow.org",      "6697", "IPv4 TLS" },
	{ "irc.nastycode.com",   "6697", "IPv4 TLS" },
	{ "irc.shelltalk.net",   "6697", "IPv4 TLS" },
	{ "irc.thunderirc.net",  "6697", "IPv4 TLS" },
	{ "irc6.bsdforall.org",  "6697", "IPv6 TLS" },
	{ "irc6.ircforever.org", "6697", "IPv6 TLS" },
	{ "irc6.ircnow.org",     "6667", "IPv6 PLAIN" },
	{ "irc6.ircnow.org",     "6697", "IPv6 TLS" },
	{ "irc6.nastycode.com",  "6697", "IPv6 TLS" },
	{ "irc6.shelltalk.net",  "6697", "IPv6 TLS" },
	{ "irc6.thunderirc.net", "6697", "IPv6 TLS" },
	{ NULL,                  NULL,   NULL },
};

static IRC_SERVER libera_servers[] = {
	{ "irc.libera.chat",      "6667", "Default PLAIN" },
	{ "irc.libera.chat",      "6697", "Default TLS" },
	{ "irc.eu.libera.chat",   "6697", "Europe" },
	{ "irc.us.libera.chat",   "6697", "US & Canada" },
	{ "irc.au.libera.chat",   "6697", "Australia and New Zealand" },
	{ "irc.ea.libera.chat",   "6697", "East Asia"},
	{ "irc.ipv4.libera.chat", "8000", "IPv4 PLAIN" },
	{ "irc.ipv4.libera.chat", "6697", "IPv4 TLS" },
	{ "irc.ipv6.libera.chat", "8000", "IPv6 PLAIN" },
	{ "irc.ipv6.libera.chat", "6697", "IPv6 TLS" },
	{ NULL,                   NULL,   NULL },
};

static IRC_SERVER quakenet_servers[] = {
	{ "adrift.sg.quakenet.org",      "6667", "Adrift" },
	{ "datapacket.hk.quakenet.org",  "6667", "DataPacket" },
	{ "euroserv.fr.quakenet.org",    "6667", "Euroserv" },
	{ "hostsailor.ro.quakenet.org",  "6667", "HostSailor" },
	{ "irc.ipv6.quakenet.org",       "6667", "IPv6 PLAIN" },
	{ "port80c.se.quakenet.org",     "6667", "" },
	{ "stockholm.se.quakenet.org",   "6667", "Stockholm" },
	{ "underworld2.no.quakenet.org", "6667", "Underworld" },
	{ NULL,                          NULL,   NULL },
};

static IRC_SERVER undernet_servers[] = {
	{ "irc.undernet.org",  "6667", "IPv4 WorldWide" },
	{ "irc6.undernet.org", "6667", "IPv6 WorldWide" },
	{ NULL,                NULL,   NULL },
};

static IRC_SERVER test_servers[] = {
	{ "default.icb.net",       "7326", "Chime ICB server" },
	{ "internetcitizens.band", "7326", "Fuchsschwanz ICB server PLAIN" },
	{ "internetcitizens.band", "7327", "Fuchsschwanz ICB server TLS" },
	{ "testnet.ergo.chat",     "6667", "Testnet for the Ergo IRC server PLAIN" },
	{ "testnet.ergo.chat",     "6697", "Testnet for the Ergo IRC server TLS" },
	{ "testnet.inspircd.org",  "6667", "InspIRCd Test Network PLAIN" },
	{ "testnet.inspircd.org",  "6697", "InspIRCd Test Network TLS" },
	{ NULL,                    NULL,   NULL },
};

static bool
shouldConnectUsingPassword(void)
{
	char answer[20] = { '\0' };

	while (true) {
		printf("Connect using password? [Y/n]: ");
		fflush(stdout);

		if (fgets(answer, sizeof answer, stdin) == NULL) {
			;
		} else if (strchr(answer, '\n') == NULL) {
			int c;

			puts("input too big");

			while (c = getchar(), c != '\n' && c != EOF)
				/* discard */;
		} else if (strings_match(trim(answer), "") ||
		    strings_match(answer, "y") || strings_match(answer, "Y")) {
			break;
		} else if (strings_match(answer, "n") ||
		    strings_match(answer, "N")) {
			return false;
		} else {
			;
		}
	}

	return true;
}

/*lint -sem(get_password, r_null) */
static char *
get_password(void)
{
	int c = EOF;
	static char pass[400] = { '\0' };

	escape_curses();

	if (!shouldConnectUsingPassword()) {
		resume_curses();
		return NULL;
	}

	while (true) {
		(void) printf("Password (will echo): ");
		(void) fflush(stdout);

		const bool fgets_error =
		    (fgets(pass, ARRAY_SIZE(pass), stdin) == NULL);

		if (fgets_error) {
			(void) putchar('\n');
			continue;
		} else if (strchr(pass, '\n') == NULL) {
			(void) puts("input too big");

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

static PIRC_SERVER
get_server_v2(PIRC_SERVER ptr, const size_t size, const char *hdr)
{
	PIRC_SERVER srv;
	char ans[20] = { '\0' };
	int i, srvno;

	escape_curses();
	puts(hdr);
	srv = ptr;
	i = 0;
	while (srv->host != NULL) {
		printf("    (%d) %-30s %s %s\n", i, srv->host, srv->port,
		    srv->desc);
		srv++;
		i++;
	}
	if (i > 0)
		i -= 1;
	srvno = 0;
	while (true) {
		printf("Your choice (0-%d): ", i);
		fflush(stdout);

/*
 * sscanf() is safe in this context
 */
#if WIN32
#pragma warning(disable: 4996)
#endif
		if (fgets(ans, sizeof ans, stdin) == NULL) {
			;
		} else if (strchr(ans, '\n') == NULL) {
			int c;

			puts("input too big");

			while (c = getchar(), c != '\n' && c != EOF)
				/* discard */;
		} else if (sscanf(ans, "%d", &srvno) != 1 || srvno < 0 ||
		    srvno > i) {
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
	} /* while */

	UNUSED_PARAM(size);
	resume_curses();
	return ptr + srvno;
}

static void
reconnect_begin(void)
{
	quit_reconnecting = false;
	reconnecting = true;
}

static void
reconnect_end(void)
{
	reconnecting = false;
}

static void
turn_icb_mode_on(void)
{
	g_icb_mode = true;
}

static void
turn_icb_mode_off(void)
{
	g_icb_mode = false;
}

static bool
assign_username(char **cp)
{
	static char	*username;

	if (g_cmdline_opts->username) {
		*cp = g_cmdline_opts->username;
		return true;
	} else if ((username = Config_mod("username")) != NULL &&
	    !strings_match(username, "")) {
		*cp = username;
		return true;
	} else if ((username = g_user) != NULL &&
	    !strings_match(username, "")) {
		*cp = username;
		return true;
	}
	return false;
}

static bool
assign_rl_name(char **cp)
{
	static char	*rl_name;

	if (g_cmdline_opts->rl_name) {
		*cp = g_cmdline_opts->rl_name;
		return true;
	} else if ((rl_name = Config_mod("real_name")) != NULL &&
	    !strings_match(rl_name, "")) {
		*cp = rl_name;
		return true;
	} else if ((rl_name = g_user) != NULL && !strings_match(rl_name, "")) {
		*cp = rl_name;
		return true;
	}
	return false;
}

static bool
assign_nickname(char **cp)
{
	static char	*nickname;

	if (g_cmdline_opts->nickname) {
		*cp = g_cmdline_opts->nickname;
		return true;
	} else if ((nickname = Config_mod("nickname")) != NULL &&
	    !strings_match(nickname, "")) {
		*cp = nickname;
		return true;
	} else if ((nickname = g_user) != NULL &&
	    !strings_match(nickname, "")) {
		*cp = nickname;
		return true;
	}
	return false;
}

void
do_connect(const char *server, const char *port, const char *pass)
{
	PRINTTEXT_CONTEXT ptext_ctx;
	struct network_connect_context conn_ctx = {
		.server   = ((char *) server),
		.port     = ((char *) port),
		.password = ((char *) pass),
		.username = "",
		.rl_name  = "",
		.nickname = "",
	};

	if (atomic_load_bool(&g_connection_in_progress) ||
	    g_disconnect_wanted || !g_io_loop || g_on_air)
		return;

	printtext_context_init(&ptext_ctx, g_status_window, TYPE_SPEC1_FAILURE,
	    true);

	if (!assign_username(&conn_ctx.username)) {
		printtext(&ptext_ctx, "Unable to connect: No username");
		return;
	} else if (!assign_rl_name(&conn_ctx.rl_name)) {
		printtext(&ptext_ctx, "Unable to connect: No real name");
		return;
	} else if (!assign_nickname(&conn_ctx.nickname)) {
		printtext(&ptext_ctx, "Unable to connect: No nickname");
		return;
	}

	if (!is_valid_username(conn_ctx.username)) {
		printtext(&ptext_ctx, "Unable to connect: Invalid username: "
		    "\"%s\"", conn_ctx.username);
		printtext(&ptext_ctx, "Change with /set username");
		return;
	} else if (!is_valid_real_name(conn_ctx.rl_name)) {
		printtext(&ptext_ctx, "Unable to connect: Invalid real name: "
		    "\"%s\"", conn_ctx.rl_name);
		printtext(&ptext_ctx, "Change with /set real_name");
		return;
	} else if (!is_valid_nickname(conn_ctx.nickname)) {
		printtext(&ptext_ctx, "Unable to connect: Invalid nickname: "
		    "\"%s\"", conn_ctx.nickname);
		printtext(&ptext_ctx, "Change with /set nickname");
		return;
	} else {
		/* The value of sleep_time_seconds assigned below
		 * isn't intent to be used. net_connect() is
		 * responsible for giving it an initial value as well
		 * as changing/updating it between reconnect attempts
		 * to reflect the delay. */
		long int	sleep_time_seconds = 10;

		if (strings_match(conn_ctx.port, ICB_PORT) ||
		    strings_match(conn_ctx.port, ICB_SSL_PORT))
			turn_icb_mode_on();
		else if (strings_match(conn_ctx.port, IRC_PORT) ||
		    strings_match(conn_ctx.port, IRC_SSL_PORT))
			turn_icb_mode_off();

		if (strings_match(conn_ctx.port, ICB_SSL_PORT) ||
		    strings_match(conn_ctx.port, IRC_SSL_PORT))
			set_ssl_on();

		reconnect_begin();
		ptext_ctx.spec_type = TYPE_SPEC2;

		while (net_connect(&conn_ctx, &sleep_time_seconds) ==
		    SHOULD_RETRY_TO_CONNECT) {
			printtext(&ptext_ctx, "Next reconnect attempt in %ld "
			    "seconds...", sleep_time_seconds);

			for (long int secs = 0;
			    secs < sleep_time_seconds;
			    secs++) {
				(void) napms(1000);

				if (quit_reconnecting) {
					net_kill_connection();
					net_connect_clean_up();
					printtext(&ptext_ctx, "Reconnection "
					    "aborted!");
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
	char	*dcopy = sw_strdup(data);
	char	*server, *port;
	char	*state = "";
	int	 feeds_written = 0;

	set_ssl_off();

	if (strings_match(dcopy, "") || is_whiteSpace(dcopy)) {
		print_and_free("/connect: missing arguments", dcopy);
		return;
	} else if ((feeds_written = strFeed(dcopy, 1)) == 1) {
		char *token;

		token = strtok_r(dcopy, "\n:", &state);
		sw_assert(token != NULL);

		if (strings_match(token, "-tls") ||
		    strings_match(token, "-ssl"))
			set_ssl_on();

		server = strtok_r(NULL, "\n:", &state);
		sw_assert(server != NULL);

		if ((port = strtok_r(NULL, "\n:", &state)) == NULL) {
			if (ssl_is_enabled())
				port = IRC_SSL_PORT;
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
		print_and_free("/connect: reconnecting... /disconnect ?",
		    dcopy);
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
		static PIRC_SERVER srvptr;

		g_disconnect_wanted = false;
		g_connection_lost = g_on_air = false;

		if (strings_match_ignore_case(server, "afternet")) {
			srvptr = get_server_v2(&afternet_servers[0],
			    ARRAY_SIZE(afternet_servers), "AfterNET IRC network"
			    " - www.afternet.org");
			IRC_CONNECT(srvptr->host, srvptr->port);
		} else if (strings_match_ignore_case(server, "alphachat")) {
			srvptr = get_server_v2(&alphachat_servers[0],
			    ARRAY_SIZE(alphachat_servers), "AlphaChat - "
			    "www.alphachat.net");
			IRC_CONNECT(srvptr->host, srvptr->port);
		} else if (strings_match_ignore_case(server, "anonops")) {
			srvptr = get_server_v2(&anonops_servers[0],
			    ARRAY_SIZE(anonops_servers), "AnonOps IRC network");
			IRC_CONNECT(srvptr->host, srvptr->port);
		} else if (strings_match_ignore_case(server, "efnet")) {
			srvptr = get_server_v2(&efnet_servers[0],
			    ARRAY_SIZE(efnet_servers), "Eris Free network");
			IRC_CONNECT(srvptr->host, srvptr->port);
		} else if (strings_match_ignore_case(server, "freenode")) {
			srvptr = get_server_v2(&freenode_servers[0],
			    ARRAY_SIZE(freenode_servers), "Freenode IRC "
			    "network");
			IRC_CONNECT(srvptr->host, srvptr->port);
		} else if (strings_match_ignore_case(server, "ircnet")) {
			srvptr = get_server_v2(&ircnet_servers[0],
			    ARRAY_SIZE(ircnet_servers), "IRCnet servers");
			IRC_CONNECT(srvptr->host, srvptr->port);
		} else if (strings_match_ignore_case(server, "ircnow")) {
			srvptr = get_server_v2(&ircnow_servers[0],
			    ARRAY_SIZE(ircnow_servers), "IRCNow: "
			    "The Users' Network");
			IRC_CONNECT(srvptr->host, srvptr->port);
		} else if (strings_match_ignore_case(server, "libera")) {
			srvptr = get_server_v2(&libera_servers[0],
			    ARRAY_SIZE(libera_servers), "Libera Chat");
			IRC_CONNECT(srvptr->host, srvptr->port);
		} else if (strings_match_ignore_case(server, "quakenet")) {
			srvptr = get_server_v2(&quakenet_servers[0],
			    ARRAY_SIZE(quakenet_servers), "QuakeNet IRC "
			    "network");
			IRC_CONNECT(srvptr->host, srvptr->port);
		} else if (strings_match_ignore_case(server, "undernet")) {
			srvptr = get_server_v2(&undernet_servers[0],
			    ARRAY_SIZE(undernet_servers), "Undernet IRC "
			    "Network");
			IRC_CONNECT(srvptr->host, srvptr->port);
		} else if (strings_match_ignore_case(server, "test")) {
			srvptr = get_server_v2(&test_servers[0],
			    ARRAY_SIZE(test_servers), "Test servers; "
			    "both ICB and IRC");
			IRC_CONNECT(srvptr->host, srvptr->port);
		} else {
			IRC_CONNECT(server, port);
		}

		free(dcopy);
	}
}

/* usage: /disconnect [message] */
void
cmd_disconnect(const char *data)
{
	const bool	has_message = !strings_match(data, "");

	quit_reconnecting = true;

	if (g_on_air) {
		g_disconnect_wanted = true;
		g_connection_lost = g_on_air = false;

		if (has_message)
			(void) net_send("QUIT :%s", data);
		else
			(void) net_send("QUIT :%s", Config("quit_message"));

		if (atomic_load_bool(&g_connection_in_progress))
			event_welcome_signalit();

		while (atomic_load_bool(&g_irc_listening))
			(void) napms(1);
	}
}
