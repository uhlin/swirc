/* commands/dcc-w32.cpp
   Copyright (C) 2024-2026 Markus Uhlin. All rights reserved.

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

#include "../assertAPI.h"
#include "../errHand.h"
#include "../tls-server.h"

#include "../events/welcome-w32.h" /* dword_product() */

#include "dcc.h"

typedef void __cdecl VoidCdecl;

static VoidCdecl
dcc_getit(void *arg)
{
	auto obj = static_cast<dcc_get *>(arg);

	obj->get_file();
	dcc::exit_thread();
}

NORETURN void
dcc::exit_thread(void)
{
	_endthread();
	sw_assert_not_reached();
}

void
dcc::get_file_detached(dcc_get *obj)
{
	if (_beginthread(dcc_getit, 0, obj) == g_beginthread_failed)
		err_sys("%s: _beginthread", __func__);
}

void
dcc::set_recv_timeout(SOCKET sock, const DWORD seconds)
{
	const DWORD	timeout_milliseconds = dword_product(seconds, 1000);
	const int	optlen = static_cast<int>(sizeof(DWORD));

	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast
	    <const char *>(&timeout_milliseconds), optlen) != 0)
		err_log(0, "%s: setsockopt", __func__);
}
