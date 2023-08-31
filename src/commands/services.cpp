/* Communicate with IRC services
   Copyright (C) 2016-2023 Markus Uhlin. All rights reserved.

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

#include <stdexcept>
#include <string>

#include "../config.h"
#include "../dataClassify.h"
#include "../errHand.h"
#include "../main.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "services.h"
#include "squery.h"

static stringarray_t cs_cmds = {
//lint -e786
	cs_cmds_macro(--),
//lint +e786
};

static stringarray_t ns_cmds = {
//lint -e786
	ns_cmds_macro(--),
//lint +e786
};

class irc_service_cmd {
	std::string	srv_host;
	std::string	msg;

public:
	explicit irc_service_cmd(const char *);

	const std::string &
	get_srv_host(void) const
	{
		return this->srv_host;
	}

	const char *
	get_msg(void) const
	{
		return this->msg.c_str();
	}
};

irc_service_cmd::irc_service_cmd(const char *data)
{
	char *dcopy;
	char *last = const_cast<char *>("");
	char *token1, *token2;

	if (strings_match(data, ""))
		throw std::runtime_error("no data");

	dcopy = sw_strdup(data);
	(void) strFeed(dcopy, 1);

	if ((token1 = strtok_r(dcopy, "\n", &last)) != NULL)
		(void) this->srv_host.assign(token1);
	if ((token2 = strtok_r(NULL, "\n", &last)) != NULL)
		(void) this->msg.assign(token2);
	free(dcopy);

	if (token1 == NULL || token2 == NULL)
		throw std::runtime_error("too few tokens");
}

static void
run_command(const char *slashcmd, const char *srv_name,
    const char *host_setting, const char *data)
{
	try {
		irc_service_cmd sc(data);

		if (sc.get_srv_host().compare("--") == 0) {
			if (!is_valid_hostname(Config(host_setting)))
				throw std::runtime_error("invalid host");
			else if (net_send("PRIVMSG %s@%s :%s", srv_name,
			    Config(host_setting), sc.get_msg()) < 0)
				throw std::runtime_error("cannot send");
		} else {
			if (!is_valid_hostname(sc.get_srv_host().c_str()))
				throw std::runtime_error("invalid host");
			else if (net_send("PRIVMSG %s@%s :%s", srv_name,
			    sc.get_srv_host().c_str(), sc.get_msg()) < 0)
				throw std::runtime_error("cannot send");
		}
	} catch (const std::runtime_error &e) {
		std::string str(slashcmd);

		(void) str.append(": ").append(e.what());
		printtext_print("err", "%s", str.c_str());

		if (strings_match(e.what(), "cannot send")) {
			err_log(ENOTCONN, "%s", slashcmd);
			g_connection_lost = true;
		}
	}
}

/*
 * usage: /chanserv <[service hostname | --]> <command> [...]
 */
void
cmd_chanserv(const char *data)
{
	run_command("/chanserv", "ChanServ", "chanserv_host", data);
}

/*
 * usage: /nickserv <[service hostname | --]> <command> [...]
 */
void
cmd_nickserv(const char *data)
{
	run_command("/nickserv", "NickServ", "nickserv_host", data);
}

/*
 * usage: /qbot <[service hostname | --]> <command> [...]
 */
void
cmd_qbot(const char *data)
{
	run_command("/qbot", "Q", "qbot_host", data);
}

static void
add_cmd(PTEXTBUF matches, const char *str)
{
	if (textBuf_size(matches) != 0) {
		if ((errno = textBuf_ins_next(matches, textBuf_tail(matches),
		    str, -1)) != 0)
			err_sys("%s: textBuf_ins_next", __func__);
	} else {
		if ((errno = textBuf_ins_next(matches, NULL, str, -1)) != 0)
			err_sys("%s: textBuf_ins_next", __func__);
	}
}

static PTEXTBUF
get_list(const char *search_var, stringarray_t array, const size_t size)
{
	PTEXTBUF matches = textBuf_new();

	for (size_t i = 0; i < size; i++) {
		const char *cmd = array[i];

		if (!strncmp(search_var, cmd, strlen(search_var)))
			add_cmd(matches, cmd);
	}

	return matches;
}

PTEXTBUF
get_list_of_matching_cs_cmds(const char *search_var)
{
	PTEXTBUF matches;

	matches = get_list(search_var, cs_cmds, ARRAY_SIZE(cs_cmds));

	if (textBuf_size(matches) == 0) {
		textBuf_destroy(matches);
		return NULL;
	}

	return matches;
}

PTEXTBUF
get_list_of_matching_ns_cmds(const char *search_var)
{
	PTEXTBUF matches;

	matches = get_list(search_var, ns_cmds, ARRAY_SIZE(ns_cmds));

	if (textBuf_size(matches) == 0) {
		textBuf_destroy(matches);
		return NULL;
	}

	return matches;
}
