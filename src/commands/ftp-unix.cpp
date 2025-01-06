/* The FTP command  --  Unix specific functions
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

#include <sys/socket.h>

#include <pthread.h>

#include "../assertAPI.h"
#include "../errHand.h"
#include "../strHand.h"

#include "ftp.h"

static void *
cmd_doit(void *arg)
{
	STRING name = static_cast<STRING>(arg);

	free(name);
	ftp::exit_thread();

	/* NOTREACHED */
	return nullptr;
}

NORETURN void
ftp::exit_thread(void)
{
	int dummy = 0;

	pthread_exit(&dummy);
	sw_assert_not_reached();
}

void
ftp::do_cmd_detached(CSTRING cmd)
{
	pthread_t tid;

	if ((errno = pthread_create(&tid, nullptr, cmd_doit,
	    sw_strdup(cmd))) != 0)
		err_sys("%s: pthread_create", __func__);
	else if ((errno = pthread_detach(tid)) != 0)
		err_sys("%s: pthread_detach", __func__);
}

void
ftp::set_timeout(SOCKET sock, int optname, const time_t seconds)
{
	struct timeval tv = { 0 };

	if (optname != SO_RCVTIMEO &&
	    optname != SO_SNDTIMEO) {
		err_log(0, "%s: illegal option name", __func__);
		return;
	}

	tv.tv_sec = seconds;
	tv.tv_usec = 0;

	errno = 0;

	if (setsockopt(sock, SOL_SOCKET, optname, &tv, sizeof tv) != 0)
		err_log(errno, "%s: setsockopt", __func__);
}
