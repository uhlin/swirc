/* Networking for WIN32
   Copyright (C) 2014-2021 Markus Uhlin. All rights reserved.

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

#include <process.h>
#include <string.h>

#include "dataClassify.h"
#include "errHand.h"
#include "io-loop.h"
#include "libUtils.h"
#include "main.h"
#include "network.h"
#include "strHand.h"
#include "strdup_printf.h"

#include "commands/connect.h" /* do_connect() */

typedef void __cdecl VoidCdecl;

SOCKET g_socket = INVALID_SOCKET;

static const uintptr_t BEGINTHREAD_FAILED = (uintptr_t) -1L;
static uintptr_t listen_thread_id;

bool
winsock_deinit(void)
{
    return (WSACleanup() == 0 ? true : false);
}

bool
winsock_init(void)
{
    WORD    ver_req = MAKEWORD(2, 2);
    WSADATA wsa;

    return (WSAStartup(ver_req, &wsa) == 0 ? true : false);
}

int
net_recv_plain(struct network_recv_context *ctx,
	       char *recvbuf, int recvbuf_size)
{
    fd_set readset;
    int bytes_received = SOCKET_ERROR;
    struct timeval tv = {
	.tv_sec  = ctx->sec,
	.tv_usec = ctx->microsec,
    };

    FD_ZERO(&readset);
    FD_SET(ctx->sock, &readset);

    if (select(-1, &readset, NULL, NULL, &tv) == SOCKET_ERROR)
	return -1;
    else if (!FD_ISSET(ctx->sock, &readset)) /* No data to recv() */
	return 0;
    else if (bytes_received = recv(ctx->sock,recvbuf,recvbuf_size,ctx->flags),
	     bytes_received == SOCKET_ERROR)
	return (WSAGetLastError() == WSAEWOULDBLOCK ? 0 : -1);
    else if (bytes_received == 0) /* Connection gracefully closed */
	return -1;
    return bytes_received;
}

int
net_send_plain(const char *fmt, ...)
{
    char *buffer = NULL;
    int n_sent = SOCKET_ERROR;
    va_list ap;

    if (g_socket == INVALID_SOCKET)
	return -1;
    else if (isNull(fmt))
	err_exit(EINVAL, "net_send_plain");
    else if (isEmpty(fmt))
	return 0; /* nothing sent */

    va_start(ap, fmt);
    buffer = strdup_vprintf(fmt, ap);
    va_end(ap);

    if (!g_icb_mode)
	realloc_strcat(&buffer, "\r\n");

    if ((n_sent = send(g_socket, buffer, size_to_int(strlen(buffer)), 0)) ==
	SOCKET_ERROR) {
	free_and_null(&buffer);
	return (WSAGetLastError() == WSAEWOULDBLOCK ? 0 : -1);
    }

    free_and_null(&buffer);
    return n_sent;
}

static VoidCdecl
do_connect_wrapper(void *arg)
{
    struct server *server = arg;

    do_connect(server->host, server->port, server->pass);
    server_destroy(server);
    _endthread();
}

void
net_do_connect_detached(const char *host, const char *port, const char *pass)
{
	struct server *server = server_new(host, port, pass);

	errno = 0;

	if (_beginthread(do_connect_wrapper, 0, server) == BEGINTHREAD_FAILED)
		err_sys("net_do_connect_detached: _beginthread");
}

void
net_listen_thread_join(void)
{
	(void) WaitForSingleObject((HANDLE) listen_thread_id, 10000);
}

static VoidCdecl
listen_thread_fn(void *arg)
{
	bool connection_lost;

	(void) arg;

	net_irc_listen(&connection_lost);

	if (!g_disconnect_wanted && g_io_loop && connection_lost) {
		net_do_connect_detached(g_last_server, g_last_port,
		    (!strings_match(g_last_pass, "") ? g_last_pass : NULL));
	}

	g_disconnect_wanted = false;
	_endthread();
}

void
net_spawn_listen_thread(void)
{
	if ((listen_thread_id = _beginthread(listen_thread_fn, 0, NULL)) ==
	    BEGINTHREAD_FAILED)
		err_sys("net_spawn_listen_thread: _beginthread");
}

/* ---------------------------------------------------------------------- */

/*
 * dword_product() is defined in events/welcome-w32.c
 */
DWORD dword_product(const DWORD elt_count, const DWORD elt_size);

void
net_set_recv_timeout(const DWORD seconds)
{
	const DWORD	timeout_milliseconds = dword_product(seconds, 1000);
	const int	optlen = (int) (sizeof(DWORD));

	if (setsockopt(g_socket, SOL_SOCKET, SO_RCVTIMEO,
	    ((char *) &timeout_milliseconds), optlen) != 0)
		err_log(0, "net_set_recv_timeout: setsockopt");
}

void
net_set_send_timeout(const DWORD seconds)
{
	const DWORD	timeout_milliseconds = dword_product(seconds, 1000);
	const int	optlen = (int) (sizeof(DWORD));

	if (setsockopt(g_socket, SOL_SOCKET, SO_SNDTIMEO,
	    ((char *) &timeout_milliseconds), optlen) != 0)
		err_log(0, "net_set_send_timeout: setsockopt");
}
