/* TLS server (Unix specific functions)
   Copyright (C) 2021 Markus Uhlin. All rights reserved.

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

#include <pthread.h>

#include "assertAPI.h"
#include "atomicops.h"
#include "errHand.h"
#include "tls-server.h"

static void *
accept_thread(void *arg)
{
	const int port = *(static_cast<int *>(arg));

	tls_server_accept_new_connections(port);
	tls_server_exit_thread();

	/* NOTREACHED */
	return NULL;
}

static void *
com_with_client(void *arg)
{
	SSL *ssl = static_cast<SSL *>(arg);

	tls_server_enter_loop(ssl);
	SSL_free(ssl);
	return NULL;
}

void
tls_server_begin(const int port)
{
	pthread_t	tid;
	static int	i;

	i = port;

	if ((errno = pthread_create(&tid, NULL, accept_thread, &i)) != 0)
		err_sys("tls_server_begin: pthread_create");
	else if ((errno = pthread_detach(tid)) != 0)
		err_sys("tls_server_begin: pthread_detach");
}

void
tls_server_end(void)
{
	(void) atomic_swap_bool(&g_accepting_new_connections, false);
	(void) atomic_swap_bool(&g_tls_server_loop, false);
}

void
tls_server_com_with_client(SSL *ssl)
{
	pthread_t tid;

	if ((errno = pthread_create(&tid, NULL, com_with_client, ssl)) != 0)
		err_sys("tls_server_com_with_client: pthread_create");
	else if ((errno = pthread_detach(tid)) != 0)
		err_sys("tls_server_com_with_client: pthread_detach");
}

NORETURN void
tls_server_exit_thread(void)
{
	int dummy = 0;

	pthread_exit(&dummy);
	sw_assert_not_reached();
}
