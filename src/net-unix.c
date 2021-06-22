/* Networking for UNIX
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

#include <sys/select.h>
#include <sys/socket.h>

#include <pthread.h>
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

int g_socket = -1;

static pthread_t listen_thread_id;

int
net_recv_plain(struct network_recv_context *ctx,
	       char *recvbuf, int recvbuf_size)
{
    const int      maxfdp1 = ctx->sock + 1;
    fd_set         readset;
    int            bytes_received;
    struct timeval tv = {
	.tv_sec  = ctx->sec,
	.tv_usec = ctx->microsec,
    };

    FD_ZERO(&readset);
    FD_SET(ctx->sock, &readset);

    errno = 0;

    if (select(maxfdp1, &readset, NULL, NULL, &tv) == -1)
	return (errno == EINTR ? 0 : -1);
    else if (!FD_ISSET(ctx->sock, &readset)) /* No data to recv() */
	return 0;
    else if (bytes_received = recv(ctx->sock,recvbuf,recvbuf_size,ctx->flags),
	     bytes_received == -1)
	return (errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR ? 0 : -1);
    else if (bytes_received == 0) /* Disconnected! */
	return -1;
    return bytes_received;
}

int
net_send_plain(const char *fmt, ...)
{
    char *buffer = NULL;
    int n_sent = -1;
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

    errno = 0;
    if ((n_sent = send(g_socket, buffer, strlen(buffer), 0)) == -1) {
	free_and_null(&buffer);
	return (errno == EAGAIN || errno == EWOULDBLOCK ? 0 : -1);
    }

    free_and_null(&buffer);
    return n_sent;
}

/*lint -sem(do_connect_wrapper, r_null) */
static void *
do_connect_wrapper(void *arg)
{
    struct server *server = arg;

    do_connect(server->host, server->port, server->pass);
    server_destroy(server);
    return NULL;
}

void
net_do_connect_detached(const char *host, const char *port, const char *pass)
{
    pthread_t thread;
    struct server *server = server_new(host, port, pass);

    if ((errno = pthread_create(&thread,NULL,do_connect_wrapper,server)) != 0)
	err_sys("net_do_connect_detached: pthread_create");
    else if ((errno = pthread_detach(thread)) != 0)
	err_sys("net_do_connect_detached: pthread_detach");
}

void
net_listen_thread_join(void)
{
    if ((errno = pthread_join(listen_thread_id, NULL)) != 0)
	err_sys("net_listen_thread_join: pthread_join");
}

/*lint -sem(listen_thread_fn, r_null) */
static void *
listen_thread_fn(void *arg)
{
    bool connection_lost;

    (void) arg;

    net_irc_listen(&connection_lost);

    if (!g_disconnect_wanted && g_io_loop && connection_lost) {
	net_do_connect_detached(g_last_server, g_last_port,
	    !strings_match(g_last_pass, "") ? g_last_pass : NULL);
    }

    g_disconnect_wanted = false;
    return (NULL);
}

void
net_spawn_listen_thread(void)
{
    if (errno = pthread_create(&listen_thread_id, NULL, listen_thread_fn, NULL),
	errno != 0)
	err_sys("net_spawn_listen_thread: pthread_create");
    else if ((errno = pthread_detach(listen_thread_id)) != 0)
	err_sys("net_spawn_listen_thread: pthread_detach");
}

/* ---------------------------------------------------------------------- */

void
net_set_recv_timeout(const time_t seconds)
{
	struct timeval tv = {
		.tv_sec = seconds,
		.tv_usec = 0,
	};

	errno = 0;

	if (setsockopt(g_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) != 0)
		err_log(errno, "net_set_recv_timeout: setsockopt");
}

void
net_set_send_timeout(const time_t seconds)
{
	struct timeval tv = {
		.tv_sec = seconds,
		.tv_usec = 0,
	};

	errno = 0;

	if (setsockopt(g_socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv) != 0)
		err_log(errno, "net_set_send_timeout: setsockopt");
}
