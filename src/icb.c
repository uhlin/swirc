/* ICB protocol handling
   Copyright (C) 2019-2024 Markus Uhlin. All rights reserved.

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

#ifdef UNIT_TESTING
#include <setjmp.h>
#include <cmocka.h>
#endif

#include "assertAPI.h"
#include "dataClassify.h"
#include "errHand.h"
#include "icb.h"
#include "irc.h"
#include "libUtils.h"
#include "main.h"
#include "network.h"
#include "printtext.h"
#include "strHand.h"
#include "strdup_printf.h"

#include "events/names.h"

volatile bool g_icb_processing_names = false;

static char	 icb_protolevel[ICB_PACKET_MAX] = { '\0' };
static char	 icb_hostid[ICB_PACKET_MAX] = { '\0' };
static char	 icb_serverid[ICB_PACKET_MAX] = { '\0' };
static char	*icb_group = NULL;

static const char	passed1[] = " has passed moderation to ";
static const char	passed2[] = " just passed you moderation of group ";
static const char	passed3[] = " just passed you moderation of ";

static IDLE_MOD idle_mod = { 0 };

static void	process_event(const char *, ...) PRINTFLIKE(1);
static void	sendpacket(bool *, const char *, ...) PRINTFLIKE(2);

/*lint -printf(1, process_event) */
/*lint -printf(2, sendpacket) */

static const char *
get_label(void)
{
	int		ret;
	static char	label[ICB_PACKET_MAX] = { '\0' };

	if (icb_group == NULL ||
	    (ret = snprintf(label, ARRAY_SIZE(label), "#%s", icb_group)) < 0 ||
	    ((size_t) ret) >= ARRAY_SIZE(label)) {
		debug("%s: error: zeroing label...", __func__);
		BZERO(label, sizeof label);
	}

	return (&label[0]);
}

static void
process_event(const char *format, ...)
{
	char	*event;
	char	*message_concat = NULL;
	enum message_concat_state
		 state = CONCAT_BUFFER_IS_EMPTY;
	va_list	 ap;

	va_start(ap, format);
	event = strdup_vprintf(format, ap);
	va_end(ap);

	irc_handle_interpret_events(event, &message_concat, &state);
	free(event);
}

static void
login_ok(void)
{
	if (strings_match(icb_protolevel, "") ||
	    strings_match(icb_hostid, "") ||
	    strings_match(icb_serverid, "")) {
		printf_and_free(NULL, "%s: empty protocol level, host id or "
		    "server id", __func__);
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
	char		*last = "";
	char		*pktdata_copy = sw_strdup(pktdata);
	const char	*nickname, *message;

	if ((nickname = strtok_r(pktdata_copy, ICB_FIELD_SEP, &last)) == NULL ||
	    (message = strtok_r(NULL, ICB_FIELD_SEP, &last)) == NULL) {
		printf_and_free(pktdata_copy, "%s: too few tokens", __func__);
		return;
	} else if (icb_group == NULL) {
		printf_and_free(pktdata_copy, "%s: not in a group", __func__);
		return;
	}

	process_event(":%s PRIVMSG #%s :%s\r\n", nickname, icb_group, message);

	free(pktdata_copy);
}

static void
handle_personal_msg_packet(const char *pktdata)
{
	char		*last = "";
	char		*pktdata_copy = sw_strdup(pktdata);
	const char	*nickname, *message;

	if ((nickname = strtok_r(pktdata_copy, ICB_FIELD_SEP, &last)) == NULL ||
	    (message = strtok_r(NULL, ICB_FIELD_SEP, &last)) == NULL) {
		printf_and_free(pktdata_copy, "%s: too few tokens", __func__);
		return;
	}

	process_event(":%s PRIVMSG %s :%s\r\n", nickname, g_my_nickname,
	    message);

	free(pktdata_copy);
}

static bool
parsing_ok_idle_mod(const char *nick, const char *group, const char *data)
{
	bool	 yesno;
	char	*str;

	str = strdup_printf("A piano suddenly falls on %s, dislodging "
	    "moderatorship of %s.", nick, group);
	yesno = strings_match(str, data);
	free(str);

	return yesno;
}

/*
 * "A piano suddenly falls on <nick>, dislodging moderatorship of
 * <group>."
 */
static void
deal_with_category_idle_mod(const char *data)
{
	char		*cp;
	char		*group = NULL;
	char		*nick = NULL;
	const char	*dataptr = &data[0];
	const char	*err_reason = "";

	if (strncmp(data, "A piano suddenly falls on ", 26) != STRINGS_MATCH) {
		err_reason = "unexpected leading string";
		goto err;
	}

	dataptr += 26;
	nick = sw_strdup(dataptr);

	if ((cp = strstr(nick, ", dislodging moderatorship of ")) == NULL) {
		err_reason = "cannot locate substring";
		goto err;
	} else {
		*cp = '\0';
	}

	dataptr += strlen(nick);

	if (strncmp(dataptr, ", dislodging moderatorship of ", 30) !=
	    STRINGS_MATCH) {
		err_reason = "string mismatch";
		goto err;
	} else {
		dataptr += 30;
	}

	group = strdup_printf("#%s", dataptr);

	if (group[strlen(group) - 1] == '.')
		group[strlen(group) - 1] = '\0';
	if (!strings_match_ignore_case(group, get_label())) {
		err_reason = "group mismatch";
		goto err;
	} else if (sw_strcpy(idle_mod.nick, nick, ARRAY_SIZE(idle_mod.nick))
	    != 0) {
		err_reason = "cannot store nickname";
		goto err;
	} else if (sw_strcpy(idle_mod.group, group, ARRAY_SIZE(idle_mod.group))
	    != 0) {
		err_reason = "cannot store group";
		goto err;
	} else if (!parsing_ok_idle_mod(nick, group + 1, data)) {
		err_reason = "parsing not ok!";
		goto err;
	}

	process_event(":%s NOTICE %s :%s\r\n", icb_hostid, g_my_nickname, data);

	free(nick);
	free(group);
	return;

  err:
	BZERO(idle_mod.nick, sizeof idle_mod.nick);
	BZERO(idle_mod.group, sizeof idle_mod.group);
	err_log(0, "%s: %s", __func__, err_reason);
	free(nick);
	free(group);
}

static void
deal_with_category_name(const char *data)
{
	const char		*dataptr = &data[0];
	static const char	 changed[] = " changed nickname to ";

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
	bool		 by_server = false;
	char		*nick = NULL;
	const char	*dataptr = &data[0];

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

		immutable_cp_t new_mod = dataptr;

		if (event_names_htbl_lookup(new_mod, window_label) == NULL) {
			goto err;
		} else if (by_server) {
			process_event(":%s MODE #%s +o %s\r\n", &icb_hostid[0],
			    icb_group, new_mod);
		} else {
			process_event(":%s MODE #%s -o+o %s %s\r\n", nick,
			    icb_group, nick, new_mod);
		}
	} else if (strstr(data, passed2) != NULL || strstr(data, passed3) !=
	    NULL) {
		if (by_server) {
			process_event(":%s MODE #%s +o %s\r\n", &icb_hostid[0],
			    icb_group, g_my_nickname);
		} else {
			process_event(":%s MODE #%s -o+o %s %s\r\n", nick,
			    icb_group, nick, g_my_nickname);
		}
	} else {
		sw_assert_not_reached();
	}

  err:
	free(nick);
}

static bool
parsing_ok_timeout(const char *new_mod, const char *data)
{
	bool	 yesno;
	char	*str;

	str = strdup_printf("%s is now mod.", new_mod);
	yesno = strings_match(str, data);
	free(str);

	return yesno;
}

/*
 * "<nick> is now mod."
 */
static void
deal_with_category_timeout(const char *data)
{
	PNAMES		 names;
	char		*cp;
	char		*new_mod = NULL;
	const char	*err_reason = "";

	new_mod = sw_strdup(data);

	if ((cp = strstr(new_mod, " is now mod.")) == NULL) {
		err_reason = "cannot locate substring";
		goto err;
	} else {
		*cp = '\0';
	}

	if (strings_match(idle_mod.nick, "") ||
	    strings_match(idle_mod.group, "")) {
		err_reason = "idle mod: empty nick/group";
		goto err;
	} else if ((names = event_names_htbl_lookup(idle_mod.nick,
	    idle_mod.group)) == NULL) {
		err_reason = "no such mod";
		goto err;
	} else if (!names->is_op) {
		err_reason = "mod not mod";
		goto err;
	} else if (event_names_htbl_lookup(new_mod, idle_mod.group) == NULL) {
		err_reason = "new mod non-existent";
		goto err;
	} else if (!parsing_ok_timeout(new_mod, data)) {
		err_reason = "parsing not ok!";
		goto err;
	}

	process_event(":%s NOTICE %s :%s\r\n", icb_hostid, g_my_nickname, data);
	process_event(":%s MODE %s -o+o %s %s\r\n", icb_hostid, idle_mod.group,
	    idle_mod.nick, new_mod);

	free(new_mod);
	return;

  err:
	err_log(0, "%s: %s", __func__, err_reason);
	free(new_mod);
}

static void
deal_with_category_topic(const char *window_label, const char *data)
{
	PRINTTEXT_CONTEXT	 ctx;
	char			 concat[ICB_PACKET_MAX] = { '\0' };
	const char		*dataptr = &data[0];
	static const char	 changed[] = " changed the topic to \"";

	printtext_context_init(&ctx, NULL, TYPE_SPEC1, true);

	if ((ctx.window = window_by_label(window_label)) == NULL) {
		return;
	} else if (strings_match(data, "The topic is not set.")) {
		printtext(&ctx, "%s", data);
	} else if (!strncmp(data, "The topic is: ", 14)) {
		dataptr += 14;
		process_event(":%s 332 %s #%s :%s\r\n", icb_hostid,
		    g_my_nickname, icb_group, dataptr);
	} else if (strstr(data, changed)) {
		char *nick = sw_strdup(data);
		nick[strcspn(nick, " ")] = '\0';
		PNAMES names = event_names_htbl_lookup(nick, window_label);
		free(nick);
		if (names == NULL ||
		    sw_strcpy(concat, names->nick, ARRAY_SIZE(concat)) != 0 ||
		    sw_strcat(concat, changed, ARRAY_SIZE(concat)) != 0 ||
		    strncmp(data, concat, strlen(concat)) != STRINGS_MATCH)
			return;
		dataptr += strlen(concat);
		char *cp = sw_strdup(dataptr);
		if (cp[strlen(cp) - 1] == '\"')
			cp[strlen(cp) - 1] = '\0';
		process_event(":%s TOPIC #%s :%s\r\n", names->nick, icb_group,
		    cp);
		free(cp);
	}
}

static void
deal_with_category_status(const char *data)
{
	char		*cp = NULL;
	const char	*dataptr = &data[0];

	if (!strncmp(dataptr, "You are now in group ", 21)) {
		if (icb_group) {
			process_event(":%s PART #%s\r\n", g_my_nickname,
			    icb_group);
			free(icb_group);
		}
		dataptr += 21;
		icb_group = sw_strdup(dataptr);
		const bool as_moderator =
		    (cp = strstr(icb_group, " as moderator")) != NULL;
		if (as_moderator)
			*cp = '\0';
		process_event(":%s JOIN :#%s\r\n", g_my_nickname, icb_group);
		icb_send_who(icb_group);
	}
}

/***************************************************
 *
 * Sign-on / Arrive
 *
 ***************************************************/
static void
sign_on_arrive(char *str, const char *sep)
{
	char		*last = "";
	const char	*nick, *user, *host;

	if ((nick = strtok_r(str, sep, &last)) == NULL) {
		err_log(EINVAL, "%s: no nickname", __func__);
		return;
	}
	if ((user = strtok_r(NULL, sep, &last)) == NULL)
		user = "<no user>";
	if ((host = strtok_r(NULL, sep, &last)) == NULL)
		host = "<no host>";
	process_event(":%s!%s@%s JOIN #%s\r\n", nick, user, host, icb_group);
}

/***************************************************
 *
 * Sign-off / Depart
 *
 ***************************************************/
static void
sign_off_depart(char *str, const char *sep)
{
	char		*last = "";
	const char	*nick;

	if (!strncmp(str, "Your group moderator", 20)) {
		/* TODO: Investigate handling */;
	} else if ((nick = strtok_r(str, sep, &last)) == NULL) {
		err_log(EINVAL, "%s: no nickname", __func__);
	} else {
		const char	*user, *host;

		if ((user = strtok_r(NULL, sep, &last)) == NULL)
			user = "<no user>";
		if ((host = strtok_r(NULL, sep, &last)) == NULL)
			host = "<no host>";
		process_event(":%s!%s@%s PART #%s\r\n", nick, user, host,
		    icb_group);
	}
}

static void
handle_status_msg_packet(const char *pktdata)
{
	PRINTTEXT_CONTEXT	 ctx;
	char			*pktdata_copy = sw_strdup(pktdata);
	int			 offset = 0;
	static const char	 sep[] = " (@)";

	printtext_context_init(&ctx, g_status_window, TYPE_SPEC_NONE, true);

	if (!strncmp(pktdata_copy, stat_msg(Boot), 5)) {
		ctx.window	= window_by_label(get_label());
		ctx.spec_type	= TYPE_SPEC3;

		if (ctx.window)
			printtext(&ctx, "*** %s", &pktdata_copy[5]);
	} else if (!strncmp(pktdata_copy, stat_msg(Idle-Mod), 9)) {
		deal_with_category_idle_mod(&pktdata_copy[9]);
	} else if (!strncmp(pktdata_copy, stat_msg(Name), 5)) {
		deal_with_category_name(&pktdata_copy[5]);
	} else if (!strncmp(pktdata_copy, stat_msg(No-Pass), 8)) {
		ctx.spec_type = TYPE_SPEC1;
		printtext(&ctx, "%s", &pktdata_copy[8]);
	} else if (!strncmp(pktdata_copy, stat_msg(Notify), 7)) {
		process_event(":%s NOTICE %s :%s\r\n", icb_hostid,
		    g_my_nickname, &pktdata_copy[7]);

		if (icb_group)
			deal_with_category_pass(get_label(), &pktdata_copy[7]);
	} else if (!strncmp(pktdata_copy, stat_msg(Pass), 5)) {
		deal_with_category_pass(get_label(), &pktdata_copy[5]);
	} else if (!strncmp(pktdata_copy, stat_msg(Sign-on), 8) ||
	    !strncmp(pktdata_copy, stat_msg(Arrive), 7)) {
		offset = (!strncmp(pktdata_copy, stat_msg(Sign-on), 8) ? 8 : 7);

		sign_on_arrive(&pktdata_copy[offset], &sep[0]);
	} else if (!strncmp(pktdata_copy, stat_msg(Sign-off), 9) ||
	    !strncmp(pktdata_copy, stat_msg(Depart), 7)) {
		offset = (!strncmp(pktdata_copy, stat_msg(Sign-off), 9)
		    ? 9 : 7);

		sign_off_depart(&pktdata_copy[offset], &sep[0]);
	} else if (!strncmp(pktdata_copy, stat_msg(Status), 7)) {
		deal_with_category_status(&pktdata_copy[7]);
	} else if (!strncmp(pktdata_copy, stat_msg(Timeout), 8)) {
		deal_with_category_timeout(&pktdata_copy[8]);
	} else if (!strncmp(pktdata_copy, stat_msg(Topic), 6)) {
		deal_with_category_topic(get_label(), &pktdata_copy[6]);
	} else {
		char	*cp;

		while ((cp = strpbrk(pktdata_copy, ICB_FIELD_SEP)) != NULL)
			*cp = 'X';

		ctx.spec_type = TYPE_SPEC1_WARN;
		printtext(&ctx, "%s: unknown status message category",
		    __func__);
		printtext(&ctx, "packet data: %s", pktdata_copy);
	} /* if-then-else */

	free(pktdata_copy);
}

static void
handle_error_msg_packet(const char *pktdata)
{
	PRINTTEXT_CONTEXT	ctx;

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
		printf_and_free(pktdata_copy, "%s: too few tokens", __func__);
		return;
	}

	(void) category; /* unused */

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
	char		*last = "";
	const char	*initial_token, *nickname;
#if 0
	const char	*seconds_idle, *response_time, *login_time, *username,
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
		err_log(EPROTO, "%s", __func__);
		return;
	}

	const bool is_moderator = !strings_match(initial_token, " ");

	process_event(":%s 353 %s = #%s :%s%s\r\n", icb_hostid, g_my_nickname,
	    icb_group, (is_moderator ? "@" : ""), nickname);
}

static void
handle_cmd_output_packet(const char *pktdata)
{
	PIRC_WINDOW		 win = NULL;
	PRINTTEXT_CONTEXT	 ctx;
	char			*pktdata_copy = sw_strdup(pktdata);

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
		printtext(&ctx, "%s: unknown output type", __func__);
		printtext(&ctx, "data: %s", pktdata_copy);
	} /* if-then-else */

	free(pktdata_copy);
}

static void
handle_proto_packet(const char *pktdata)
{
	char		*last = "";
	char		*pktdata_copy = sw_strdup(pktdata);
	const char	*cp;

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
	BZERO(icb_protolevel, sizeof icb_protolevel);
	BZERO(icb_hostid, sizeof icb_hostid);
	BZERO(icb_serverid, sizeof icb_serverid);
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
	printtext(&ctx, "%s: '%c' (length: %d): %s", __func__, type, length,
	    pktdata);
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
		err_log(ENOTCONN, "%s: net_send", __func__);
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
	bool	was_truncated = true;
	size_t	offset = 0;

	while (was_truncated) {
		sendpacket(&was_truncated, "b%s", &text[offset]);

		if (was_truncated) {
			offset += ICB_MESSAGE_MAX;
			offset -= 1;
		}
	}
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

static bool
mod_offset(size_t *offset, const char *to_who)
{
	static const size_t minimum_msgsize = 30;

	*offset += ICB_MESSAGE_MAX;
	*offset -= 2;
	*offset -= strlen(ICB_FIELD_SEP);
	*offset -= strlen(to_who);
	*offset -= 1;

	if (*offset < minimum_msgsize)
		return false;
	return true;
}

void
icb_send_pm(const char *to_who, const char *text)
{
	bool	was_truncated = true;
	size_t	offset = 0;

	while (was_truncated) {
		sendpacket(&was_truncated, "hm%s%s %s", ICB_FIELD_SEP, to_who,
		    &text[offset]);
		if (was_truncated && !mod_offset(&offset, to_who)) {
			PRINTTEXT_CONTEXT ctx;

			printtext_context_init(&ctx, g_active_window,
			    TYPE_SPEC1_FAILURE, true);
			printtext(&ctx, "%s: too long receiver: "
			    "did not complete transfer", __func__);
			err_log(ECANCELED, "%s: too long receiver", __func__);
			break;
		}
	}
}

#ifdef UNIT_TESTING
void
icb_send_pm_test1(void **state)
{
	bool was_truncated = false;
	chararray_t text =
	    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
	    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
	    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
	    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
	    "xxx";
	chararray_t to_who = "t";
	size_t offset = 0;

	sendpacket(&was_truncated, "hm%s%s %s", ICB_FIELD_SEP, to_who,
	    &text[offset]);
	assert_true(was_truncated);
	assert_true(mod_offset(&offset, to_who));
	sendpacket(&was_truncated, "hm%s%s %s", ICB_FIELD_SEP, to_who,
	    &text[offset]);
	assert_false(was_truncated);
	assert_string_equal(&text[offset], "xxx");
}

void
icb_send_pm_test2(void **state)
{
	bool was_truncated = false;
	chararray_t text =
	    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
	    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
	    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
	    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
	    "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
	    "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
	    "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
	    "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
	    "ccc";
	chararray_t to_who = "max";
	size_t offset = 0;

	sendpacket(&was_truncated, "hm%s%s %s", ICB_FIELD_SEP, to_who,
	    &text[offset]);
	assert_true(!strncmp(&text[offset], "AAA", 3));
	assert_true(was_truncated);
	assert_true(mod_offset(&offset, to_who));

	sendpacket(&was_truncated, "hm%s%s %s", ICB_FIELD_SEP, to_who,
	    &text[offset]);
	assert_true(!strncmp(&text[offset], "BBB", 3));
	assert_true(was_truncated);
	assert_true(mod_offset(&offset, to_who));

	sendpacket(&was_truncated, "hm%s%s %s", ICB_FIELD_SEP, to_who,
	    &text[offset]);
	assert_string_equal(&text[offset], "ccc");
	assert_false(was_truncated);
}

/*
 * UNIT_TESTING
 */
#endif

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
