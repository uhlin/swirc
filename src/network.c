/* Platform independent networking routines
   Copyright (C) 2014, 2016 Markus Uhlin. All rights reserved.

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

   THIS SOFTWARE IS PROVIDED THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS
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

#include "config.h"
#include "irc.h"
#include "network.h"
#include "printtext.h"

#include "events/welcome.h"

NET_SEND_FN net_send = net_send_plain;
NET_RECV_FN net_recv = net_recv_plain;

bool g_connection_in_progress = false;
volatile bool g_on_air = false;

struct addrinfo *
net_addr_resolve(const char *host, const char *port)
{
    struct addrinfo hints;
    struct addrinfo *res;

    hints.ai_flags     = AI_CANONNAME;
    hints.ai_family    = AF_UNSPEC;
    hints.ai_socktype  = SOCK_STREAM;
    hints.ai_protocol  = 0;
    hints.ai_addrlen   = 0;
    hints.ai_addr      = NULL;
    hints.ai_canonname = NULL;
    hints.ai_next      = NULL;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
	return (NULL);
    }

    return (res);
}

void
net_connect(const struct network_connect_context *ctx)
{
    struct printtext_context ptext_ctx = {
	.window     = g_status_window,
	.spec_type  = TYPE_SPEC1,
	.include_ts = true,
    };
    struct addrinfo *res, *rp;

    g_connection_in_progress = true;

    printtext(&ptext_ctx, "Connecting to %s (%s)", ctx->server, ctx->port);
    ptext_ctx.spec_type  = TYPE_SPEC1_SUCCESS;

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
#if defined(UNIX)
	if ((g_socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
	    continue;
	}
#elif defined(WIN32)
	if ((g_socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == INVALID_SOCKET) {
	    continue;
	}
#endif
	else if (connect(g_socket, rp->ai_addr, rp->ai_addrlen) == 0) {
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
    if (!g_on_air) {
	ptext_ctx.spec_type = TYPE_SPEC1_FAILURE;
	printtext(&ptext_ctx, "Failed to establish a connection");
#ifdef WIN32
	(void) winsock_deinit();
#endif
	goto out;
    }

    event_welcome_cond_init();
    net_spawn_listenThread();

    if (ctx->password) {
	(void) net_send("PASS %s", ctx->password);
    }

    (void) net_send("NICK %s", ctx->nickname);
    (void) net_send("USER %s 8 * :%s", ctx->username, ctx->rl_name);

    if (!event_welcome_is_signaled()) {
	event_welcome_cond_destroy();

	ptext_ctx.spec_type = TYPE_SPEC1_FAILURE;
	printtext(&ptext_ctx, "Event welcome not signaled! (connection_timeout=%s)",
		  Config("connection_timeout"));
	printtext(&ptext_ctx, "Disconnecting...");
	g_on_air = false;
	net_listenThread_join(); /* wait for thread termination */
	goto out;
    }

    event_welcome_cond_destroy();

  out:
    g_connection_in_progress = false;
}
