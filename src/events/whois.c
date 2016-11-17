/* whois events
   Copyright (C) 2014-2016 Markus Uhlin. All rights reserved.

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

#include <limits.h>
#include <time.h>

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
    struct time_idle *ti = xcalloc(sizeof *ti, 1);
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

/* event_whois_ssl: 275, 671

   Examples:
     :irc.server.com 275 <issuer> <target> :is connected via SSL (secure link)
     :irc.server.com 671 <issuer> <target> :is using a secure connection */
void
event_whois_ssl(struct irc_message_compo *compo)
{
    char *state = "";
    char *tnick, *msg;
    struct printtext_context ctx;

    ctx.window     = g_status_window;
    ctx.spec_type  = TYPE_SPEC1_WARN;
    ctx.include_ts = true;

    if (Strfeed(compo->params, 2) != 2) {
	printtext(&ctx, "On issuing event %s: Strfeed(..., 2) != 2",
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

/* event_whois_cert: 276

   Example:
     :irc.server.com 276 <issuer> <target> :has client certificate fingerprint
                                            <string> */
void
event_whois_cert(struct irc_message_compo *compo)
{
    char *state = "";
    char *tnick, *msg;
    struct printtext_context ctx;

    ctx.window     = g_status_window;
    ctx.spec_type  = TYPE_SPEC1_WARN;
    ctx.include_ts = true;

    if (Strfeed(compo->params, 2) != 2) {
	printtext(&ctx, "On issuing event %s: Strfeed(..., 2) != 2",
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
	printtext(&ctx, "%s %s %s", Theme("whois_cert"), tnick, msg);
    }
}

/* event_whois_away: 301

   Example:
     :irc.server.com 301 <issuer> <target> :<away reason> */
void
event_whois_away(struct irc_message_compo *compo)
{
    char *away_reason;
    char *state = "";
    struct printtext_context ctx;

    ctx.window     = g_status_window;
    ctx.spec_type  = TYPE_SPEC1_WARN;
    ctx.include_ts = true;

    if (Strfeed(compo->params, 2) != 2) {
	printtext(&ctx, "On issuing event %s: Strfeed(..., 2) != 2",
		  compo->command);
	return;
    }

    (void) strtok_r(compo->params, "\n", &state);
    (void) strtok_r(NULL, "\n", &state);

    if ((away_reason = strtok_r(NULL, "\n", &state)) == NULL) {
	printtext(&ctx, "On issuing event %s: Unable to extract message",
		  compo->command);
	return;
    }

    if (*away_reason == ':') {
	away_reason++;
    }

    if (*away_reason) {
	ctx.window    = g_active_window;
	ctx.spec_type = TYPE_SPEC1;
	printtext(&ctx, "%s %s", Theme("whois_away"), away_reason);
    }
}

/* event_whois_service: 307

   Example:
     :irc.server.com 307 <issuer> <target> :user has identified to services */
void
event_whois_service(struct irc_message_compo *compo)
{
    char *msg;
    char *state = "";
    struct printtext_context ctx;

    ctx.window     = g_status_window;
    ctx.spec_type  = TYPE_SPEC1_WARN;
    ctx.include_ts = true;

    if (Strfeed(compo->params, 2) != 2) {
	printtext(&ctx, "On issuing event %s: Strfeed(..., 2) != 2",
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
    char *nick, *user, *host, *rl_name;
    char *state = "";
    struct printtext_context ctx;

    ctx.window     = g_status_window;
    ctx.spec_type  = TYPE_SPEC1_WARN;
    ctx.include_ts = true;

#if 0
    nick = user = host = rl_name = NULL;
#endif

    if (Strfeed(compo->params, 5) != 5) {
	printtext(&ctx, "On issuing event %s: Strfeed(..., 5) != 5",
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

/* event_whois_server: 312

   Example:
     :irc.server.com 312 <issuer> <target> <server> :<info> */
void
event_whois_server(struct irc_message_compo *compo)
{
    char *srv, *info;
    char *state = "";
    struct printtext_context ctx;

    ctx.window     = g_status_window;
    ctx.spec_type  = TYPE_SPEC1_WARN;
    ctx.include_ts = true;

    if (Strfeed(compo->params, 3) != 3) {
	printtext(&ctx, "On issuing event %s: Strfeed(..., 3) != 3",
		  compo->command);
	return;
    }

    (void) strtok_r(compo->params, "\n", &state);
    (void) strtok_r(NULL, "\n", &state);
    srv  = strtok_r(NULL, "\n", &state);
    info = strtok_r(NULL, "\n", &state);

    if (srv == NULL || info == NULL) {
	printtext(&ctx, "On issuing event %s: Erroneous server params",
		  compo->command);
	return;
    }

    if (*info == ':') {
	info++;
    }

    if (*info) {
	ctx.window    = g_active_window;
	ctx.spec_type = TYPE_SPEC1;
	printtext(&ctx, "%s %s %s%s%s", Theme("whois_server"), srv,
		  LEFT_BRKT, info, RIGHT_BRKT);
    }
}

/* event_whois_ircOp: 313

   Example:
     :irc.server.com 313 <issuer> <target> :is an IRC Operator */
void
event_whois_ircOp(struct irc_message_compo *compo)
{
    char *msg;
    char *state = "";
    struct printtext_context ctx;

    ctx.window     = g_status_window;
    ctx.spec_type  = TYPE_SPEC1_WARN;
    ctx.include_ts = true;

    if (Strfeed(compo->params, 2) != 2) {
	printtext(&ctx, "On issuing event %s: Strfeed(..., 2) != 2",
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
	printtext(&ctx, "%s %s", Theme("whois_ircOp"), msg);
    }
}

/* event_whois_idle: 317

   Example:
     :irc.server.com 317 <issuer> <target> <sec idle> <signon time>
                         :<comment> */
void
event_whois_idle(struct irc_message_compo *compo)
{
    char *ep1, *ep2;
    char *sec_idle_str, *signon_time_str;
    char *state = "";
    long int sec_idle, signon_time;
    struct printtext_context ctx;
    struct time_idle *ti;

    if (Strfeed(compo->params, 4) != 4) {
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

    ctx.window     = g_active_window;
    ctx.spec_type  = TYPE_SPEC1;
    ctx.include_ts = true;
    printtext(&ctx, "%s %ld days %ld hours %ld mins %ld secs %ssignon: %s%s",
	      Theme("whois_idle"), ti->days, ti->hours, ti->mins, ti->secs,
	      LEFT_BRKT, ti->buf, RIGHT_BRKT);
    free(ti);
    return;

  bad:
    ctx.window     = g_status_window;
    ctx.spec_type  = TYPE_SPEC1_WARN;
    ctx.include_ts = true;
    printtext(&ctx, "On issuing event %s: An error occurred", compo->command);
}

/* event_whois_channels: 319

   Example:
     :irc.server.com 319 <issuer> <target> :<channel list> */
void
event_whois_channels(struct irc_message_compo *compo)
{
    char *chan_list;
    char *state = "";
    struct printtext_context ctx;

    ctx.window     = g_status_window;
    ctx.spec_type  = TYPE_SPEC1_WARN;
    ctx.include_ts = true;

    if (Strfeed(compo->params, 2) != 2) {
	printtext(&ctx, "On issuing event %s: Strfeed(..., 2) != 2",
		  compo->command);
	return;
    }

    (void) strtok_r(compo->params, "\n", &state);
    (void) strtok_r(NULL, "\n", &state);

    if ((chan_list = strtok_r(NULL, "\n", &state)) == NULL) {
	printtext(&ctx, "On issuing event %s: Unable to extract message",
		  compo->command);
	return;
    }

    if (*chan_list == ':') {
	chan_list++;
    }

    if (*chan_list) {
	ctx.window    = g_active_window;
	ctx.spec_type = TYPE_SPEC1;
	printtext(&ctx, "%s %s", Theme("whois_channels"), chan_list);
    }
}

/* event_whois_acc: 330

   Example:
     :irc.server.com 330 <issuer> <target> <account name> :is logged in as */
void
event_whois_acc(struct irc_message_compo *compo)
{
    char *account_name, *comment;
    char *state = "";
    struct printtext_context ctx;

    ctx.window     = g_status_window;
    ctx.spec_type  = TYPE_SPEC1_WARN;
    ctx.include_ts = true;

    if (Strfeed(compo->params, 3) != 3) {
	printtext(&ctx, "On issuing event %s: Strfeed(..., 3) != 3",
		  compo->command);
	return;
    }

    (void) strtok_r(compo->params, "\n", &state);
    (void) strtok_r(NULL, "\n", &state);
    account_name = strtok_r(NULL, "\n", &state);
    comment      = strtok_r(NULL, "\n", &state);

    if (account_name == NULL || comment == NULL) {
	printtext(&ctx, "On issuing event %s: Erroneous server params",
		  compo->command);
	return;
    }

    if (*comment == ':') {
	comment++;
    }

    if (*comment) {
	ctx.window    = g_active_window;
	ctx.spec_type = TYPE_SPEC1;
	printtext(&ctx, "%s %s %s", Theme("whois_acc"), comment, account_name);
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
    char *state = "";
    char *str, *str_copy, *cp;
    struct printtext_context ctx;

    if (Strfeed(compo->params, 2) != 2) {
	goto bad;
    }

    (void) strtok_r(compo->params, "\n", &state);
    (void) strtok_r(NULL, "\n", &state);

    if ((str = strtok_r(NULL, "\n", &state)) == NULL) {
	goto bad;
    }

    str_copy = sw_strdup(str);
    cp       = &str_copy[0];
    squeeze(str_copy, ":");

    ctx.window     = g_active_window;
    ctx.spec_type  = TYPE_SPEC1;
    ctx.include_ts = true;
    printtext(&ctx, "%s %s", Theme("whois_host"), cp);
    free(str_copy);
    return;

  bad:
    ctx.window     = g_status_window;
    ctx.spec_type  = TYPE_SPEC1_WARN;
    ctx.include_ts = true;
    printtext(&ctx, "On issuing event %s: An error occurred", compo->command);
}

/* event_whois_conn: 378

   Example:
     :irc.server.com 378 <issuer> <target> :is connecting from
                                            <hostname> <IP> */
void
event_whois_conn(struct irc_message_compo *compo)
{
    char *msg;
    char *state = "";
    struct printtext_context ctx;

    ctx.window     = g_status_window;
    ctx.spec_type  = TYPE_SPEC1_WARN;
    ctx.include_ts = true;

    if (Strfeed(compo->params, 2) != 2) {
	printtext(&ctx, "On issuing event %s: Strfeed(..., 2) != 2",
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
	printtext(&ctx, "%s %s", Theme("whois_conn"), msg);
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
    char *msg;
    char *state = "";
    struct printtext_context ctx;

    ctx.window     = g_status_window;
    ctx.spec_type  = TYPE_SPEC1_WARN;
    ctx.include_ts = true;

    if (Strfeed(compo->params, 2) != 2) {
	printtext(&ctx, "On issuing event %s: Strfeed(..., 2) != 2",
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
	printtext(&ctx, "%s %s", Theme("whois_modes"), msg);
    }
}

/* event_whoReply: 352 (RPL_WHOREPLY)

   Example:
     :irc.server.com 352 <my nick> <channel> <user> <host> <server> <nick>
                         <"H" / "G"> :<hopcount> <real name> */
void
event_whoReply(struct irc_message_compo *compo)
{
    char	*state	  = "";
    char	*channel  = NULL;
    char	*user	  = NULL;
    char	*host	  = NULL;
    char	*server	  = NULL;
    char	*nick	  = NULL;
    char	*symbol	  = NULL;
    char	*hopcount = NULL;
    char	*rl_name  = NULL;
    struct printtext_context ctx = {
	.window	    = g_status_window,
	.spec_type  = TYPE_SPEC1,
	.include_ts = true,
    };

    if (Strfeed(compo->params, 8) != 8)
	goto err;
    (void) strtok_r(compo->params, "\n", &state); /* my nick */
    if ((channel     = strtok_r(NULL, "\n", &state)) == NULL
	|| (user     = strtok_r(NULL, "\n", &state)) == NULL
	|| (host     = strtok_r(NULL, "\n", &state)) == NULL
	|| (server   = strtok_r(NULL, "\n", &state)) == NULL /* unused */
	|| (nick     = strtok_r(NULL, "\n", &state)) == NULL
	|| (symbol   = strtok_r(NULL, "\n", &state)) == NULL
	|| (hopcount = strtok_r(NULL, "\n", &state)) == NULL
	|| (rl_name  = strtok_r(NULL, "\n", &state)) == NULL)
	goto err;
    if (*hopcount == ':')
	hopcount++;
    printtext(&ctx, "%s%s%s%c%s: %s%s%c %s %s %s@%s %s%s%s%c%s",
	      LEFT_BRKT, COLOR1, channel, NORMAL, RIGHT_BRKT,
	      COLOR2, nick, NORMAL,
	      symbol, hopcount, user, host,
	      LEFT_BRKT, COLOR2, rl_name, NORMAL, RIGHT_BRKT);
    return;

err:
    ctx.spec_type = TYPE_SPEC1_FAILURE;
    printtext(&ctx, "On issuing event %s: An error occurred", compo->command);
}
