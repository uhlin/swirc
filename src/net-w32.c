/* Networking for WIN32
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

#include <process.h>
#include <string.h>

#include "assertAPI.h"
#include "errHand.h"
#include "irc.h"
#include "libUtils.h"
#include "network.h"
#include "printtext.h"
#include "strdup_printf.h"

SOCKET g_socket = INVALID_SOCKET;

static uintptr_t listenThread_id;

static void __cdecl
listenThread_fn(void *arg)
{
    struct network_recv_context ctx;
    int        bytes_received;
    const int  recvbuf_size   = 2048;
    char      *recvbuf        = xcalloc(recvbuf_size, 1);
    char      *message_concat = NULL;
    enum message_concat_state state = CONCAT_BUFFER_IS_EMPTY;
    struct printtext_context ptext_ctx;

    (void) arg;

    ctx.sock     = g_socket;
    ctx.flags    = 0;
    ctx.sec      = 5;
    ctx.microsec = 0;

    irc_init();

    do {
	BZERO(recvbuf, recvbuf_size);
	if ((bytes_received = net_recv(&ctx, recvbuf, recvbuf_size)) == -1) {
	    goto out;
	} else if (bytes_received > 0) {
	    irc_handle_interpret_events(recvbuf, &message_concat, &state);
	} else {
	    /*empty*/;
	}
    } while (g_on_air);

  out:
    ptext_ctx.window     = g_active_window;
    ptext_ctx.spec_type  = TYPE_SPEC1;
    ptext_ctx.include_ts = true;
    if (g_on_air) {
	printtext(&ptext_ctx, "Connection to IRC server lost");
	g_on_air = false;
    }
    printtext(&ptext_ctx, "Disconnected");
    (void) closesocket(g_socket);
    (void) winsock_deinit();
    irc_deinit();
    free_not_null(recvbuf);
    free_not_null(message_concat);
}

bool
winsock_init(void)
{
    WORD    ver_req = MAKEWORD(2, 2);
    WSADATA wsa;

    return (WSAStartup(ver_req, &wsa) == 0 ? true : false);
}

bool
winsock_deinit(void)
{
    return (WSACleanup() == 0 ? true : false);
}

int
net_send(SOCKET sock, int flags, const char *fmt, ...)
{
    va_list ap;
    char *buffer;
    const char message_terminate[] = "\r\n";
    int n_sent;

    if (!fmt) {
	err_exit(EINVAL, "net_send error");
    } else if (*fmt == '\0') {
	return (0); /* nothing sent */
    } else {
	va_start(ap, fmt);
	buffer = Strdup_vprintf(fmt, ap);
	va_end(ap);

	realloc_strcat(&buffer, message_terminate);

	if ((n_sent = send(sock, buffer, strlen(buffer), flags)) == SOCKET_ERROR) {
	    free_and_null(&buffer);
	    return (WSAGetLastError() == WSAEWOULDBLOCK ? 0 : -1);
	}

	free_and_null(&buffer);
	return (n_sent);
    }

    /*NOTREACHED*/
    sw_assert_not_reached();
    return (-1);
}

int
net_recv(struct network_recv_context *ctx, char *recvbuf, int recvbuf_size)
{
    fd_set readset;
    struct timeval tv;
    int bytes_received;

    FD_ZERO(&readset);
    FD_SET(ctx->sock, &readset);

    tv.tv_sec  = ctx->sec;
    tv.tv_usec = ctx->microsec;

    if (select(-1, &readset, NULL, NULL, &tv) == SOCKET_ERROR) {
	return (-1);
    } else if (!FD_ISSET(ctx->sock, &readset)) { /* No data to recv() */
	return (0);
    } else if ((bytes_received = recv(ctx->sock, recvbuf, recvbuf_size, ctx->flags)) == SOCKET_ERROR) {
	return (WSAGetLastError() == WSAEWOULDBLOCK ? 0 : -1);
    } else if (bytes_received == 0) { /* Connection gracefully closed */
	return (-1);
    } else {
	return (bytes_received);
    }

    /*NOTREACHED*/
    sw_assert_not_reached();
    return (-1);
}

void
net_spawn_listenThread(void)
{
    const uintptr_t unsuccessful = (uintptr_t) -1L;

    if ((listenThread_id = _beginthread(listenThread_fn, 0, NULL)) == unsuccessful)
	err_sys("_beginthread error");
}

void
net_listenThread_join(void)
{
    (void) WaitForSingleObject((HANDLE) listenThread_id, 10000);
}
