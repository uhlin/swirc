/* whois events
   Copyright (C) 2014-2020 Markus Uhlin. All rights reserved.

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

struct time_idle {
    long int days;
    long int hours;
    long int mins;
    long int secs;
    char buf[200];
};

/*lint -sem(get_time_idle, r_null) */
static struct time_idle *
get_time_idle(long int sec_idle, long int signon_time)
{
    time_t elapsed = signon_time;
    struct tm res;
    struct time_idle *ti =
	static_cast<struct time_idle *>(xcalloc(sizeof *ti, 1));
    const char fmt[] = "%c";

#if defined(UNIX)
    if (localtime_r(&elapsed, &res) == NULL)
#elif defined(WIN32)
    if (localtime_s(&res, &elapsed) != 0)
#endif
	{
	    free(ti);
	    return (NULL);
	}

    ti->days  = sec_idle / 86400;
    ti->hours = (sec_idle / 3600) - (ti->days * 24);
    ti->mins  = (sec_idle / 60) - (ti->days * 1440) - (ti->hours * 60);
    ti->secs  = sec_idle % 60;

    if (strftime(ti->buf, sizeof ti->buf - 1, fmt, &res) == 0) {
	free(ti);
	return (NULL);
    }

    return (ti);
}

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
    char *ep1, *ep2;
    char *sec_idle_str, *signon_time_str;
    char *state = "";
    long int sec_idle, signon_time;
    struct time_idle *ti;

    if (strFeed(compo->params, 4) != 4) {
	goto bad;
    }

    (void) strtok_r(compo->params, "\n", &state);
    (void) strtok_r(NULL, "\n", &state);
    sec_idle_str    = strtok_r(NULL, "\n", &state);
    signon_time_str = strtok_r(NULL, "\n", &state);

    if (sec_idle_str == NULL || signon_time_str == NULL) {
	goto bad;
    }

    errno = 0;
    sec_idle = strtol(sec_idle_str, &ep1, 10);
    if (sec_idle_str[0] == '\0' || *ep1 != '\0') {
	goto bad;
    } else if (errno == ERANGE &&
	       (sec_idle == LONG_MAX || sec_idle == LONG_MIN)) {
	goto bad;
    } else {
	/* do nothing */;
    }

    errno = 0;
    signon_time = strtol(signon_time_str, &ep2, 10);
    if (signon_time_str[0] == '\0' || *ep2 != '\0') {
	goto bad;
    } else if (errno == ERANGE &&
	       (signon_time == LONG_MAX || signon_time == LONG_MIN)) {
	goto bad;
    } else {
	/* do nothing */;
    }

    if ((ti = get_time_idle(sec_idle, signon_time)) == NULL) {
	goto bad;
    }

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);
    printtext(&ctx, "%s %ld days %ld hours %ld mins %ld secs %ssignon: %s%s",
	      Theme("whois_idle"), ti->days, ti->hours, ti->mins, ti->secs,
	      LEFT_BRKT, ti->buf, RIGHT_BRKT);
    free(ti);
    return;

  bad:
    printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);
    printtext(&ctx, "On issuing event %s: An error occurred", compo->command);
}

/* event_whois_ircOp: 313

   Example:
     :irc.server.com 313 <issuer> <target> :is an IRC Operator */
void
event_whois_ircOp(struct irc_message_compo *compo)
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
	    throw std::runtime_error("no message");
	if (*msg == ':')
	    msg++;
	if (*msg)
	    printtext(&ctx, "%s %s", Theme("whois_ircOp"), msg);
    } catch (const std::runtime_error &e) {
	ctx.window = g_status_window;
	ctx.spec_type = TYPE_SPEC1_WARN;
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
	    throw std::runtime_error("no message");
	if (*msg == ':')
	    msg++;
	if (*msg)
	    printtext(&ctx, "%s %s", Theme("whois_modes"), msg);
    } catch (const std::runtime_error &e) {
	ctx.window = g_status_window;
	ctx.spec_type = TYPE_SPEC1_WARN;
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
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

    try {
	char *state = const_cast<char *>("");

	if (strFeed(compo->params, 3) != 3)
	    throw std::runtime_error("strFeed");

	(void) strtok_r(compo->params, "\n", &state);
	(void) strtok_r(NULL, "\n", &state);
	char *srv  = strtok_r(NULL, "\n", &state);
	char *info = strtok_r(NULL, "\n", &state);

	if (srv == NULL)
	    throw std::runtime_error("no server");
	else if (info == NULL)
	    throw std::runtime_error("no info");

	if (*info == ':')
	    info++;
	if (*info) {
	    printtext(&ctx, "%s %s %s%s%s",
		Theme("whois_server"), srv, LEFT_BRKT, info, RIGHT_BRKT);
	}
    } catch (const std::runtime_error &e) {
	ctx.window = g_status_window;
	ctx.spec_type = TYPE_SPEC1_WARN;
	printtext(&ctx, "event_whois_server(%s): error: %s",
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
    PRINTTEXT_CONTEXT ctx;
    char *state = "";
    char *tnick, *msg;

    printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);

    if (strFeed(compo->params, 2) != 2) {
	printtext(&ctx, "On issuing event %s: strFeed(..., 2) != 2",
		  compo->command);
	return;
    }

    (void) strtok_r(compo->params, "\n", &state);
    tnick = strtok_r(NULL, "\n", &state);
    msg   = strtok_r(NULL, "\n", &state);

    if (tnick == NULL || msg == NULL) {
	printtext(&ctx, "On issuing event %s: Unable to extract message",
		  compo->command);
	return;
    }

    if (*msg == ':') {
	msg++;
    }

    if (*msg) {
	ctx.window    = g_active_window;
	ctx.spec_type = TYPE_SPEC1;
	printtext(&ctx, "%s %s %s", Theme("whois_ssl"), tnick, msg);
    }
}

/* event_whois_service: 307

   Example:
     :irc.server.com 307 <issuer> <target> :user has identified to services */
void
event_whois_service(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;
    char *msg;
    char *state = "";

    printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);

    if (strFeed(compo->params, 2) != 2) {
	printtext(&ctx, "On issuing event %s: strFeed(..., 2) != 2",
		  compo->command);
	return;
    }

    (void) strtok_r(compo->params, "\n", &state);
    (void) strtok_r(NULL, "\n", &state);

    if ((msg = strtok_r(NULL, "\n", &state)) == NULL) {
	printtext(&ctx, "On issuing event %s: Unable to extract message",
		  compo->command);
	return;
    }

    if (*msg == ':') {
	msg++;
    }

    if (*msg) {
	ctx.window    = g_active_window;
	ctx.spec_type = TYPE_SPEC1;
	printtext(&ctx, "%s %s", Theme("whois_service"), msg);
    }
}

/* event_whois_user: 311

   Example:
     :irc.server.com 311 <issuer> <target> <username> <hostname> *
                         :<real name> */
void
event_whois_user(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;
    char *nick, *user, *host, *rl_name;
    char *state = "";

    printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);

#if 0
    nick = user = host = rl_name = NULL;
#endif

    if (strFeed(compo->params, 5) != 5) {
	printtext(&ctx, "On issuing event %s: strFeed(..., 5) != 5",
		  compo->command);
	return;
    }

    (void) strtok_r(compo->params, "\n", &state); /* <issuer> */
    nick    = strtok_r(NULL, "\n", &state);
    user    = strtok_r(NULL, "\n", &state);
    host    = strtok_r(NULL, "\n", &state);
    (void) strtok_r(NULL, "\n", &state);
    rl_name = strtok_r(NULL, "\n", &state);

    if (nick == NULL || user == NULL || host == NULL || rl_name == NULL) {
	printtext(&ctx, "On issuing event %s: Erroneous server params",
		  compo->command);
	return;
    }

    ctx.window    = g_active_window;
    ctx.spec_type = TYPE_SPEC1;
    printtext(&ctx, "%c%s%c %s%s@%s%s", BOLD, nick, BOLD,
	      LEFT_BRKT, user, host, RIGHT_BRKT);

    if (*rl_name == ':') {
	rl_name++;
    }

    if (*rl_name) {
	printtext(&ctx, "%s %s", Theme("whois_ircName"), rl_name);
    }
}
