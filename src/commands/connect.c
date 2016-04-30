/* connect and disconnect commands
   Copyright (C) 2016 Markus Uhlin. All rights reserved.

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

   THIS SOFTWARE IS PROVIDED THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

#include "common.h"

#include "../assertAPI.h"
#include "../config.h"
#include "../dataClassify.h"
#include "../main.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "connect.h"

static bool secure_connection = false;

static struct printtext_context ptext_ctx = {
    .window	= NULL,
    .spec_type	= TYPE_SPEC1_FAILURE,
    .include_ts = true,
};

static void
do_connect(char *server, char *port)
{
    struct network_connect_context conn_ctx = {
	.server   = server,
	.port     = port,
	.password = g_connection_password ? g_cmdline_opts->password : NULL,
	.username = "",
	.rl_name  = "",
	.nickname = "",
    };

    ptext_ctx.window = g_active_window;

    if (g_cmdline_opts->username) {
	conn_ctx.username = g_cmdline_opts->username;
    } else if (Config_mod("username")) {
	conn_ctx.username = Config_mod("username");
    } else {
	printtext(&ptext_ctx, "Unable to connect: No username");
	return;
    }

    if (g_cmdline_opts->rl_name) {
	conn_ctx.rl_name = g_cmdline_opts->rl_name;
    } else if (Config_mod("real_name")) {
	conn_ctx.rl_name = Config_mod("real_name");
    } else {
	printtext(&ptext_ctx, "Unable to connect: No real name");
	return;
    }

    if (g_cmdline_opts->nickname) {
	conn_ctx.nickname = g_cmdline_opts->nickname;
    } else if (Config_mod("nickname")) {
	conn_ctx.nickname = Config_mod("nickname");
    } else {
	printtext(&ptext_ctx, "Unable to connect: No nickname");
	return;
    }

    if (!is_valid_username(conn_ctx.username)) {
	printtext(&ptext_ctx, "Unable to connect: Invalid username: \"%s\"", conn_ctx.username);
	return;
    } else if (!is_valid_real_name(conn_ctx.rl_name)) {
	printtext(&ptext_ctx, "Unable to connect: Invalid real name: \"%s\"", conn_ctx.rl_name);
	return;
    } else if (!is_valid_nickname(conn_ctx.nickname)) {
	printtext(&ptext_ctx, "Unable to connect: Invalid nickname: \"%s\"", conn_ctx.nickname);
	return;
    } else {
	net_connect(&conn_ctx);
    }
}

void
set_ssl_on()
{
    secure_connection = true;
}

void
set_ssl_off()
{
    secure_connection = false;
}

bool
is_ssl_enabled()
{
    return secure_connection;
}

/* usage: /connect [-ssl] <server[:port]> */
void
cmd_connect(const char *data)
{
    char *dcopy = sw_strdup(data);
    char *server, *port;
    char *state = "";
    int feeds_written = 0;

    set_ssl_off();
    ptext_ctx.window = g_active_window;

    if (Strings_match(dcopy, "") || is_whiteSpace(dcopy)) {
	printtext(&ptext_ctx, "/connect: missing arguments");
	free(dcopy);
	return;
    } else if ((feeds_written = Strfeed(dcopy, 1)) == 1) {
	char *token;

	token = strtok_r(dcopy, "\n:", &state);
	sw_assert(token != NULL);
	if (Strings_match(token, "-ssl"))
	    set_ssl_on();

	server = strtok_r(NULL, "\n:", &state);
	sw_assert(server != NULL);

	if ((port = strtok_r(NULL, "\n:", &state)) == NULL)
	    port = "6697";
    } else if (feeds_written == 0) {
	server = strtok_r(dcopy, "\n:", &state);
	sw_assert(server != NULL);

	if ((port = strtok_r(NULL, "\n:", &state)) == NULL)
	    port = "6667";
    } else {
	sw_assert_not_reached();
    }

    if (g_connection_in_progress) {
	printtext(&ptext_ctx, "/connect: connection in progress");
	free(dcopy);
	return;
    } else if (g_on_air) {
	printtext(&ptext_ctx, "/connect: already connected!");
	free(dcopy);
	return;
    } else if (strtok_r(NULL, "\n:", &state) != NULL) {
	printtext(&ptext_ctx, "/connect: implicit trailing data");
	free(dcopy);
	return;
    } else if (!is_valid_hostname(server)) {
	printtext(&ptext_ctx, "/connect: bogus server name");
	free(dcopy);
	return;
    } else if (!is_numeric(port)) {
	printtext(&ptext_ctx, "/connect: bogus port number");
	free(dcopy);
	return;
    } else {
	do_connect(server, port);
	free(dcopy);
    }
}

/* usage: /disconnect [message] */
void
cmd_disconnect(const char *data)
{
    const bool has_message = !Strings_match(data, "");

    if (g_on_air) {
	if (has_message)
	    net_send("QUIT :%s", data);
	else
	    net_send("QUIT :%s", Config("quit_message"));
	g_on_air = false;
	net_listenThread_join();
    }
}
