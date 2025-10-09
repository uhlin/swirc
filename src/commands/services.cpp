/* Communicate with IRC services
   Copyright (C) 2016-2025 Markus Uhlin. All rights reserved.

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

#include <openssl/crypto.h> /* OPENSSL_cleanse() */

#include <stdexcept>
#include <string>

#include "../config.h"
#include "../dataClassify.h"
#include "../errHand.h"
#include "../main.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "connect.h"
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
	explicit irc_service_cmd(CSTRING);

	const std::string &
	get_srv_host(void) const
	{
		return this->srv_host;
	}

	CSTRING
	get_msg(void) const
	{
		return this->msg.c_str();
	}
};

irc_service_cmd::irc_service_cmd(CSTRING data)
{
	CSTRING	 token[2];
	STRING	 dcopy;
	char	*last = const_cast<char *>("");

	if (strings_match(data, ""))
		throw std::runtime_error("no data");

	dcopy = sw_strdup(data);
	(void) strFeed(dcopy, 1);

	if ((token[0] = strtok_r(dcopy, "\n", &last)) != NULL)
		(void) this->srv_host.assign(token[0]);
	if ((token[1] = strtok_r(NULL, "\n", &last)) != NULL)
		(void) this->msg.assign(token[1]);
	free(dcopy);

	if (token[0] == NULL || token[1] == NULL)
		throw std::runtime_error("too few tokens");
}

static void
run_command(CSTRING slashcmd, CSTRING srv_name, CSTRING host_setting,
    CSTRING data)
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
		printtext_print("err", "%s: %s", slashcmd, e.what());

		if (strings_match(e.what(), "cannot send")) {
			err_log(ENOTCONN, "%s", slashcmd);
			(void) atomic_swap_bool(&g_connection_lost, true);
		}
	}
}

/*
 * usage: /chanserv <[service hostname | --]> <command> [...]
 */
void
cmd_chanserv(CSTRING data)
{
	run_command("/chanserv", "ChanServ", "chanserv_host", data);
}

/*
 * usage: /nickserv <[service hostname | --]> <command> [...]
 */
void
cmd_nickserv(CSTRING data)
{
	run_command("/nickserv", "NickServ", "nickserv_host", data);
}

/*
 * usage: /nsid [--force] [--swap] [--pass-only]
 */
void
cmd_nsid(CSTRING data)
{
	CSTRING str[2];
	bool	o_force = false;
	bool	o_swap = false;
	bool	o_pass_only = false;
	char	buf[800];
	int	ret;

	if (!strings_match(data, "")) {
		CSTRING token;
		STRING dcopy = sw_strdup(data);
		STRING last = const_cast<STRING>("");

		for (STRING args = dcopy;
		    (token = strtok_r(args, " ", &last)) != NULL;
		    args = NULL) {
			if (strings_match(token, "--force"))
				o_force = true;
			else if (strings_match(token, "--swap"))
				o_swap = true;
			else if (strings_match(token, "--pass-only"))
				o_pass_only = true;
			else {
				printtext_print("warn", "%s: implicit data: %s",
				    __func__, token);
				free(dcopy);
				return;
			}
		}

		free(dcopy);
	}

	if (o_swap && o_pass_only) {
		printtext_print("err", "%s: --swap cannot be combined with "
		    "--pass-only", __func__);
		return;
	}

	if (!ssl_is_enabled() && !o_force) {
		printtext_print("err",
		    "%s: communication are done in plain text: "
		    "use --force to override", __func__);
		return;
	} else if ((str[0] = config_get_normalized_sasl_username()) == NULL ||
		   (str[1] = config_get_normalized_sasl_password()) == NULL) {
		printtext_print("err", "%s: unable to get username/password",
		    __func__);
		return;
	}

	if (o_swap) {
		ret = snprintf(buf, sizeof buf, "-- identify %s %s", str[1],
		    str[0]);
	} else if (o_pass_only) {
		ret = snprintf(buf, sizeof buf, "-- identify %s", str[1]);
	} else {
		ret = snprintf(buf, sizeof buf, "-- identify %s %s", str[0],
		    str[1]);
	}

	if (ret < 0 || static_cast<size_t>(ret) >= sizeof buf) {
		printtext_print("err", "%s: insufficient buffer space",
		    __func__);
		return;
	}

	run_command("/nickserv", "NickServ", "nickserv_host", &buf[0]);
	OPENSSL_cleanse(buf, sizeof buf);
}

/*
 * usage: /qbot <[service hostname | --]> <command> [...]
 */
void
cmd_qbot(CSTRING data)
{
	run_command("/qbot", "Q", "qbot_host", data);
}

static PTEXTBUF
get_list(CSTRING search_var, stringarray_t array, const size_t size)
{
	PTEXTBUF	matches = textBuf_new();
	const size_t	varlen = strlen(search_var);

	for (size_t i = 0; i < size; i++) {
		CSTRING cmd = array[i];

		if (!strncmp(search_var, cmd, varlen))
			textBuf_emplace_back(__func__, matches, cmd, 0);
	}

	return matches;
}

PTEXTBUF
get_list_of_matching_cs_cmds(CSTRING search_var)
{
	PTEXTBUF matches = get_list(search_var, cs_cmds, ARRAY_SIZE(cs_cmds));

	if (textBuf_size(matches) == 0) {
		textBuf_destroy(matches);
		return NULL;
	}

	return matches;
}

PTEXTBUF
get_list_of_matching_ns_cmds(CSTRING search_var)
{
	PTEXTBUF matches = get_list(search_var, ns_cmds, ARRAY_SIZE(ns_cmds));

	if (textBuf_size(matches) == 0) {
		textBuf_destroy(matches);
		return NULL;
	}

	return matches;
}
