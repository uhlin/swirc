/* TLS server (Win32 specific functions)
   Copyright (C) 2021-2024 Markus Uhlin. All rights reserved.

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

#include "assertAPI.h"
#include "atomicops.h"
#include "errHand.h"
#include "tls-server.h"

#include "commands/dcc.h"

typedef void __cdecl VoidCdecl;

const uintptr_t g_beginthread_failed = static_cast<uintptr_t>(-1L);

static VoidCdecl
accept_thread(void *arg)
{
	const int port = *(static_cast<int *>(arg));

	tls_server::accept_new_connections(port);
	tls_server::exit_thread();
}

static VoidCdecl
com_thread(void *arg)
{
	SSL *ssl = static_cast<SSL *>(arg);

	dcc::handle_incoming_conn(ssl);
	SSL_free(ssl);
	_endthread();
}

void
tls_server::begin(const int port)
{
	static int i;

	i = port;

	if (_beginthread(accept_thread, 0, &i) == g_beginthread_failed)
		err_sys("%s: _beginthread", __func__);
}

void
tls_server::com_with_client(SSL *ssl)
{
	if (_beginthread(com_thread, 0, ssl) == g_beginthread_failed)
		err_sys("%s: _beginthread", __func__);
}

NORETURN void
tls_server::exit_thread(void)
{
	_endthread();
	sw_assert_not_reached();
}
