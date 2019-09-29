/* Platform independent networking routines
   Copyright (C) 2014-2019 Markus Uhlin. All rights reserved.

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

#ifdef UNIX
#include <sys/socket.h>
#include <sys/types.h>

#include <netdb.h>
#include <unistd.h> /* close() */
#endif

#include <stdexcept>
#include <string.h>

#include "config.h"
#include "errHand.h"
#include "icb.h"
#include "irc.h"
#include "libUtils.h"
#include "main.h"
#include "network.h"
#include "printtext.h"
#include "strHand.h"

#include "commands/connect.h"
#include "events/cap.h"
#include "events/welcome.h"

#ifdef UNIX
#define INVALID_SOCKET -1
#endif

/****************************************************************
*                                                               *
*  ------------------ Structure definitions ------------------  *
*                                                               *
****************************************************************/

struct reconnect_context {
    long int backoff_delay;
    long int delay;
    long int delay_max;
    long int retries;

    reconnect_context() {
	this->backoff_delay = 0;
	this->delay         = 0;
	this->delay_max     = 0;
	this->retries       = 0;
    }
};

/****************************************************************
*                                                               *
*  -------------- Objects with external linkage --------------  *
*                                                               *
****************************************************************/

/*
 * net_send_fake() store the sent data into this buffer
 */
char g_sent[512] = "";

NET_SEND_FN net_send = net_send_plain;
NET_RECV_FN net_recv = net_recv_plain;

volatile bool g_connection_in_progress = false;
volatile bool g_on_air = false;

char g_last_server[512] = { 0 };
char g_last_port[16] = { 0 };

/****************************************************************
*                                                               *
*  -------------- Objects with internal linkage --------------  *
*                                                               *
****************************************************************/

static const int RECVBUF_SIZE = 2048;
static long int retry = 0;
static struct reconnect_context reconn_ctx;

/****************************************************************
*                                                               *
*  ---------------------    Functions    ---------------------  *
*                                                               *
****************************************************************/

static void
reconnect_context_reinit(struct reconnect_context *ctx)
{
    if (ctx) {
	ctx->backoff_delay = get_reconnect_backoff_delay();
	ctx->delay         = get_reconnect_delay();
	ctx->delay_max     = get_reconnect_delay_max();
	ctx->retries       = get_reconnect_retries();
    }
}

static void
select_send_and_recv_funcs()
{
    if (ssl_is_enabled()) {
	net_send = net_ssl_send;
	net_recv = net_ssl_recv;
    } else {
	net_send = net_send_plain;
	net_recv = net_recv_plain;
    }
}

static PTR_ARGS_NONNULL void
send_reg_cmds(const struct network_connect_context *ctx)
{
    PRINTTEXT_CONTEXT ptext_ctx;

    printtext_context_init(&ptext_ctx, g_status_window, TYPE_SPEC1_SUCCESS,
	true);

    if (config_bool_unparse("account_notify", false)) {
	if (net_send("CAP REQ :account-notify") > 0)
	    printtext(&ptext_ctx, "Requesting account notify");
    }

    if (config_bool_unparse("away_notify", false)) {
	if (net_send("CAP REQ :away-notify") > 0)
	    printtext(&ptext_ctx, "Requesting away notify");
    }

    if (config_bool_unparse("invite_notify", false)) {
	if (net_send("CAP REQ :invite-notify") > 0)
	    printtext(&ptext_ctx, "Requesting invite notify");
    }

    if (config_bool_unparse("ircv3_server_time", false)) {
	if (net_send("CAP REQ :server-time") > 0)
	    printtext(&ptext_ctx, "Requesting server time");
    }

    if (sasl_is_enabled()) {
	if (strings_match(get_sasl_mechanism(), "PLAIN") && !ssl_is_enabled()) {
	    ptext_ctx.spec_type = TYPE_SPEC1_WARN;
	    printtext(&ptext_ctx, "SASL mechanism matches PLAIN and TLS/SSL "
		"is not enabled. Not requesting SASL authentication.");
	} else {
	    if (net_send("CAP REQ :sasl") > 0)
		printtext(&ptext_ctx, "Requesting SASL authentication");
	}
    }

    if (ctx->password) {
	(void) net_send("PASS %s", ctx->password);
    }

    (void) net_send("NICK %s", ctx->nickname);
    (void) net_send("USER %s 8 * :%s", ctx->username, ctx->rl_name);
}

/*
 * ICB Login Packet
 * ----------------
 * (Fields: Minimum: 5, Maximum: 7)
 *
 * Field 0: Login id of user. Required.
 * Field 1: Nickname to use upon login into ICB. Required.
 * Field 2: Default group to log into in ICB, or do group who of. A null string
 *          for who listing will show all groups. Required.
 * Field 3: Login command. Required. Currently one of the following:
 * - "login" log into ICB
 * - "w" just show who is currently logged into ICB
 * Field 4: Password to authenticate the user to ICB. Required, but often blank.
 * Field 5: If when logging in, default group (field 2) does not exist, create
 *          it with this status. Optional.
 * Field 6: Protocol level. Optional. Deprecated.
 *
 * Thus the ICB Login Packet has the following layout:
 * aLoginid^ANickname^ADefaultGroup^ACommand^APass^AGroupStatus^AProtocolLevel
 */
static PTR_ARGS_NONNULL void
send_icb_login_packet(const struct network_connect_context *ctx)
{
    char *packet = sw_strdup(" ");

    realloc_strcat(&packet, "a");
    realloc_strcat(&packet, ctx->username);
    realloc_strcat(&packet, ICB_FIELD_SEP);

    irc_set_my_nickname(ctx->nickname);
    realloc_strcat(&packet, ctx->nickname);
    realloc_strcat(&packet, ICB_FIELD_SEP);
    realloc_strcat(&packet, ICB_FIELD_SEP);

    realloc_strcat(&packet, "login");
    realloc_strcat(&packet, ICB_FIELD_SEP);

    realloc_strcat(&packet, ctx->password ? ctx->password : "");
    realloc_strcat(&packet, ICB_FIELD_SEP);
    realloc_strcat(&packet, ICB_FIELD_SEP);
    realloc_strcat(&packet, ICB_FIELD_SEP);

    packet[0] = (char) strlen(&packet[1]);
    net_send("%s", packet);
    free_not_null(packet);
}

bool
sasl_is_enabled(void)
{
    return config_bool_unparse("sasl", false);
}

conn_res_t
net_connect(
    const struct network_connect_context *ctx,
    long int *sleep_time_seconds)
{
    PRINTTEXT_CONTEXT ptext_ctx;
    static bool reconn_initialized = false;
    struct addrinfo *res = NULL, *rp = NULL;

    if (ctx == NULL || sleep_time_seconds == NULL)
	err_exit(EINVAL, "net_connect");
    else if (atomic_load_bool(&g_connection_in_progress))
	return CONNECTION_FAILED;

    (void) atomic_swap_bool(&g_connection_in_progress, true);

    if (!atomic_load_bool(&reconn_initialized)) {
	reconn_ctx.backoff_delay = get_reconnect_backoff_delay();
	reconn_ctx.delay         = get_reconnect_delay();
	reconn_ctx.delay_max     = get_reconnect_delay_max();
	reconn_ctx.retries       = get_reconnect_retries();

	(void) atomic_swap_bool(&reconn_initialized, true);
    }

    printtext_context_init(&ptext_ctx, g_status_window, TYPE_SPEC1, true);
    printtext(&ptext_ctx, "Connecting to %s (%s)", ctx->server, ctx->port);

    try {
	ptext_ctx.spec_type = TYPE_SPEC1_SUCCESS;

#ifdef WIN32
	if (!winsock_init())
	    throw std::runtime_error("Cannot initiate use of the Winsock DLL");
	else
	    printtext(&ptext_ctx, "Use of the Winsock DLL granted");
#endif

	if ((res = net_addr_resolve(ctx->server, ctx->port)) == NULL)
	    throw std::runtime_error("Unable to get a list of IP addresses");
	else
	    printtext(&ptext_ctx, "Get a list of IP addresses complete");

	for (rp = res; rp; rp = rp->ai_next) {
	    if ((g_socket = socket(rp->ai_family,
				   rp->ai_socktype,
				   rp->ai_protocol)) == INVALID_SOCKET)
		continue;

	    net_set_recv_timeout(TEMP_RECV_TIMEOUT);
	    net_set_send_timeout(TEMP_SEND_TIMEOUT);

	    if (connect(g_socket, rp->ai_addr, rp->ai_addrlen) == 0) {
		printtext(&ptext_ctx, "Connected!");
		g_on_air = true;
		net_set_recv_timeout(DEFAULT_RECV_TIMEOUT);
		net_set_send_timeout(DEFAULT_SEND_TIMEOUT);
		break;
	    } else {
		CLOSE_GLOBAL_SOCKET();
	    }
	} /* for */

	if (res)
	    freeaddrinfo(res);
	select_send_and_recv_funcs();

	if (!g_on_air || (ssl_is_enabled() && net_ssl_begin() == -1))
	    throw std::runtime_error("Failed to establish a connection");
	if (ssl_is_enabled() && config_bool_unparse("hostname_checking", true))
	    {
		if (net_ssl_check_hostname(ctx->server, 0) != OK)
		    throw std::runtime_error("Hostname checking failed!");
		else
		    printtext(&ptext_ctx, "Hostname checking OK!");
	    }
	event_welcome_cond_init();
	net_spawn_listenThread();
	if (g_icb_mode)
	    send_icb_login_packet(ctx);
	else
	    send_reg_cmds(ctx);

	if (!event_welcome_is_signaled()) {
	    event_welcome_cond_destroy();
	    throw std::runtime_error("Event welcome not signaled!");
	}

	event_welcome_cond_destroy();
    } catch (std::runtime_error &e) {
	ptext_ctx.spec_type = TYPE_SPEC1_FAILURE;
	printtext(&ptext_ctx, "%s", e.what());
	net_kill_connection();

	if (retry++ < reconn_ctx.retries) {
	    const bool is_initial_reconnect_attempt = (retry == 1);

	    if (is_initial_reconnect_attempt)
		*sleep_time_seconds = reconn_ctx.delay;
	    else
		*sleep_time_seconds += reconn_ctx.backoff_delay;

	    /* --------------------------------------------- */

	    if (*sleep_time_seconds > reconn_ctx.delay_max)
		*sleep_time_seconds = reconn_ctx.delay_max;

	    (void) atomic_swap_bool(&g_connection_in_progress, false);
	    return SHOULD_RETRY_TO_CONNECT;
	}
	net_connect_clean_up();
	return CONNECTION_FAILED;
    }

    snprintf(g_last_server, ARRAY_SIZE(g_last_server), "%s", ctx->server);
    snprintf(g_last_port, ARRAY_SIZE(g_last_port), "%s", ctx->port);
    window_foreach_rejoin_all_channels();
    net_connect_clean_up();
    return CONNECTION_ESTABLISHED;
}

int
net_send_fake(const char *fmt, ...)
{
    int bytes_sent;
    va_list ap;

    va_start(ap, fmt);
    bytes_sent = vsnprintf(g_sent, ARRAY_SIZE(g_sent), fmt, ap);
    va_end(ap);

    return bytes_sent;
}

struct addrinfo *
net_addr_resolve(const char *host, const char *port)
{
    struct addrinfo *res = NULL;
    struct addrinfo hints;

    if (host == NULL || port == NULL)
	return NULL;

    BZERO(&hints, sizeof hints);
    hints.ai_flags     = AI_CANONNAME;
    hints.ai_family    = AF_INET;
    hints.ai_socktype  = SOCK_STREAM;
    hints.ai_protocol  = 0;
    hints.ai_addrlen   = 0;
    hints.ai_addr      = NULL;
    hints.ai_canonname = NULL;
    hints.ai_next      = NULL;

    return getaddrinfo(host, port, &hints, &res) != 0 ? NULL : res;
}

struct server *
server_new(const char *host, const char *port)
{
    struct server *server = (struct server *) xmalloc(sizeof *server);

    server->host = sw_strdup(host);
    server->port = sw_strdup(port);

    return server;
}

void
net_connect_clean_up(void)
{
    reconnect_context_reinit(&reconn_ctx);
    retry = 0;
    atomic_swap_bool(&g_connection_in_progress, false);
}

void
net_irc_listen(bool *connection_lost)
{
    PRINTTEXT_CONTEXT ptext_ctx;
    char *message_concat = NULL;
    char *recvbuf = (char *) xmalloc(RECVBUF_SIZE);
    enum message_concat_state state = CONCAT_BUFFER_IS_EMPTY;
    int bytes_received = -1;
    struct network_recv_context ctx(g_socket, 0, 5, 0);

    *connection_lost = false;
    irc_init();

    do {
	BZERO(recvbuf, RECVBUF_SIZE);

	if ((bytes_received = net_recv(&ctx, recvbuf, RECVBUF_SIZE-1)) == -1) {
	    break;
	} else if (bytes_received > 0) {
	    if (g_icb_mode)
		icb_irc_proxy(recvbuf[0], recvbuf[1], &recvbuf[2]);
	    else
		irc_handle_interpret_events(recvbuf, &message_concat, &state);
	}
    } while (g_on_air);

    printtext_context_init(&ptext_ctx, g_active_window, TYPE_SPEC1_WARN, true);

    if (g_on_air) {
	*connection_lost = true;
	printtext(&ptext_ctx, "Connection to IRC server lost");
    }

    net_kill_connection();
    irc_deinit();
    free_not_null(recvbuf);
    free_not_null(message_concat);
    printtext(&ptext_ctx, "Disconnected");
}

void
net_kill_connection(void)
{
    g_on_air = false;
    net_ssl_end();
#if defined(UNIX)
    if (g_socket != INVALID_SOCKET)
	close(g_socket);
#elif defined(WIN32)
    if (g_socket != INVALID_SOCKET)
	closesocket(g_socket);
    winsock_deinit();
#endif
    g_socket = INVALID_SOCKET;
}

void
server_destroy(struct server *server)
{
    if (server == NULL)
	return;
    free_not_null(server->host);
    free_not_null(server->port);
    free(server);
}
