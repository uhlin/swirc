/* Platform independent networking routines
   Copyright (C) 2014-2018 Markus Uhlin. All rights reserved.

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

#include <string.h>

#include "config.h"
#include "errHand.h"
#include "irc.h"
#include "libUtils.h"
#include "network.h"
#include "printtext.h"
#include "strHand.h"

#include "commands/connect.h"
#include "events/cap.h"
#include "events/welcome.h"

NET_SEND_FN net_send = net_send_plain;
NET_RECV_FN net_recv = net_recv_plain;

volatile bool g_connection_in_progress = false;
volatile bool g_on_air = false;

static const int RECVBUF_SIZE = 2048;

bool
is_sasl_enabled(void)
{
    return config_bool_unparse("sasl", false);
}

struct addrinfo *
net_addr_resolve(const char *host, const char *port)
{
    struct addrinfo hints = {
	.ai_flags     = AI_CANONNAME,
	.ai_family    = AF_INET,
	.ai_socktype  = SOCK_STREAM,
	.ai_protocol  = 0,
	.ai_addrlen   = 0,
	.ai_addr      = NULL,
	.ai_canonname = NULL,
	.ai_next      = NULL,
    };
    struct addrinfo *res = NULL;

    if (!host || !port || getaddrinfo(host, port, &hints, &res) != 0) {
	return (NULL);
    }

    return (res);
}

static void
select_send_and_recv_funcs()
{
    if (is_ssl_enabled()) {
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

    if (config_bool_unparse("ircv3_server_time", false)) {
	if (net_send("CAP REQ :server-time") > 0)
	    printtext(&ptext_ctx, "Requesting server time");
    }

    if (is_sasl_enabled()) {
	if (strings_match(get_sasl_mechanism(), "PLAIN") && !is_ssl_enabled()) {
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

void
net_connect(const struct network_connect_context *ctx)
{
    PRINTTEXT_CONTEXT ptext_ctx;
    struct addrinfo *res = NULL, *rp = NULL;

    if (ctx == NULL)
	err_exit(EINVAL, "net_connect");
    g_connection_in_progress = true;
    printtext_context_init(&ptext_ctx, g_status_window, TYPE_SPEC1, true);
    printtext(&ptext_ctx, "Connecting to %s (%s)", ctx->server, ctx->port);
    ptext_ctx.spec_type = TYPE_SPEC1_SUCCESS;

#ifdef WIN32
    if (!winsock_init()) {
	ptext_ctx.spec_type = TYPE_SPEC1_FAILURE;
	printtext(&ptext_ctx, "Cannot initiate use of the Winsock DLL");
	goto out;
    } else {
	printtext(&ptext_ctx, "Use of the Winsock DLL granted");
    }
#endif

    if ((res = net_addr_resolve(ctx->server, ctx->port)) == NULL) {
	ptext_ctx.spec_type = TYPE_SPEC1_FAILURE;
	printtext(&ptext_ctx, "Unable to get a list of IP addresses");
#ifdef WIN32
	(void) winsock_deinit();
#endif
	goto out;
    } else {
	printtext(&ptext_ctx, "Get a list of IP addresses complete");
    }

    for (rp = res; rp; rp = rp->ai_next) {
#ifdef UNIX
#define INVALID_SOCKET -1
#endif
	if ((g_socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol))
	    == INVALID_SOCKET) {
	    continue;
	} else if (connect(g_socket, rp->ai_addr, rp->ai_addrlen) == 0) {
	    printtext(&ptext_ctx, "Connected!");
	    g_on_air = true;
	    break;
	} else {
#if defined(UNIX)
	    (void) close(g_socket);
#elif defined(WIN32)
	    (void) closesocket(g_socket);
#endif
	}
    }

    freeaddrinfo(res);
    select_send_and_recv_funcs();

    if (!g_on_air || (is_ssl_enabled() && net_ssl_start() == -1)) {
	ptext_ctx.spec_type = TYPE_SPEC1_FAILURE;
	printtext(&ptext_ctx, "Failed to establish a connection");
	g_on_air = false;
#ifdef WIN32
	(void) winsock_deinit();
#endif
	goto out;
    }

    if (is_ssl_enabled() && config_bool_unparse("hostname_checking", true)) {
	if (net_ssl_check_hostname(ctx->server, 0) != OK) {
	    ptext_ctx.spec_type = TYPE_SPEC1_FAILURE;
	    printtext(&ptext_ctx, "Hostname checking failed!");
	    g_on_air = false;
#ifdef WIN32
	    (void) winsock_deinit();
#endif
	    goto out;
	} else {
	    printtext(&ptext_ctx, "Hostname checking OK!");
	}
    }

    event_welcome_cond_init();
    net_spawn_listenThread();
    send_reg_cmds(ctx);

    if (!event_welcome_is_signaled()) {
	event_welcome_cond_destroy();
	ptext_ctx.spec_type = TYPE_SPEC1_FAILURE;
	printtext(&ptext_ctx, "Event welcome not signaled! "
	    "(connection_timeout=%s)", Config("connection_timeout"));
	printtext(&ptext_ctx, "Disconnecting...");
	g_on_air = false;
	net_listenThread_join(); /* wait for thread termination */
	goto out;
    }

    event_welcome_cond_destroy();

  out:
    g_connection_in_progress = false;
}

void
net_irc_listen(void)
{
    PRINTTEXT_CONTEXT ptext_ctx;
    char *message_concat = NULL;
    char *recvbuf = xcalloc(RECVBUF_SIZE, 1);
    enum message_concat_state state = CONCAT_BUFFER_IS_EMPTY;
    int bytes_received = -1;
    struct network_recv_context ctx = {
	.sock     = g_socket,
	.flags    = 0,
	.sec      = 5,
	.microsec = 0,
    };

    printtext_context_init(&ptext_ctx, NULL, TYPE_SPEC1_WARN, true);
    irc_init();

    do {
	BZERO(recvbuf, RECVBUF_SIZE);
	if ((bytes_received = net_recv(&ctx, recvbuf, RECVBUF_SIZE-1)) == -1) {
	    goto out;
	} else if (bytes_received > 0) {
	    irc_handle_interpret_events(recvbuf, &message_concat, &state);
	} else {
	    /*empty*/;
	}
    } while (g_on_air);

  out:
    ptext_ctx.window = g_active_window;
    if (g_on_air) {
	printtext(&ptext_ctx, "Connection to IRC server lost");
	g_on_air = false;
    }
    printtext(&ptext_ctx, "Disconnected");
    net_ssl_close();
#if defined(UNIX)
    close(g_socket);
#elif defined(WIN32)
    closesocket(g_socket);
    winsock_deinit();
#endif
    irc_deinit();
    free_not_null(recvbuf);
    free_not_null(message_concat);
}
