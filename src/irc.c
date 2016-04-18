/* Handle and interpret IRC events
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

#include "assertAPI.h"
#include "dataClassify.h"
#include "errHand.h"
#include "irc.h"
#include "libUtils.h"
#include "network.h"
#include "printtext.h"
#include "strHand.h"

#include "events/channel.h"
#include "events/error.h"
#include "events/misc.h"
#include "events/motd.h"
#include "events/names.h"
#include "events/noop.h"
#include "events/notice.h"
#include "events/ping.h"
#include "events/privmsg.h"
#include "events/welcome.h"
#include "events/whois.h"

/* Objects with external linkage
   ============================= */

char	*g_server_hostname = NULL;
char	*g_my_nickname     = NULL;
bool	 g_alt_nick_tested = false;

/* Objects with internal linkage
   ============================= */

static struct normal_events_tag {
    char		*normal_event;
    event_handler_fn	 event_handler;
} normal_events[] = {
    { "NICK",    event_nick    },
    { "QUIT",    event_quit    },
    { "SQUIT",   event_noop    },
    { "JOIN",    event_join    },
    { "PART",    event_part    },
    { "MODE",    event_noop    },
    { "TOPIC",   event_noop    },
    { "INVITE",  event_noop    },
    { "KICK",    event_kick    },
    { "PRIVMSG", event_privmsg },
    { "NOTICE",  event_notice  },
    { "PING",    event_ping    },
    { "ERROR",   event_error   },
    { "WALLOPS", event_noop    },
};

static struct numeric_events_tag {
    char             *numeric_event;
    char             *official_name;
    enum to_window    window;
    int               ext_bits;
    event_handler_fn  event_handler;
} numeric_events[] = {
    { "001", "RPL_WELCOME",             NO_WINDOW,      0, event_welcome },
    { "002", "RPL_YOURHOST",            STATUS_WINDOW,  1, NULL },
    { "003", "RPL_CREATED",             STATUS_WINDOW,  1, NULL },
    { "004", "RPL_MYINFO",              STATUS_WINDOW,  1, NULL },
    { "005", "RPL_BOUNCE",              NO_WINDOW,      0, event_bounce },
    { "020", "",                        STATUS_WINDOW,  1, NULL },
    { "042", "",                        NO_WINDOW,      0, event_allaround_extract_remove_colon },
    { "221", "RPL_UMODEIS",             STATUS_WINDOW,  1, NULL },
    { "250", "",                        STATUS_WINDOW,  1, NULL },
    { "251", "RPL_LUSERCLIENT",         STATUS_WINDOW,  1, NULL },
    { "252", "RPL_LUSEROP",             NO_WINDOW,      0, event_allaround_extract_remove_colon },
    { "253", "RPL_LUSERUNKNOWN",        NO_WINDOW,      0, event_allaround_extract_remove_colon },
    { "254", "RPL_LUSERCHANNELS",       NO_WINDOW,      0, event_allaround_extract_remove_colon },
    { "255", "RPL_LUSERME",             STATUS_WINDOW,  1, NULL },
    { "265", "",                        NO_WINDOW,      0, event_local_and_global_users },
    { "266", "",                        NO_WINDOW,      0, event_local_and_global_users },
    { "275", "",                        NO_WINDOW,      0, event_whois_ssl },
    { "276", "",                        NO_WINDOW,      0, event_whois_cert },
    { "301", "RPL_AWAY",                NO_WINDOW,      0, event_whois_away },
    { "307", "",                        NO_WINDOW,      0, event_whois_service },
    { "310", "",                        NO_WINDOW,      0, event_whois_modes },
    { "311", "RPL_WHOISUSER",           NO_WINDOW,      0, event_whois_user },
    { "312", "RPL_WHOISSERVER",         NO_WINDOW,      0, event_whois_server },
    { "313", "RPL_WHOISOPERATOR",       NO_WINDOW,      0, event_whois_ircOp },
    { "314", "RPL_WHOWASUSER",          NO_WINDOW,      0, event_whois_user },
    { "317", "RPL_WHOISIDLE",           NO_WINDOW,      0, event_whois_idle },
    { "318", "RPL_ENDOFWHOIS",          ACTIVE_WINDOW,  2, NULL },
    { "319", "RPL_WHOISCHANNELS",       NO_WINDOW,      0, event_whois_channels },
    { "330", "",                        NO_WINDOW,      0, event_whois_acc },
    { "332", "RPL_TOPIC",               NO_WINDOW,      0, event_topic },
    { "338", "",                        NO_WINDOW,      0, event_whois_host },
    { "353", "RPL_NAMREPLY",            NO_WINDOW,      0, event_names },
    { "366", "RPL_ENDOFNAMES",          NO_WINDOW,      0, event_eof_names },
    { "369", "RPL_ENDOFWHOWAS",         ACTIVE_WINDOW,  2, NULL },
    { "372", "RPL_MOTD",                NO_WINDOW,      0, event_motd },
    { "375", "RPL_MOTDSTART",           NO_WINDOW,      0, event_motd },
    { "376", "RPL_ENDOFMOTD",           NO_WINDOW,      0, event_motd },
    { "378", "",                        NO_WINDOW,      0, event_whois_conn },
    { "379", "",                        NO_WINDOW,      0, event_whois_modes },
    { "396", "",                        NO_WINDOW,      0, event_allaround_extract_remove_colon },
    { "404", "ERR_CANNOTSENDTOCHAN",    ACTIVE_WINDOW,  2, NULL },
    { "422", "ERR_NOMOTD",              STATUS_WINDOW,  1, NULL },
    { "433", "ERR_NICKNAMEINUSE",       NO_WINDOW,      0, event_nicknameInUse },
    { "439", "",                        STATUS_WINDOW,  1, NULL },
    { "470", "",                        NO_WINDOW,      0, event_channel_forward },
    { "615", "",                        NO_WINDOW,      0, event_whois_modes },
    { "616", "",                        NO_WINDOW,      0, event_whois_host },
    { "671", "",                        NO_WINDOW,      0, event_whois_ssl },
};

/* Function declarations
   ===================== */

static char	*get_last_token(const char *buffer);
static void	 irc_search_and_route_event(struct irc_message_compo *);

static void
FreeMsgCompo(struct irc_message_compo *);
static struct irc_message_compo *
SortMsgCompo(char *protocol_message, bool message_has_prefix);

void
irc_handle_interpret_events(char *recvbuffer,
			    char **message_concat,
			    enum message_concat_state *state)
{
    const char  separators[] = "\r\n";
    bool        terminated_recvchunk;
    char       *last_token = NULL;
    char       *cp, *savp = "";
    long int    loop_count;

    if (recvbuffer == NULL || (*state != CONCAT_BUFFER_IS_EMPTY && *state != CONCAT_BUFFER_CONTAIN_DATA)) {
	err_exit(EINVAL, "irc_handle_interpret_events error");
    } else if (Strings_match(recvbuffer, "") || strpbrk(recvbuffer, separators) == NULL) {
	return;
    } else {
	;
    }

    switch (recvbuffer[strlen(recvbuffer) - 1]) {
    case '\r':
    case '\n':
	terminated_recvchunk = true;
	break;
    default:
	terminated_recvchunk = false;
	break;
    }

    if (!terminated_recvchunk) {
	last_token = get_last_token(recvbuffer); /* Must be freed */
    }

    for (cp = &recvbuffer[0], loop_count = 0;; cp = NULL, loop_count++) {
	char *token;
	int message_has_prefix, requested_feeds;
	char *protocol_message;
	struct irc_message_compo *compo;

	if ((token = strtok_r(cp, separators, &savp)) == NULL) {
	    break; /* No more tokens  --  end loop... */
	} else if (last_token != NULL && *state == CONCAT_BUFFER_IS_EMPTY &&
		   Strings_match(token, last_token)) {
	    free_and_null(&(*message_concat));
	    *message_concat = sw_strdup(last_token);
	    free(last_token);
	    *state = CONCAT_BUFFER_CONTAIN_DATA;	/* On the next call to this function the (incomplete) */
	    return;					/* irc message will be concatenated */
	} else if (loop_count == 0 && *state == CONCAT_BUFFER_CONTAIN_DATA) {
            *state = CONCAT_BUFFER_IS_EMPTY;		/* Flag it as empty again (in practice it isn't) */
	    realloc_strcat(&(*message_concat), token);
	    token = *message_concat;			/* The special token can now be passed to the handler */
	} else {
	    ;
	}

	message_has_prefix = *(protocol_message = sw_strdup(token)) == ':';
	requested_feeds = message_has_prefix ? 2 : 1;
	if (Strfeed(protocol_message, requested_feeds) != requested_feeds) {
	    struct printtext_context ptext_ctx = {
		.window     = g_status_window,
		.spec_type  = TYPE_SPEC1_FAILURE,
		.include_ts = true,
	    };

	    printtext(&ptext_ctx,
		      "In irc_handle_interpret_events: Strfeed(..., %d) != %d",
		      requested_feeds, requested_feeds);
	    free(protocol_message);
	    continue;
	}
	compo = SortMsgCompo(protocol_message, message_has_prefix);
	free(protocol_message);
	irc_search_and_route_event(compo);
	FreeMsgCompo(compo);
    }
}

static char *
get_last_token(const char *buffer)
{
    const char *last_token = strrchr(buffer, '\n');

    if (last_token == NULL) {
	err_msg("get_last_token error. (this is a bug and shouldn't happen!)");
	abort();
    }

    return (sw_strdup(++last_token));
}

static struct irc_message_compo *
SortMsgCompo(char *protocol_message, bool message_has_prefix)
{
    struct irc_message_compo *compo = xcalloc(sizeof *compo, 1);
    short int loop_run;
    char *cp, *savp = "";

    compo->prefix  = NULL;
    compo->command = NULL;
    compo->params  = NULL;

    for (loop_run = 0, cp = &protocol_message[0];; loop_run++, cp = NULL) {
	char *token;

	if ((token = strtok_r(cp, "\n", &savp)) == NULL) {
	    break;
	}

	switch (loop_run) {
	case 0:
	    if (message_has_prefix) {
		compo->prefix = sw_strdup(token);
	    } else {
		compo->command = sw_strdup(token);
	    }
	    break; /* loop_run 0 */
	case 1:
	    if (message_has_prefix) {
		compo->command = sw_strdup(token);
	    } else {
		compo->params = sw_strdup(token);
	    }
	    break; /* loop_run 1 */
	case 2:
	    compo->params = sw_strdup(token);
	    break; /* loop_run 2 */
	default:
	    sw_assert_not_reached();
	}

	if (compo->command != NULL && compo->params != NULL) {
	    break;
	}
    }

    sw_assert(compo->command != NULL && compo->params != NULL);
    return (compo);
}

static void
FreeMsgCompo(struct irc_message_compo *compo)
{
    free_not_null(compo->prefix);
    free_not_null(compo->command);
    free_not_null(compo->params);

    free(compo);
}

/* Set to 1 for extended info. Intended for debugging only */
#define UNKNOWN_EVENT_DISPLAY_EXTENDED_INFO 1

static void
irc_search_and_route_event(struct irc_message_compo *compo)
{
    struct printtext_context ptext_ctx = {
	.window     = g_status_window,
	.spec_type  = TYPE_SPEC1_WARN,
	.include_ts = true,
    };

    if (is_alphabetic(compo->command)) {
	struct normal_events_tag *sp;
	const size_t size = ARRAY_SIZE(normal_events);

	for (sp = &normal_events[0]; sp < &normal_events[size]; sp++) {
	    if (Strings_match(sp->normal_event, compo->command)) {
		sp->event_handler(compo);
		return;
	    }
	}

	printtext(&ptext_ctx, "Unknown normal event: %s", compo->command);
#if UNKNOWN_EVENT_DISPLAY_EXTENDED_INFO
	printtext(&ptext_ctx, "params = %s", compo->params);
	printtext(&ptext_ctx, "prefix = %s", compo->prefix ? compo->prefix : "none");
#endif
    } else if (is_numeric(compo->command) && strlen(compo->command) == 3) {
	struct numeric_events_tag *sp;
	const size_t size = ARRAY_SIZE(numeric_events);

	for (sp = &numeric_events[0]; sp < &numeric_events[size]; sp++) {
	    if (Strings_match(sp->numeric_event, compo->command)) {
		if (sp->event_handler != NULL) {
		    sp->event_handler(compo);
		} else {
		    if (sp->window == STATUS_WINDOW)
			irc_extract_msg(compo, g_status_window, sp->ext_bits);
		    else if (sp->window == ACTIVE_WINDOW)
			irc_extract_msg(compo, g_active_window, sp->ext_bits);
		    else
			sw_assert_not_reached();
		}

		return;
	    }
	}

	printtext(&ptext_ctx, "Unknown numeric event: %s", compo->command);
#if UNKNOWN_EVENT_DISPLAY_EXTENDED_INFO
	printtext(&ptext_ctx, "params = %s", compo->params);
	printtext(&ptext_ctx, "prefix = %s", compo->prefix ? compo->prefix : "none");
#endif
    } else {
	printtext(&ptext_ctx, "Erroneous event: %s", compo->command);
    }
}

void
irc_extract_msg(struct irc_message_compo *compo, PIRC_WINDOW to_window, int ext_bits)
{
    int i;
    char *cp, *savp = "";

    if (Strfeed(compo->params, ext_bits) != ext_bits) {
	struct printtext_context ptext_ctx = {
	    .window     = g_status_window,
	    .spec_type  = TYPE_SPEC1_FAILURE,
	    .include_ts = true,
	};

	printtext(&ptext_ctx, "In irc_extract_msg: Strfeed(..., %d) != %d", ext_bits, ext_bits);
	return;
    }

    for (i = 0, cp = &compo->params[0];; i++, cp = NULL) {
	char *token;

	if ((token = strtok_r(cp, "\n", &savp)) == NULL) {
	    break;
	} else if (i == ext_bits) {
	    if (*token == ':') {
		token++;
	    }

	    if (*token) {
		struct printtext_context ptext_ctx = {
		    .window     = to_window,
		    .spec_type  = TYPE_SPEC1,
		    .include_ts = true,
		};

		printtext(&ptext_ctx, "%s", token);
		return;
	    }
	} else {
	    ;
	}
    }
}

void
irc_init(void)
{
    event_names_init();
}

void
irc_deinit(void)
{
    free_and_null(&g_server_hostname);
    free_and_null(&g_my_nickname);

    g_alt_nick_tested = false;

    event_names_deinit();
}

void
irc_set_server_hostname(const char *srv_host)
{
    if (srv_host == NULL || Strings_match(srv_host, "")) {
	err_exit(EINVAL, "irc_set_server_hostname error");
    }

    if (g_server_hostname) {
	free(g_server_hostname);
    }

    g_server_hostname = sw_strdup(srv_host);
}

void
irc_set_my_nickname(const char *nick)
{
    if (nick == NULL || Strings_match(nick, "")) {
	err_exit(EINVAL, "irc_set_my_nickname error");
    }

    if (g_my_nickname) {
	free(g_my_nickname);
    }

    g_my_nickname = sw_strdup(nick);
}

/* Function used to clean up within an event after a failure that is
 * fatal enough to prefer IRC shutdown */
void
irc_unsuccessful_event_cleanup(void)
{
    (void) net_send(g_socket, 0, "QUIT");
    g_on_air = false;
}
