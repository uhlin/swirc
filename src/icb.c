/* ICB protocol handling
   Copyright (C) 2019-2021 Markus Uhlin. All rights reserved.

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

#include "assertAPI.h"
#include "dataClassify.h"
#include "errHand.h"
#include "icb.h"
#include "irc.h"
#include "libUtils.h"
#include "network.h"
#include "printtext.h"
#include "strHand.h"
#include "strdup_printf.h"

#include "events/names.h"

volatile bool g_icb_processing_names = false;

static char	 icb_protolevel[ICB_PACKET_MAX] = { '\0' };
static char	 icb_hostid[ICB_PACKET_MAX]     = { '\0' };
static char	 icb_serverid[ICB_PACKET_MAX]   = { '\0' };
static char	*icb_group = NULL;

static const char	passed1[] = " has passed moderation to ";
static const char	passed2[] = " just passed you moderation of group ";
static const char	passed3[] = " just passed you moderation of ";

static void	 process_event(const char *, ...) PRINTFLIKE(1);
static void	 sendpacket(bool *, const char *, ...) PRINTFLIKE(2);

/*lint -printf(1, process_event) */
/*lint -printf(2, sendpacket) */

static const char *
get_label(void)
{
    static char label[ICB_PACKET_MAX] = { '\0' };

    memset(label, 0, ARRAY_SIZE(label));

    if (!isNull(icb_group))
	(void) snprintf(label, ARRAY_SIZE(label), "#%s", icb_group);
    return (&label[0]);
}

static void
process_event(const char *format, ...)
{
    char *event = NULL;
    char *message_concat = NULL;
    enum message_concat_state state = CONCAT_BUFFER_IS_EMPTY;
    va_list ap;

    va_start(ap, format);
    event = strdup_vprintf(format, ap);
    va_end(ap);

    irc_handle_interpret_events(event, &message_concat, &state);
    free(event);
}

static void
login_ok(void)
{
    if (strings_match(icb_protolevel, "") || strings_match(icb_hostid, "") ||
	strings_match(icb_serverid, "")) {
	print_and_free("login_ok: empty protocol level, host id or server id",
	    NULL);
	g_on_air = false;
	return;
    }

    /*
     * 001: RPL_WELCOME
     */
    process_event(":%s 001 %s :Welcome to ICB, %s!\r\n", icb_hostid,
	g_my_nickname, g_my_nickname);

    /*
     * 002: RPL_YOURHOST
     */
    process_event(":%s 002 %s :Your host is %s, running version %s\r\n",
	icb_hostid, g_my_nickname, icb_serverid, icb_protolevel);

    /*
     * 375: RPL_MOTDSTART
     */
    process_event(":%s 375 %s :- %s Message Of The Day -\r\n", icb_hostid,
	g_my_nickname, icb_serverid);

    /*
     * 372: RPL_MOTD
     */
    process_event(
	":%s 372 %s :-----------------------------\r\n"
	":%s 372 %s :   Internet Citizen's Band   \r\n"
	":%s 372 %s :-----------------------------\r\n",
	icb_hostid, g_my_nickname,
	icb_hostid, g_my_nickname,
	icb_hostid, g_my_nickname);

    /*
     * 376: RPL_ENDOFMOTD
     */
    process_event(":%s 376 %s :End of MOTD\r\n", icb_hostid, g_my_nickname);
}

static void
handle_open_msg_packet(const char *pktdata)
{
    char	*last         = "";
    char	*message      = NULL;
    char	*nickname     = NULL;
    char	*pktdata_copy = sw_strdup(pktdata);

    nickname = strtok_r(pktdata_copy, ICB_FIELD_SEP, &last);
    message = strtok_r(NULL, ICB_FIELD_SEP, &last);

    if (isNull(nickname) || isNull(message)) {
	print_and_free("handle_open_msg_packet: too few tokens!", pktdata_copy);
	return;
    } else if (isNull(icb_group)) {
	print_and_free("handle_open_msg_packet: not in a group", pktdata_copy);
	return;
    }

    process_event(":%s PRIVMSG #%s :%s\r\n", nickname, icb_group, message);
    free_and_null(&pktdata_copy);
}

static void
handle_personal_msg_packet(const char *pktdata)
{
    char	*last         = "";
    char	*message      = NULL;
    char	*nickname     = NULL;
    char	*pktdata_copy = sw_strdup(pktdata);

    nickname = strtok_r(pktdata_copy, ICB_FIELD_SEP, &last);
    message = strtok_r(NULL, ICB_FIELD_SEP, &last);

    if (isNull(nickname) || isNull(message)) {
	print_and_free("handle_personal_msg_packet: too few tokens!",
	    pktdata_copy);
	return;
    }

    process_event(":%s PRIVMSG %s :%s\r\n", nickname, g_my_nickname, message);
    free_and_null(&pktdata_copy);
}

static void
deal_with_category_name(const char *data)
{
    const char	*dataptr   = &data[0];
    const char	 changed[] = " changed nickname to ";

    if (strstr(data, changed)) {
	char *old_nick = sw_strdup(data);
	old_nick[strcspn(old_nick, " ")] = '\0';
	dataptr += strlen(old_nick);
	dataptr += strlen(changed);
	process_event(":%s NICK :%s\r\n", old_nick, dataptr);
	free(old_nick);
    }
}

static bool
modpass(const char *data)
{
    return (strstr(data, passed1) != NULL ||
	    strstr(data, passed2) != NULL ||
	    strstr(data, passed3) != NULL);
}

static void
deal_with_category_pass(const char *window_label, const char *data)
{
    bool	 by_server = false;
    char	*nick      = NULL;
    const char	*dataptr   = &data[0];

    if (!modpass(data))
	return;
    nick = sw_strdup(data);
    nick[strcspn(nick, " ")] = '\0';
    if (event_names_htbl_lookup(nick, window_label) == NULL) {
	if (!strings_match(nick, "server"))
	    goto err;
	else
	    by_server = true;
    }
    if (strstr(data, passed1)) {
	dataptr += strlen(nick);
	dataptr += strlen(passed1);
	const char *new_mod = dataptr;
	if (event_names_htbl_lookup(new_mod, window_label) == NULL)
	    goto err;
	else if (by_server) {
	    process_event(":%s MODE #%s +o %s\r\n", &icb_hostid[0], icb_group,
		new_mod);
	} else {
	    process_event(":%s MODE #%s -o+o %s %s\r\n", nick, icb_group, nick,
		new_mod);
	}
    } else if (strstr(data, passed2) != NULL ||
	       strstr(data, passed3) != NULL) {
	if (by_server) {
	    process_event(":%s MODE #%s +o %s\r\n", &icb_hostid[0], icb_group,
		g_my_nickname);
	} else {
	    process_event(":%s MODE #%s -o+o %s %s\r\n", nick, icb_group, nick,
		g_my_nickname);
	}
    } else {
	sw_assert_not_reached();
    }

  err:
    free(nick);
}

static void
deal_with_category_topic(const char *window_label, const char *data)
{
    PRINTTEXT_CONTEXT	 ctx;
    char		 concat[ICB_PACKET_MAX] = "";
    const char		*dataptr = &data[0];
    const char		 changed[] = " changed the topic to \"";

    printtext_context_init(&ctx, window_by_label(window_label), TYPE_SPEC1,
	true);

    if (isNull(ctx.window))
	return;
    else if (strings_match(data, "The topic is not set."))
	printtext(&ctx, "%s", data);
    else if (!strncmp(data, "The topic is: ", 14)) {
	dataptr += 14;
	process_event(":%s 332 %s #%s :%s\r\n", icb_hostid, g_my_nickname,
	    icb_group, dataptr);
    } else if (strstr(data, changed)) {
	char *nick = sw_strdup(data);
	nick[strcspn(nick, " ")] = '\0';
	PNAMES names = event_names_htbl_lookup(nick, window_label);
	free(nick);
	if (isNull(names) ||
	    sw_strcpy(concat, names->nick, ARRAY_SIZE(concat)) != 0 ||
	    sw_strcat(concat, changed, ARRAY_SIZE(concat)) != 0 ||
	    strncmp(data, concat, strlen(concat)) != STRINGS_MATCH)
	    return;
	dataptr += strlen(concat);
	char *cp = sw_strdup(dataptr);
	if (cp[strlen(cp) - 1] == '\"')
	    cp[strlen(cp) - 1] = '\0';
	process_event(":%s TOPIC #%s :%s\r\n", names->nick, icb_group, cp);
	free(cp);
    }
}

static void
deal_with_category_status(const char *data)
{
    char	*cp      = NULL;
    const char	*dataptr = &data[0];

    if (!strncmp(dataptr, "You are now in group ", 21)) {
	if (!isNull(icb_group))
	    process_event(":%s PART #%s\r\n", g_my_nickname, icb_group);
	dataptr += 21;
	free_and_null(&icb_group);
	icb_group = sw_strdup(dataptr);
	const bool as_moderator =
	    (cp = strstr(icb_group, " as moderator")) != NULL;
	if (as_moderator)
	    *cp = '\0';
	process_event(":%s JOIN :#%s\r\n", g_my_nickname, icb_group);
	icb_send_who(icb_group);
    }
}

static void
handle_status_msg_packet(const char *pktdata)
{
    PRINTTEXT_CONTEXT	 ctx;
    char		*cp           = NULL;
    char		*last         = "";
    char		*nick         = NULL,
			*user         = NULL,
			*host         = NULL;
    char		*pktdata_copy = sw_strdup(pktdata);
    const char		 sep[]        = " (@)";

    printtext_context_init(&ctx, g_status_window, TYPE_SPEC_NONE, true);

    if (!strncmp(pktdata_copy, "Boot" ICB_FIELD_SEP, 5)) {
	ctx.window = window_by_label(get_label());
	ctx.spec_type = TYPE_SPEC3;

	if (!isNull(ctx.window))
	    printtext(&ctx, "*** %s", &pktdata_copy[5]);
    } else if (!strncmp(pktdata_copy, "Name" ICB_FIELD_SEP, 5)) {
	deal_with_category_name(&pktdata_copy[5]);
    } else if (!strncmp(pktdata_copy, "No-Pass" ICB_FIELD_SEP, 8)) {
	ctx.spec_type = TYPE_SPEC1;
	printtext(&ctx, "%s", &pktdata_copy[8]);
    } else if (!strncmp(pktdata_copy, "Notify" ICB_FIELD_SEP, 7)) {
	process_event(":%s NOTICE %s :%s\r\n", icb_hostid, g_my_nickname,
	    &pktdata_copy[7]);

	if (!isNull(icb_group))
	    deal_with_category_pass(get_label(), &pktdata_copy[7]);
    } else if (!strncmp(pktdata_copy, "Pass" ICB_FIELD_SEP, 5)) {
	deal_with_category_pass(get_label(), &pktdata_copy[5]);
    } else if (!strncmp(pktdata_copy, "Sign-on" ICB_FIELD_SEP, 8) ||
	       !strncmp(pktdata_copy, "Arrive" ICB_FIELD_SEP, 7)) {
/***************************************************
 *
 * Sign-on / Arrive
 *
 ***************************************************/

	const int offset =
	    (!strncmp(pktdata_copy, "Sign-on" ICB_FIELD_SEP, 8) ? 8 : 7);

	if ((nick = strtok_r(&pktdata_copy[offset], sep, &last)) == NULL) {
	    ctx.spec_type = TYPE_SPEC1_FAILURE;
	    printtext(&ctx, "handle_status_msg_packet: during sign-on: "
		"no nick");
	} else {
	    user = strtok_r(NULL, sep, &last);
	    host = strtok_r(NULL, sep, &last);

	    process_event(":%s!%s@%s JOIN #%s\r\n", nick,
		user ? user : "<no user>", host ? host : "<no host>",
		icb_group);
	}
    } else if (!strncmp(pktdata_copy, "Sign-off" ICB_FIELD_SEP, 9) ||
	       !strncmp(pktdata_copy, "Depart" ICB_FIELD_SEP, 7)) {
/***************************************************
 *
 * Sign-off / Depart
 *
 ***************************************************/

	const int offset =
	    (!strncmp(pktdata_copy, "Sign-off" ICB_FIELD_SEP, 9) ? 9 : 7);

	if (!strncmp(&pktdata_copy[offset], "Your group moderator", 20)) {
	    ;
	} else if ((nick = strtok_r(&pktdata_copy[offset],sep,&last)) == NULL) {
	    ctx.spec_type = TYPE_SPEC1_FAILURE;
	    printtext(&ctx, "handle_status_msg_packet: during sign-off: "
		"no nick");
	} else {
	    user = strtok_r(NULL, sep, &last);
	    host = strtok_r(NULL, sep, &last);

	    process_event(":%s!%s@%s PART #%s\r\n", nick,
		user ? user : "<no user>", host ? host : "<no host>",
		icb_group);
	}
    } else if (!strncmp(pktdata_copy, "Status" ICB_FIELD_SEP, 7)) {
	deal_with_category_status(&pktdata_copy[7]);
    } else if (!strncmp(pktdata_copy, "Topic" ICB_FIELD_SEP, 6)) {
	deal_with_category_topic(get_label(), &pktdata_copy[6]);
    } else {
	while ((cp = strpbrk(pktdata_copy, ICB_FIELD_SEP)) != NULL)
	    *cp = 'X';
	ctx.spec_type = TYPE_SPEC1_WARN;
	printtext(&ctx, "handle_status_msg_packet: "
	    "unknown status message category");
	printtext(&ctx, "packet data: %s", pktdata_copy);
    } /* if-then-else */

    free_and_null(&pktdata_copy);
}

static void
handle_error_msg_packet(const char *pktdata)
{
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_FAILURE, true);
    printtext(&ctx, "%s", pktdata);
}

static void
handle_important_msg_packet(const char *pktdata)
{
	char	*category, *msgtext;
	char	*last = "";
	char	*pktdata_copy = sw_strdup(pktdata);

	if ((category = strtok_r(pktdata_copy, ICB_FIELD_SEP, &last)) == NULL ||
	    (msgtext = strtok_r(NULL, ICB_FIELD_SEP, &last)) == NULL) {
		print_and_free("handle_important_msg_packet: too few tokens!",
		    pktdata_copy);
		return;
	}

	process_event(":%s NOTICE %s :%s%s%s\r\n", icb_hostid, g_my_nickname,
	    TXT_BOLD, squeeze_text_deco(msgtext), TXT_BOLD);

	free(pktdata_copy);
}

static void
handle_exit_packet(void)
{
	process_event("ERROR :Closing Link: Received exit packet\r\n");
}

static void
who_listing(char *cp)
{
	PIRC_WINDOW	 win;
	char		*initial_token, *nickname;
	char		*last = "";
#if 0
	char		*seconds_idle, *response_time, *login_time, *username,
			*userhost, *reg_status;
#endif

	if ((win = window_by_label(get_label())) != NULL &&
	    win->received_names) {
		return;
	} else if ((initial_token = strtok_r(cp, ICB_FIELD_SEP, &last)) == NULL
	    || (nickname = strtok_r(NULL, ICB_FIELD_SEP, &last)) == NULL) {
		/*
		 * Missing essential initial token or nickname during
		 * who listing.
		 */
		err_log(EPROTO, "who_listing");
		return;
	}

	const bool is_moderator = !strings_match(initial_token, " ");

	process_event(":%s 353 %s = #%s :%s%s\r\n", icb_hostid, g_my_nickname,
	    icb_group, (is_moderator ? "@" : ""), nickname);
}

static void
handle_cmd_output_packet(const char *pktdata)
{
	PIRC_WINDOW win = NULL;
	PRINTTEXT_CONTEXT ctx;
	char *pktdata_copy = sw_strdup(pktdata);

	printtext_context_init(&ctx, g_status_window, TYPE_SPEC_NONE, true);

	if (!strncmp(pktdata_copy, "co", 2)) {
		/*
		 * Generic command output
		 */

		squeeze(pktdata_copy, ICB_FIELD_SEP);
		ctx.spec_type = TYPE_SPEC1;
		printtext(&ctx, "%s", &pktdata_copy[2]);

		if ((win = window_by_label(get_label())) != NULL &&
		    !(win->received_names)) {
			char	str[ICB_PACKET_MAX] = { '\0' };
			int	ret;

			if ((ret = snprintf(str, ARRAY_SIZE(str), "Group: %s",
			    icb_group)) < 0 || ((size_t) ret) >=
			    ARRAY_SIZE(str)) {
				err_log(ENOBUFS, "generic command output");
			} else if (!strncmp(&pktdata_copy[2], str,
			    strlen(str))) {
				(void) atomic_swap_bool(&g_icb_processing_names,
				    true);
			}
		}
	} else if (!strncmp(pktdata_copy, "wh", 2)) {
		debug("Tell client to output header for who listing. "
		    "Deprecated.");
	} else if (!strncmp(pktdata_copy, "wl", 2)) {
		who_listing(&pktdata_copy[2]);
	} else {
		/*
		 * Unknown output type
		 */

		char *cp;

		while ((cp = strpbrk(pktdata_copy, ICB_FIELD_SEP)) != NULL)
			*cp = 'X';

		ctx.spec_type = TYPE_SPEC1_WARN;
		printtext(&ctx, "handle_cmd_output_packet: "
		    "unknown output type");
		printtext(&ctx, "data: %s", pktdata_copy);
	} /* if-then-else */

	free(pktdata_copy);
}

static void
handle_proto_packet(const char *pktdata)
{
	char	*cp;
	char	*last = "";
	char	*pktdata_copy = sw_strdup(pktdata);

	if ((cp = strtok_r(pktdata_copy, ICB_FIELD_SEP, &last)) == NULL ||
	    sw_strcpy(icb_protolevel, cp, ARRAY_SIZE(icb_protolevel)) != 0)
		goto err;
	else if ((cp = strtok_r(NULL, ICB_FIELD_SEP, &last)) == NULL ||
	    sw_strcpy(icb_hostid, cp, ARRAY_SIZE(icb_hostid)) != 0)
		goto err;
	else if ((cp = strtok_r(NULL, ICB_FIELD_SEP, &last)) == NULL ||
	    sw_strcpy(icb_serverid, cp, ARRAY_SIZE(icb_serverid)) != 0)
		goto err;

	free(pktdata_copy);
	return;

  err:
	free(pktdata_copy);
	BZERO(icb_protolevel, ARRAY_SIZE(icb_protolevel));
	BZERO(icb_hostid, ARRAY_SIZE(icb_hostid));
	BZERO(icb_serverid, ARRAY_SIZE(icb_serverid));
}

static void
handle_beep_packet(const char *pktdata)
{
	PRINTTEXT_CONTEXT	ctx;

	if (pktdata == NULL || strings_match(pktdata, ""))
		return;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_WARN, true);
	printtext(&ctx, "You were beeped by %s%s%s",
	    TXT_BOLD, pktdata, TXT_BOLD);
}

static void
handle_ping_packet(const char *pktdata)
{
	icb_send_pong(pktdata);
}

static void
unknown_packet_type(const int length, const char type, const char *pktdata)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_FAILURE, true);
	printtext(&ctx, "unknown_packet_type: '%c' (length: %d): %s", type,
	    length, pktdata);
}

void
icb_irc_proxy(const int length, const char type, const char *pktdata)
{
	switch (type) {
	case 'a':
		login_ok();
		break;
	case 'b':
		handle_open_msg_packet(pktdata);
		break;
	case 'c':
		handle_personal_msg_packet(pktdata);
		break;
	case 'd':
		handle_status_msg_packet(pktdata);
		break;
	case 'e':
		handle_error_msg_packet(pktdata);
		break;
	case 'f':
		handle_important_msg_packet(pktdata);
		break;
	case 'g':
		handle_exit_packet();
		break;
	case 'i':
		handle_cmd_output_packet(pktdata);
		break;
	case 'j':
		handle_proto_packet(pktdata);
		break;
	case 'k':
		handle_beep_packet(pktdata);
		break;
	case 'l':
		handle_ping_packet(pktdata);
		break;
	default:
		unknown_packet_type(length, type, pktdata);
		break;
	}
}

void
icb_process_event_eof_names(void)
{
	process_event(":%s 366 %s #%s :End of names\r\n", icb_hostid,
	    g_my_nickname, icb_group);
	(void) atomic_swap_bool(&g_icb_processing_names, false);
}

static void
sendpacket(bool *was_truncated, const char *format, ...)
{
	char msg[ICB_MESSAGE_MAX] = { '\0' };
	int ret;
	va_list ap;

	if (!isNull(was_truncated))
		*was_truncated = false;

	va_start(ap, format);
	ret = vsnprintf(msg, ARRAY_SIZE(msg), format, ap);
	va_end(ap);

	if (ret < 0 || ((size_t) ret) >= ARRAY_SIZE(msg)) {
		if (!isNull(was_truncated))
			*was_truncated = true;
	}

	const int msglen = (int) strlen(msg);

	if (net_send("%c%s", msglen, msg) < 0)
		err_log(ENOTCONN, "sendpacket: net_send");
}

void
icb_send_beep(const char *arg)
{
	sendpacket(NULL, "hbeep%s%s", ICB_FIELD_SEP, arg);
}

void
icb_send_boot(const char *victim)
{
	sendpacket(NULL, "hboot%s%s", ICB_FIELD_SEP, victim);
}

void
icb_send_group(const char *group)
{
	sendpacket(NULL, "hg%s%s", ICB_FIELD_SEP, group);
}

void
icb_send_name(const char *new_nick)
{
	sendpacket(NULL, "hname%s%s", ICB_FIELD_SEP, new_nick);
}

void
icb_send_noop(void)
{
	sendpacket(NULL, "n");
}

void
icb_send_open_msg(const char *text)
{
	bool	was_truncated = false;

	sendpacket(&was_truncated, "b%s", text);

	if (was_truncated)
		err_log(ENOBUFS, "icb_send_open_msg: text truncated");
}

/*
 * Pass moderation
 */
void
icb_send_pass_mod(const char *to_who)
{
	if (to_who != NULL && !strings_match(to_who, ""))
		sendpacket(NULL, "hpass%s%s", ICB_FIELD_SEP, to_who);
}

void
icb_send_ping(const char *arg)
{
	if (arg != NULL && !strings_match(arg, ""))
		sendpacket(NULL, "l%s", arg);
	else
		sendpacket(NULL, "l");
}

void
icb_send_pm(const char *to_who, const char *text)
{
	bool	was_truncated = false;

	sendpacket(&was_truncated, "hm%s%s %s", ICB_FIELD_SEP, to_who, text);

	if (was_truncated)
		err_log(ENOBUFS, "icb_send_pm: text truncated");
}

void
icb_send_pong(const char *arg)
{
	if (arg != NULL && !strings_match(arg, ""))
		sendpacket(NULL, "m%s", arg);
	else
		sendpacket(NULL, "m");
}

void
icb_send_topic(const char *new_topic)
{
	sendpacket(NULL, "htopic%s%s", ICB_FIELD_SEP, new_topic);
}

void
icb_send_who(const char *arg)
{
	sendpacket(NULL, "hw%s%s", ICB_FIELD_SEP, arg);
}
