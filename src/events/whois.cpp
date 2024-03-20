/* Whois events
   Copyright (C) 2014-2024 Markus Uhlin. All rights reserved.

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

#include <climits>
#include <ctime>
#include <stdexcept>
#include <string>

#include "../config.h"
#include "../dataClassify.h"
#include "../errHand.h"
#include "../irc.h"
#include "../libUtils.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "whois.h"

#define NO_MSG "no message"

class time_idle {
	long int	days;
	long int	hours;
	long int	mins;
	long int	secs;

	char buf[200];

public:
	time_idle(long int, long int);

	long int
	getDays(void) const
	{
		return this->days;
	}

	long int
	getHours(void) const
	{
		return this->hours;
	}

	long int
	getMins(void) const
	{
		return this->mins;
	}

	long int
	getSecs(void) const
	{
		return this->secs;
	}

	const char *
	getBuf(void) const
	{
		return (&this->buf[0]);
	}
};

time_idle::time_idle(long int sec_idle, long int signon_time)
{
	struct tm	res = { 0 };
	time_t		elapsed = signon_time;

#if defined(UNIX)
	if (localtime_r(&elapsed, &res) == NULL)
		throw std::runtime_error("cannot retrieve tm structure");
#elif defined(WIN32)
	if (localtime_s(&res, &elapsed) != 0)
		throw std::runtime_error("cannot retrieve tm structure");
#endif

	this->days = (sec_idle / 86400);
	this->hours = (sec_idle / 3600) - (this->days * 24);
	this->mins = (sec_idle / 60) - (this->days * 1440) - (this->hours * 60);
	this->secs = (sec_idle % 60);

	if (strftime(this->buf, ARRAY_SIZE(this->buf), "%c", &res) == 0)
		throw std::runtime_error("cannot format date and time");
}

static bool
get_msg(char *params, std::string &str)
{
	char *msg;
	char *state = const_cast<char *>("");

	if (strFeed(params, 2) != 2) {
		(void) str.assign("");
		return false;
	}

	(void) strtok_r(params, "\n", &state);
	(void) strtok_r(NULL, "\n", &state);

	if ((msg = strtok_r(NULL, "\n", &state)) == NULL) {
		(void) str.assign("");
		return false;
	}

	if (*msg == ':')
		msg++;

	(void) str.assign(msg);
	return true;
}

static bool
is_privconv()
{
	if (strings_match_ignore_case(ACTWINLABEL, g_status_window_label) ||
	    is_irc_channel(ACTWINLABEL))
		return false;
	return true;
}

/* event_whoReply: 352 (RPL_WHOREPLY)

   Example:
     :irc.server.com 352 <my nick> <channel> <user> <host> <server> <nick>
                         <"H" / "G"> :<hopcount> <real name> */
void
event_whoReply(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1, true);

	try {
		char	*channel, *user, *host, *server, *nick, *symbol,
			*hopcount, *rl_name;
		char	*state = const_cast<char *>("");

		if (strFeed(compo->params, 8) != 8)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state); /* my nick */

		if ((channel = strtok_r(NULL, "\n", &state)) == NULL ||
		    (user = strtok_r(NULL, "\n", &state)) == NULL ||
		    (host = strtok_r(NULL, "\n", &state)) == NULL ||
		    (server = strtok_r(NULL, "\n", &state)) == NULL || // unused
		    (nick = strtok_r(NULL, "\n", &state)) == NULL ||
		    (symbol = strtok_r(NULL, "\n", &state)) == NULL ||
		    (hopcount = strtok_r(NULL, "\n", &state)) == NULL ||
		    (rl_name = strtok_r(NULL, "\n", &state)) == NULL) {
			throw std::runtime_error("unable to retrieve event "
			    "components");
		}

		(void) server;

		if (*hopcount == ':')
			hopcount++;

		std::string	str1("");
		std::string	str2("");
		std::string	str3("");

		str1.append(LEFT_BRKT);
		str1.append(COLOR1).append(channel).append(TXT_NORMAL);
		str1.append(RIGHT_BRKT);

		str2.append(COLOR2).append(nick).append(TXT_NORMAL);

		str3.append(LEFT_BRKT);
		str3.append(COLOR2).append(rl_name).append(TXT_NORMAL);
		str3.append(RIGHT_BRKT);

		printtext(&ctx, "%s: %s %s %s %s@%s %s", str1.c_str(),
		    str2.c_str(), symbol, hopcount, user, host, str3.c_str());
	} catch (const std::bad_alloc &e) {
		err_exit(ENOMEM, "%s(%s): error: %s", __func__, compo->command,
		    e.what());
	} catch (const std::runtime_error &e) {
		ctx.spec_type = TYPE_SPEC1_FAILURE;

		printtext(&ctx, "%s(%s): error: %s", __func__, compo->command,
		    e.what());
	} catch (...) {
		err_log(0, "%s(%s): error: %s", __func__, compo->command,
		    "unknown exception");
	}
}

/* event_whois_acc: 330

   Example:
     :irc.server.com 330 <issuer> <target> <account name> :is logged in as */
void
event_whois_acc(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	try {
		char	*account_name;
		char	*comment;
		char	*state = const_cast<char *>("");

		if (strFeed(compo->params, 3) != 3)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state);
		(void) strtok_r(NULL, "\n", &state);

		if ((account_name = strtok_r(NULL, "\n", &state)) == NULL ||
		    (comment = strtok_r(NULL, "\n", &state)) == NULL) {
			throw std::runtime_error("unable to retrieve event "
			    "components");
		}
		if (*comment == ':')
			comment++;
		if (*comment) {
			printtext(&ctx, "%s %s %s", Theme("whois_acc"), comment,
			    account_name);
		}
	} catch (const std::runtime_error &e) {
		ctx.window	= g_status_window;
		ctx.spec_type	= TYPE_SPEC1_WARN;

		printtext(&ctx, "event_whois_acc(%s): error: %s",
		    compo->command, e.what());
	}
}

/* event_whois_away: 301

   Example:
     :irc.server.com 301 <issuer> <target> :<away reason> */
void
event_whois_away(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	try {
		char	*away_reason;
		char	*state = const_cast<char *>("");

		if (strFeed(compo->params, 2) != 2)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state);
		(void) strtok_r(NULL, "\n", &state);

		if ((away_reason = strtok_r(NULL, "\n", &state)) == NULL)
			throw std::runtime_error("no away reason");
		if (*away_reason == ':')
			away_reason++;
		if (!is_privconv() || config_bool("awaymsgs_in_privconv",
		    true)) {
			printtext(&ctx, "%s %s", Theme("whois_away"),
			    away_reason);
		}
	} catch (const std::runtime_error &e) {
		ctx.window	= g_status_window;
		ctx.spec_type	= TYPE_SPEC1_WARN;

		printtext(&ctx, "event_whois_away(%s): error: %s",
		    compo->command, e.what());
	}
}

/* event_whois_bot: 335 (RPL_WHOISBOT)

   Example:
     :irc.server.com 335 <nick> <target> :<message> */
void
event_whois_bot(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	try {
		std::string msg("");

		if (!get_msg(compo->params, msg))
			throw std::runtime_error(NO_MSG);
		else if (!msg.empty()) {
			printtext(&ctx, "%s %s", Theme("whois_bot"),
			    msg.c_str());
		}
	} catch (const std::runtime_error &e) {
		ctx.window	= g_status_window;
		ctx.spec_type	= TYPE_SPEC1_WARN;

		printtext(&ctx, "event_whois_bot(%s): error: %s",
		    compo->command, e.what());
	}
}

/* event_whois_cert: 276

   Example:
     :irc.server.com 276 <issuer> <target> :has client certificate fingerprint
                                            <string> */
void
event_whois_cert(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	try {
		char	*msg;
		char	*state = const_cast<char *>("");
		char	*tnick;

		if (strFeed(compo->params, 2) != 2)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state);

		if ((tnick = strtok_r(NULL, "\n", &state)) == NULL ||
		    (msg = strtok_r(NULL, "\n", &state)) == NULL) {
			throw std::runtime_error("unable to retrieve event "
			    "components");
		}
		if (*msg == ':')
			msg++;
		if (*msg) {
			printtext(&ctx, "%s %s %s", Theme("whois_cert"), tnick,
			    msg);
		}
	} catch (const std::runtime_error &e) {
		ctx.window	= g_status_window;
		ctx.spec_type	= TYPE_SPEC1_WARN;

		printtext(&ctx, "event_whois_cert(%s): error: %s",
		    compo->command, e.what());
	}
}

/* event_whois_channels: 319

   Example:
     :irc.server.com 319 <issuer> <target> :<channel list> */
void
event_whois_channels(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	try {
		char	*chan_list;
		char	*state = const_cast<char *>("");

		if (strFeed(compo->params, 2) != 2)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state);
		(void) strtok_r(NULL, "\n", &state);

		if ((chan_list = strtok_r(NULL, "\n", &state)) == NULL)
			throw std::runtime_error("no chan list");
		if (*chan_list == ':')
			chan_list++;
		if (*chan_list) {
			printtext(&ctx, "%s %s", Theme("whois_channels"),
			    chan_list);
		}
	} catch (const std::runtime_error &e) {
		ctx.window	= g_status_window;
		ctx.spec_type	= TYPE_SPEC1_WARN;

		printtext(&ctx, "event_whois_channels(%s): error: %s",
		    compo->command, e.what());
	}
}

/* event_whois_conn: 378

   Example:
     :irc.server.com 378 <issuer> <target> :is connecting from
                                            <hostname> <IP> */
void
event_whois_conn(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	try {
		std::string msg("");

		if (!get_msg(compo->params, msg))
			throw std::runtime_error(NO_MSG);
		else if (!msg.empty()) {
			printtext(&ctx, "%s %s", Theme("whois_conn"),
			    msg.c_str());
		}
	} catch (const std::runtime_error &e) {
		ctx.window	= g_status_window;
		ctx.spec_type	= TYPE_SPEC1_WARN;

		printtext(&ctx, "event_whois_conn(%s): error: %s",
		    compo->command, e.what());
	}
}

/* event_whois_host: 338, 616

   Example:
     :irc.server.com 338 <issuer> <target> <IP> :actually using host
     :irc.server.com 338 <issuer> <target> :is actually <user@host> [<IP>]
     :irc.server.com 616 <issuer> <target> :real hostname ... */
void
event_whois_host(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	try {
		char	*state = const_cast<char *>("");
		char	*str;
		char	*str_copy, *cp;

		if (strFeed(compo->params, 2) != 2)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state);
		(void) strtok_r(NULL, "\n", &state);

		if ((str = strtok_r(NULL, "\n", &state)) == NULL)
			throw std::runtime_error("null string");
		else if (*str == ':')
			str++;

		str_copy = sw_strdup(str);

		if ((cp = strstr(str_copy, ":actually using host")) != NULL &&
		    strlen(cp) == 20) {
			cp++;
			memmove(cp - 1, cp, strlen(cp) + 1);
		}

		printtext(&ctx, "%s %s", Theme("whois_host"), str_copy);
		free(str_copy);
	} catch (const std::runtime_error &e) {
		ctx.window	= g_status_window;
		ctx.spec_type	= TYPE_SPEC1_WARN;

		printtext(&ctx, "event_whois_host(%s): error: %s",
		    compo->command, e.what());
	}
}

/* event_whois_idle: 317

   Example:
     :irc.server.com 317 <issuer> <target> <sec idle> <signon time>
                         :<comment> */
void
event_whois_idle(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	try {
		char		*ep1 = const_cast<char *>("");
		char		*ep2 = const_cast<char *>("");
		char		*state = const_cast<char *>("");
		const char	*sec_idle_str = NULL;
		const char	*signon_time_str = NULL;
		long int	 sec_idle = LONG_MAX;
		long int	 signon_time = LONG_MAX;

		if (strFeed(compo->params, 4) != 4)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state);
		(void) strtok_r(NULL, "\n", &state);

		if ((sec_idle_str = strtok_r(NULL, "\n", &state)) == NULL ||
		    (signon_time_str = strtok_r(NULL, "\n", &state)) == NULL) {
			throw std::runtime_error("unable to retrieve event "
			    "components");
		}

		errno = 0;
		sec_idle = strtol(sec_idle_str, &ep1, 10);

		if (sec_idle_str[0] == '\0' || *ep1 != '\0')
			throw std::runtime_error("sec idle: not a number");
		else if (errno == ERANGE &&
		    (sec_idle == LONG_MAX || sec_idle == LONG_MIN))
			throw std::runtime_error("sec idle: out of range");

		errno = 0;
		signon_time = strtol(signon_time_str, &ep2, 10);

		if (signon_time_str[0] == '\0' || *ep2 != '\0')
			throw std::runtime_error("signon time: not a number");
		else if (errno == ERANGE &&
		    (signon_time == LONG_MAX || signon_time == LONG_MIN))
			throw std::runtime_error("signon time: out of range");

		time_idle ti(sec_idle, signon_time);

		printtext(&ctx, "%s %ld days %ld hours %ld mins %ld secs "
		    "%ssignon: %s%s",
		    Theme("whois_idle"),
		    ti.getDays(), ti.getHours(), ti.getMins(), ti.getSecs(),
		    LEFT_BRKT, ti.getBuf(), RIGHT_BRKT);
	} catch (const std::runtime_error &e) {
		ctx.window	= g_status_window;
		ctx.spec_type	= TYPE_SPEC1_WARN;

		printtext(&ctx, "event_whois_idle(%s): error: %s",
		    compo->command, e.what());
	}
}

/* event_whois_ircOp: 313

   Example:
     :irc.server.com 313 <issuer> <target> :is an IRC Operator */
void
event_whois_ircOp(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	try {
		std::string msg("");

		if (!get_msg(compo->params, msg))
			throw std::runtime_error(NO_MSG);
		else if (!msg.empty()) {
			printtext(&ctx, "%s %s", Theme("whois_ircOp"),
			    msg.c_str());
		}
	} catch (const std::runtime_error &e) {
		ctx.window	= g_status_window;
		ctx.spec_type	= TYPE_SPEC1_WARN;

		printtext(&ctx, "event_whois_ircOp(%s): error: %s",
		    compo->command, e.what());
	}
}

/* event_whois_modes: 379, 310, 615

   Example:
     :irc.server.com 379 <issuer> <target> :is using modes <modes>
     :irc.server.com 310 <issuer> <target> :is using modes <modes> authflags:
                                            [...]
     :irc.server.com 615 <issuer> <target> :is using modes ... */
void
event_whois_modes(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	try {
		std::string msg("");

		if (!get_msg(compo->params, msg))
			throw std::runtime_error(NO_MSG);
		else if (!msg.empty()) {
			printtext(&ctx, "%s %s", Theme("whois_modes"),
			    msg.c_str());
		}
	} catch (const std::runtime_error &e) {
		ctx.window	= g_status_window;
		ctx.spec_type	= TYPE_SPEC1_WARN;

		printtext(&ctx, "event_whois_modes(%s): error: %s",
		    compo->command, e.what());
	}
}

/* event_whois_server: 312

   Example:
     :irc.server.com 312 <issuer> <target> <server> :<info> */
void
event_whois_server(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	try {
		char	*srv, *info;
		char	*state = const_cast<char *>("");

		if (strFeed(compo->params, 3) != 3)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state);
		(void) strtok_r(NULL, "\n", &state);

		if ((srv = strtok_r(NULL, "\n", &state)) == NULL)
			throw std::runtime_error("no server");
		else if ((info = strtok_r(NULL, "\n", &state)) == NULL)
			throw std::runtime_error("no info");

		if (*info == ':')
			info++;
		if (*info) {
			printtext(&ctx, "%s %s %s%s%s", Theme("whois_server"),
			    srv, LEFT_BRKT, info, RIGHT_BRKT);
		}
	} catch (const std::runtime_error &e) {
		ctx.window	= g_status_window;
		ctx.spec_type	= TYPE_SPEC1_WARN;

		printtext(&ctx, "event_whois_server(%s): error: %s",
		    compo->command, e.what());
	}
}

/* event_whois_service: 307

   Example:
     :irc.server.com 307 <issuer> <target> :user has identified to services */
void
event_whois_service(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	try {
		std::string msg("");

		if (!get_msg(compo->params, msg))
			throw std::runtime_error(NO_MSG);
		else if (!msg.empty()) {
			printtext(&ctx, "%s %s", Theme("whois_service"),
			    msg.c_str());
		}
	} catch (const std::runtime_error &e) {
		ctx.window	= g_status_window;
		ctx.spec_type	= TYPE_SPEC1_WARN;

		printtext(&ctx, "event_whois_service(%s): error: %s",
		    compo->command, e.what());
	}
}

/* event_whois_ssl: 275, 320, 671

   Examples:
     :irc.server.com 275 <issuer> <target> :is connected via SSL (secure link)
     :irc.server.com 320 <issuer> <target> :is a Secure Connection (SSL/TLS)
     :irc.server.com 671 <issuer> <target> :is using a secure connection */
void
event_whois_ssl(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	try {
		char	*tnick, *msg;
		char	*state = const_cast<char *>("");

		if (strFeed(compo->params, 2) != 2)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state);

		if ((tnick = strtok_r(NULL, "\n", &state)) == NULL ||
		    (msg = strtok_r(NULL, "\n", &state)) == NULL) {
			throw std::runtime_error("unable to retrieve event "
			    "components");
		}

		if (*msg == ':')
			msg++;
		if (*msg) {
			printtext(&ctx, "%s %s %s", Theme("whois_ssl"), tnick,
			    msg);
		}
	} catch (const std::runtime_error &e) {
		ctx.window	= g_status_window;
		ctx.spec_type	= TYPE_SPEC1_WARN;

		printtext(&ctx, "event_whois_ssl(%s): error: %s",
		    compo->command, e.what());
	}
}

/* event_whois_user: 311

   Example:
     :irc.server.com 311 <issuer> <target> <username> <hostname> *
                         :<real name> */
void
event_whois_user(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	try {
		char	*nick, *user, *host, *rl_name;
		char	*state = const_cast<char *>("");

		if (strFeed(compo->params, 5) != 5)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state); /* <issuer> */
		nick = strtok_r(NULL, "\n", &state);
		user = strtok_r(NULL, "\n", &state);
		host = strtok_r(NULL, "\n", &state);
		(void) strtok_r(NULL, "\n", &state);
		rl_name = strtok_r(NULL, "\n", &state);

		if (nick == NULL || user == NULL || host == NULL || rl_name ==
		    NULL) {
			throw std::runtime_error("unable to retrieve event "
			    "components");
		}

		printtext(&ctx, "%c%s%c %s%s@%s%s", BOLD, nick, BOLD,
		    LEFT_BRKT, user, host, RIGHT_BRKT);

		if (*rl_name == ':')
			rl_name++;
		if (*rl_name) {
			printtext(&ctx, "%s %s", Theme("whois_ircName"),
			    rl_name);
		}
	} catch (const std::runtime_error &e) {
		ctx.window	= g_status_window;
		ctx.spec_type	= TYPE_SPEC1_WARN;

		printtext(&ctx, "event_whois_user(%s): error: %s",
		    compo->command, e.what());
	}
}
