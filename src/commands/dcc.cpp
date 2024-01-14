/* commands/dcc.cpp
   Copyright (C) 2024 Markus Uhlin. All rights reserved.

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

#include "../config.h"
#include "../errHand.h"
#include "../sig.h"
#include "../tls-server.h"

#include "dcc.h"

void
cmd_dcc(const char *data)
{
	UNUSED_PARAM(data);
}

void
dcc_init(void)
{
	if (config_bool("dcc", true)) {
		struct integer_context intctx("dcc_port", 1024, 65535, 8080);

		tls_server::begin(config_integer(&intctx));
	}
}

void
dcc_deinit(void)
{
	tls_server::end();
}

void
dcc_handle_incoming_conn(SSL *ssl)
{
	block_signals();

	switch (SSL_accept(ssl)) {
	case 0:
		debug("%s: SSL_accept: The TLS/SSL handshake was not "
		    "successful", __func__);
		return;
	case 1:
		debug("%s: SSL_accept: The TLS/SSL handshake was successfully "
		    "completed", __func__);
		break;
	default:
		debug("%s: SSL_accept: The TLS/SSL handshake was not "
		    "successful because a fatal error occurred", __func__);
		return;
	}
}
