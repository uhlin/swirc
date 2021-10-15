/* Whois events
   Copyright (C) 2014-2021 Markus Uhlin. All rights reserved.

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

#include "../irc.h"
#include "../libUtils.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "whois.h"

class time_idle {
    long int days;
    long int hours;
    long int mins;
    long int secs;
    char buf[200];

public:
    time_idle(long int sec_idle, long int signon_time) {
	struct tm res = { 0 };
	time_t elapsed = signon_time;

#if defined(UNIX)
	if (localtime_r(&elapsed, &res) == NULL)
#elif defined(WIN32)
	if (localtime_s(&res, &elapsed) != 0)
#endif
	    {
		throw std::runtime_error("cannot retrieve tm structure");
	    }

	this->days  = sec_idle / 86400;
	this->hours = (sec_idle / 3600) - (this->days * 24);
	this->mins  = (sec_idle / 60) - (this->days * 1440) - (this->hours * 60);
	this->secs  = sec_idle % 60;

	if (strftime(this->buf, ARRAY_SIZE(this->buf), "%c", &res) == 0)
	    throw std::runtime_error("cannot format date and time");
    }

    long int getDays(void) const {
	return this->days;
    }
    long int getHours(void) const {
	return this->hours;
    }
    long int getMins(void) const {
	return this->mins;
    }
    long int getSecs(void) const {
	return this->secs;
    }
    const char *getBuf(void) const {
	return (&this->buf[0]);
    }
};

/* event_whoReply: 352 (RPL_WHOREPLY)

   Example:
     :irc.server.com 352 <my nick> <channel> <user> <host> <server> <nick>
                         <"H" / "G"> :<hopcount> <real name> */
void
event_whoReply(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_status_window, TYPE_SPEC1, true);

    try {
	char *state = const_cast<char *>("");

	if (strFeed(compo->params, 8) != 8)
	    throw std::runtime_error("strFeed");

	(void) strtok_r(compo->params, "\n", &state); /* my nick */
	char *channel  = strtok_r(NULL, "\n", &state);
	char *user     = strtok_r(NULL, "\n", &state);
	char *host     = strtok_r(NULL, "\n", &state);
	char *server   = strtok_r(NULL, "\n", &state); /* unused */
	char *nick     = strtok_r(NULL, "\n", &state);
	char *symbol   = strtok_r(NULL, "\n", &state);
	char *hopcount = strtok_r(NULL, "\n", &state);
	char *rl_name  = strtok_r(NULL, "\n", &state);

	if (channel==NULL || user==NULL || host==NULL || server==NULL ||
	    nick==NULL || symbol==NULL || hopcount==NULL || rl_name==NULL)
	    throw std::runtime_error("unable to retrieve event components");
	if (*hopcount == ':')
	    hopcount++;
	printtext(&ctx, "%s%s%s%c%s: %s%s%c %s %s %s@%s %s%s%s%c%s",
	    LEFT_BRKT, COLOR1, channel, NORMAL, RIGHT_BRKT,
	    COLOR2, nick, NORMAL,
	    symbol, hopcount, user, host,
	    LEFT_BRKT, COLOR2, rl_name, NORMAL, RIGHT_BRKT);
    } catch (const std::runtime_error &e) {
	ctx.spec_type = TYPE_SPEC1_FAILURE;
	printtext(&ctx, "event_whoReply(%s): error: %s",
	    compo->command, e.what());
    }
}

/* event_whois_acc: 330

   Example:
     :irc.server.com 330 <issuer> <target> <account name> :is logged in as */
void
event_whois_acc(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

    try {
	char *state = const_cast<char *>("");

	if (strFeed(compo->params, 3) != 3)
	    throw std::runtime_error("strFeed");

	(void) strtok_r(compo->params, "\n", &state);
	(void) strtok_r(NULL, "\n", &state);
	char *account_name = strtok_r(NULL, "\n", &state);
	char *comment      = strtok_r(NULL, "\n", &state);

	if (account_name == NULL || comment == NULL)
	    throw std::runtime_error("unable to retrieve event components");
	if (*comment == ':')
	    comment++;
	if (*comment) {
	    printtext(&ctx, "%s %s %s",
		Theme("whois_acc"), comment, account_name);
	}
    } catch (const std::runtime_error &e) {
	ctx.window = g_status_window;
	ctx.spec_type = TYPE_SPEC1_WARN;
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
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

    try {
	char *away_reason = NULL;
	char *state = const_cast<char *>("");

	if (strFeed(compo->params, 2) != 2)
	    throw std::runtime_error("strFeed");

	(void) strtok_r(compo->params, "\n", &state);
	(void) strtok_r(NULL, "\n", &state);

	if ((away_reason = strtok_r(NULL, "\n", &state)) == NULL)
	    throw std::runtime_error("no away reason");
	if (*away_reason == ':')
	    away_reason++;
	if (*away_reason)
	    printtext(&ctx, "%s %s", Theme("whois_away"), away_reason);
    } catch (const std::runtime_error &e) {
	ctx.window = g_status_window;
	ctx.spec_type = TYPE_SPEC1_WARN;
	printtext(&ctx, "event_whois_away(%s): error: %s",
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
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

    try {
	char *state = const_cast<char *>("");

	if (strFeed(compo->params, 2) != 2)
	    throw std::runtime_error("strFeed");

	(void) strtok_r(compo->params, "\n", &state);
	char *tnick = strtok_r(NULL, "\n", &state);
	char *msg   = strtok_r(NULL, "\n", &state);

	if (tnick == NULL || msg == NULL)
	    throw std::runtime_error("unable to retrieve event components");
	if (*msg == ':')
	    msg++;
	if (*msg)
	    printtext(&ctx, "%s %s %s", Theme("whois_cert"), tnick, msg);
    } catch (const std::runtime_error &e) {
	ctx.window = g_status_window;
	ctx.spec_type = TYPE_SPEC1_WARN;
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
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

    try {
	char *chan_list = NULL;
	char *state = const_cast<char *>("");

	if (strFeed(compo->params, 2) != 2)
	    throw std::runtime_error("strFeed");

	(void) strtok_r(compo->params, "\n", &state);
	(void) strtok_r(NULL, "\n", &state);

	if ((chan_list = strtok_r(NULL, "\n", &state)) == NULL)
	    throw std::runtime_error("no chan list");
	if (*chan_list == ':')
	    chan_list++;
	if (*chan_list)
	    printtext(&ctx, "%s %s", Theme("whois_channels"), chan_list);
    } catch (const std::runtime_error &e) {
	ctx.window = g_status_window;
	ctx.spec_type = TYPE_SPEC1_WARN;
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
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

    try {
	char *msg = NULL;
	char *state = const_cast<char *>("");

	if (strFeed(compo->params, 2) != 2)
	    throw std::runtime_error("strFeed");

	(void) strtok_r(compo->params, "\n", &state);
	(void) strtok_r(NULL, "\n", &state);

	if ((msg = strtok_r(NULL, "\n", &state)) == NULL)
	    throw std::runtime_error("null msg");
	if (*msg == ':')
	    msg++;
	if (*msg)
	    printtext(&ctx, "%s %s", Theme("whois_conn"), msg);
    } catch (const std::runtime_error &e) {
	ctx.window = g_status_window;
	ctx.spec_type = TYPE_SPEC1_WARN;
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
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

    try {
	char *state = const_cast<char *>("");
	char *str = NULL;

	if (strFeed(compo->params, 2) != 2)
	    throw std::runtime_error("strFeed");

	(void) strtok_r(compo->params, "\n", &state);
	(void) strtok_r(NULL, "\n", &state);

	if ((str = strtok_r(NULL, "\n", &state)) == NULL)
	    throw std::runtime_error("null string");

	char *str_copy = sw_strdup(str);
	//char *cp = &str_copy[0];
	squeeze(str_copy, ":");
	printtext(&ctx, "%s %s", Theme("whois_host"), str_copy);
	free(str_copy);
    } catch (const std::runtime_error &e) {
	ctx.window = g_status_window;
	ctx.spec_type = TYPE_SPEC1_WARN;
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
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

    try {
	char *state = const_cast<char *>("");
	if (strFeed(compo->params, 4) != 4)
	    throw std::runtime_error("strFeed");

	(void) strtok_r(compo->params, "\n", &state);
	(void) strtok_r(NULL, "\n", &state);
	char *sec_idle_str    = strtok_r(NULL, "\n", &state);
	char *signon_time_str = strtok_r(NULL, "\n", &state);

	if (sec_idle_str == NULL || signon_time_str == NULL)
	    throw std::runtime_error("unable to retrieve event components");

	errno = 0;
	char *ep1 = const_cast<char *>("");
	long int sec_idle = strtol(sec_idle_str, &ep1, 10);
	if (sec_idle_str[0] == '\0' || *ep1 != '\0')
	    throw std::runtime_error("sec idle: not a number");
	else if (errno == ERANGE &&
		 (sec_idle == LONG_MAX || sec_idle == LONG_MIN))
	    throw std::runtime_error("sec idle: out of range");

	errno = 0;
	char *ep2 = const_cast<char *>("");
	long int signon_time = strtol(signon_time_str, &ep2, 10);
	if (signon_time_str[0] == '\0' || *ep2 != '\0')
	    throw std::runtime_error("signon time: not a number");
	else if (errno == ERANGE &&
		 (signon_time == LONG_MAX || signon_time == LONG_MIN))
	    throw std::runtime_error("signon time: out of range");

	time_idle ti(sec_idle, signon_time);

	printtext(&ctx, "%s %ld days %ld hours %ld mins %ld secs %ssignon: %s%s",
	    Theme("whois_idle"),
	    ti.getDays(), ti.getHours(), ti.getMins(), ti.getSecs(),
	    LEFT_BRKT, ti.getBuf(), RIGHT_BRKT);
    } catch (const std::runtime_error &e) {
	ctx.window = g_status_window;
	ctx.spec_type = TYPE_SPEC1_WARN;
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
		char	*msg;
		char	*state = const_cast<char *>("");

		if (strFeed(compo->params, 2) != 2)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state);
		(void) strtok_r(NULL, "\n", &state);

		if ((msg = strtok_r(NULL, "\n", &state)) == NULL)
			throw std::runtime_error("no message");
		if (*msg == ':')
			msg++;
		if (*msg)
			printtext(&ctx, "%s %s", Theme("whois_ircOp"), msg);
	} catch (const std::runtime_error& e) {
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
		char	*msg;
		char	*state = const_cast<char *>("");

		if (strFeed(compo->params, 2) != 2)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state);
		(void) strtok_r(NULL, "\n", &state);

		if ((msg = strtok_r(NULL, "\n", &state)) == NULL)
			throw std::runtime_error("no message");
		if (*msg == ':')
			msg++;
		if (*msg)
			printtext(&ctx, "%s %s", Theme("whois_modes"), msg);
	} catch (const std::runtime_error& e) {
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
		char	*state = const_cast<char *>("");

		if (strFeed(compo->params, 3) != 3)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state);
		(void) strtok_r(NULL, "\n", &state);
		char *srv = strtok_r(NULL, "\n", &state);
		char *info = strtok_r(NULL, "\n", &state);

		if (srv == NULL)
			throw std::runtime_error("no server");
		else if (info == NULL)
			throw std::runtime_error("no info");

		if (*info == ':')
			info++;
		if (*info) {
			printtext(&ctx, "%s %s %s%s%s", Theme("whois_server"),
			    srv, LEFT_BRKT, info, RIGHT_BRKT);
		}
	} catch (const std::runtime_error& e) {
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
		char	*msg;
		char	*state = const_cast<char *>("");

		if (strFeed(compo->params, 2) != 2)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state);
		(void) strtok_r(NULL, "\n", &state);

		if ((msg = strtok_r(NULL, "\n", &state)) == NULL)
			throw std::runtime_error("no message");
		if (*msg == ':')
			msg++;
		if (*msg)
			printtext(&ctx, "%s %s", Theme("whois_service"), msg);
	} catch (const std::runtime_error& e) {
		ctx.window	= g_status_window;
		ctx.spec_type	= TYPE_SPEC1_WARN;

		printtext(&ctx, "event_whois_service(%s): error: %s",
		    compo->command, e.what());
	}
}

/* event_whois_ssl: 275, 671

   Examples:
     :irc.server.com 275 <issuer> <target> :is connected via SSL (secure link)
     :irc.server.com 671 <issuer> <target> :is using a secure connection */
void
event_whois_ssl(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	try {
		char	*state = const_cast<char *>("");

		if (strFeed(compo->params, 2) != 2)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state);
		char *tnick = strtok_r(NULL, "\n", &state);
		char *msg = strtok_r(NULL, "\n", &state);

		if (tnick == NULL || msg == NULL) {
			throw std::runtime_error("unable to retrieve event "
			    "components");
		}

		if (*msg == ':')
			msg++;
		if (*msg) {
			printtext(&ctx, "%s %s %s", Theme("whois_ssl"), tnick,
			    msg);
		}
	} catch (const std::runtime_error& e) {
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
		char	*state = const_cast<char *>("");

		if (strFeed(compo->params, 5) != 5)
			throw std::runtime_error("strFeed");

		(void) strtok_r(compo->params, "\n", &state); /* <issuer> */
		char *nick = strtok_r(NULL, "\n", &state);
		char *user = strtok_r(NULL, "\n", &state);
		char *host = strtok_r(NULL, "\n", &state);
		(void) strtok_r(NULL, "\n", &state);
		char *rl_name = strtok_r(NULL, "\n", &state);

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
	} catch (const std::runtime_error& e) {
		ctx.window	= g_status_window;
		ctx.spec_type	= TYPE_SPEC1_WARN;

		printtext(&ctx, "event_whois_user(%s): error: %s",
		    compo->command, e.what());
	}
}
