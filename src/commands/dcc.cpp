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
#include "../dataClassify.h"
#include "../errHand.h"
#include "../filePred.h"
#include "../nestHome.h"
#include "../printtext.h"
#include "../sig.h"
#include "../strHand.h"
#include "../tls-server.h"

#include "dcc.h"

static bool
subcmd_ok(const char *subcmd)
{
	if (strings_match(subcmd, "close"))
		return true;
	else if (strings_match(subcmd, "get"))
		return true;
	else if (strings_match(subcmd, "list"))
		return true;
	else if (strings_match(subcmd, "send"))
		return true;
	return false;
}

static void
subcmd_close()
{
}

static void
subcmd_get()
{
}

static void
subcmd_list()
{
}

static void
subcmd_send(const char *nick, const char *file)
{
	if (nick == nullptr || file == nullptr) {
		printtext_print("err", "insufficient args");
		return;
	} else if (!is_valid_nickname(nick)) {
		printtext_print("err", "invalid nickname");
		return;
	}
}

/*
 * usage: /dcc [close|get|list|send] [args]
 */
void
cmd_dcc(const char *data)
{
	char *dcopy;
	char *last = const_cast<char *>("");
	char *subcmd, *arg1, *arg2;
	static const char cmd[] = "/dcc";
	static const char sep[] = "\n";

	if (strings_match(data, "")) {
		printtext_print("err", "insufficient args");
		return;
	}

	dcopy = sw_strdup(data);
	(void) strFeed(dcopy, 2);

	if ((subcmd = strtok_r(dcopy, sep, &last)) == nullptr) {
		printf_and_free(dcopy, "%s: insufficient args", cmd);
		return;
	} else if (!subcmd_ok(subcmd)) {
		printf_and_free(dcopy, "%s: invalid subcommand '%s'", cmd,
		    subcmd);
		return;
	}

	arg1 = strtok_r(nullptr, sep, &last);
	arg2 = strtok_r(nullptr, sep, &last);

	if (strings_match(subcmd, "close"))
		subcmd_close();
	else if (strings_match(subcmd, "get"))
		subcmd_get();
	else if (strings_match(subcmd, "list"))
		subcmd_list();
	else if (strings_match(subcmd, "send"))
		subcmd_send(arg1, arg2);
	else
		printtext_print("err", "%s: invalid subcommand", cmd);
	free(dcopy);
}

void
dcc::init(void)
{
	if (config_bool("dcc", true)) {
		struct integer_context intctx("dcc_port", 1024, 65535, 8080);

		tls_server::begin(config_integer(&intctx));
	}
}

void
dcc::deinit(void)
{
	tls_server::end();
}

const char *
dcc::get_upload_dir(void)
{
	static const char *dir;

	dir = Config("dcc_upload_dir");

	if (!is_directory(dir))
		return (g_dcc_upload_dir ? g_dcc_upload_dir : "");
	return dir;
}

void
dcc::handle_incoming_conn(SSL *ssl)
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

bool
dcc::want_unveil_uploads(void)
{
	const char *dir = Config("dcc_upload_dir");

	if (strings_match(dir, ""))
		return false;
	else if (strncmp(dir, g_home_dir, strlen(g_home_dir)) == 0)
		return false;
	return true;
}
