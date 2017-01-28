/* Networking for WIN32
   Copyright (C) 2014-2017 Markus Uhlin. All rights reserved.

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
#include "libUtils.h"
#include "network.h"
#include "strdup_printf.h"

SOCKET g_socket = INVALID_SOCKET;

static uintptr_t listenThread_id;

static void __cdecl
listenThread_fn(void *arg)
{
    (void) arg;
    net_irc_listen();
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
net_send_plain(const char *fmt, ...)
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

	if ((n_sent = send(g_socket, buffer, strlen(buffer), 0)) == SOCKET_ERROR) {
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
net_recv_plain(struct network_recv_context *ctx, char *recvbuf, int recvbuf_size)
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
