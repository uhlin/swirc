/* identd-w32.cpp
   Copyright (C) 2022 Markus Uhlin. All rights reserved.

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
#include "errHand.h"
#include "identd.hpp"
#include "tls-server.h" /* 'g_beginthread_failed' */

typedef void __cdecl VoidCdecl;

static VoidCdecl
accept_thread(void *arg)
{
	const int port = *(static_cast<int *>(arg));

	identd::listen_on_port(port);
	identd::exit_thread();
}

static VoidCdecl
com_thread(void *arg)
{
	ident_client *cli = static_cast<ident_client *>(arg);

	identd::enter_loop(cli);
	delete cli;
	identd::exit_thread();
}

void
identd::com_with_client(ident_client *cli)
{
	if (_beginthread(com_thread, 0, cli) == g_beginthread_failed)
		err_sys("%s: _beginthread", __func__);
}

NORETURN void
identd::exit_thread(void)
{
	_endthread();
	sw_assert_not_reached();
}

void
identd::set_reuseaddr(SOCKET sock)
{
	BOOL val = TRUE;

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>
	    (&val), sizeof(BOOL)) != 0) {
		err_log(0, "%s: setsockopt error (code = %d)", __func__,
		    WSAGetLastError());
	}
}

void
identd::start(const int port)
{
	static int i;

	i = port;

	if (_beginthread(accept_thread, 0, &i) == g_beginthread_failed)
		err_sys("%s: _beginthread", __func__);
}
