/* The FTP command  --  W32 specific functions
   Copyright (C) 2025 Markus Uhlin. All rights reserved.

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
#include "../printtext.h"
#include "../sig.h"
#include "../strHand.h"
#include "../tls-server.h"

#include "../events/welcome-w32.h" /* dword_product() */

#include "atomicops.h"
#include "ftp.h"

typedef void __cdecl VoidCdecl;

static VoidCdecl
cmd_doit(void *arg)
{
	STRING name = static_cast<STRING>(arg);

	block_signals();
	SELECT_AND_RUN_CMD();
	free(name);
	ftp::exit_thread();
}

NORETURN void
ftp::exit_thread(void)
{
	_endthread();
	sw_assert_not_reached();
}

void
ftp::do_cmd_detached(CSTRING cmd)
{
	if (_beginthread(cmd_doit, 0, sw_strdup(cmd)) == g_beginthread_failed)
		err_sys("%s: _beginthread", __func__);
}

void
ftp::set_timeout(SOCKET sock, int optname, const DWORD seconds)
{
	const DWORD	timeout_milliseconds = dword_product(seconds, 1000);
	const int	optlen = static_cast<int>(sizeof(DWORD));

	if (optname != SO_RCVTIMEO &&
	    optname != SO_SNDTIMEO) {
		err_log(0, "%s: illegal option name", __func__);
		return;
	}

	if (setsockopt(sock, SOL_SOCKET, optname, reinterpret_cast<CSTRING>
	    (&timeout_milliseconds), optlen) != 0)
		err_log(0, "%s: setsockopt", __func__);
}
