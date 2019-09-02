/* Handle and interpret IRC events
   Copyright (C) 2014-2019 Markus Uhlin. All rights reserved.

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
#include "config.h"
#include "dataClassify.h"
#include "errHand.h"
#include "irc.h"
#include "libUtils.h"
#include "main.h"
#include "network.h"
#include "printtext.h"
#include "readline.h"		/* readline_top_panel() */
#include "statusbar.h"
#include "strHand.h"

#include "events/account.h"
#include "events/auth.h"
#include "events/away.h"
#include "events/banlist.h"
#include "events/cap.h"
#include "events/channel.h"
#include "events/error.h"
#include "events/invite.h"
#include "events/list.h"
#include "events/misc.h"
#include "events/motd.h"
#include "events/names.h"
#include "events/noop.h"
#include "events/notice.h"
#include "events/ping.h"
#include "events/pong.h"
#include "events/privmsg.h"
#include "events/wallops.h"
#include "events/welcome.h"
#include "events/whois.h"

/* Set to 1 for extended info. Intended for debugging only */
#define UNKNOWN_EVENT_DISPLAY_EXTENDED_INFO 1

/****************************************************************
*                                                               *
*  -------------- Objects with external linkage --------------  *
*                                                               *
****************************************************************/

bool	 g_alt_nick_tested = false;
bool	 g_am_irc_op       = false;
bool	 g_is_away         = false;
char	*g_my_nickname     = NULL;
char	*g_server_hostname = NULL;

/****************************************************************
*                                                               *
*  -------------- Objects with internal linkage --------------  *
*                                                               *
****************************************************************/

static struct normal_events_tag {
    char		*normal_event;
    event_handler_fn	 event_handler;
} normal_events[] = {
    { "ACCOUNT",      event_account      },
    { "AUTHENTICATE", event_authenticate },
    { "AWAY",         event_away         },
    { "CAP",          event_cap          },
    { "ERROR",        event_error        },
    { "INVITE",       event_invite       },
    { "JOIN",         event_join         },
    { "KICK",         event_kick         },
    { "MODE",         event_mode         },
    { "NICK",         event_nick         },
    { "NOTICE",       event_notice       },
    { "PART",         event_part         },
    { "PING",         event_ping         },
    { "PONG",         event_pong         },
    { "PRIVMSG",      event_privmsg      },
    { "QUIT",         event_quit         },
    { "SQUIT",        event_noop         },
    { "TOPIC",        event_topic_chg    },
    { "WALLOPS",      event_wallops      },
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
    { "005", "RPL_ISUPPORT",            NO_WINDOW,      0, event_serverFeatures },
    { "020", "",                        STATUS_WINDOW,  1, NULL },
    { "042", "",                        NO_WINDOW,      0, event_allaround_extract_remove_colon },
    { "221", "RPL_UMODEIS",             NO_WINDOW,      0, event_userModeIs },
    { "232", "",                        ACTIVE_WINDOW,  1, NULL },
    { "250", "",                        STATUS_WINDOW,  1, NULL },
    { "251", "RPL_LUSERCLIENT",         STATUS_WINDOW,  1, NULL },
    { "252", "RPL_LUSEROP",             NO_WINDOW,      0, event_allaround_extract_remove_colon },
    { "253", "RPL_LUSERUNKNOWN",        NO_WINDOW,      0, event_allaround_extract_remove_colon },
    { "254", "RPL_LUSERCHANNELS",       NO_WINDOW,      0, event_allaround_extract_remove_colon },
    { "255", "RPL_LUSERME",             STATUS_WINDOW,  1, NULL },
    { "263", "RPL_TRYAGAIN",            ACTIVE_WINDOW,  2, NULL },
    { "265", "",                        NO_WINDOW,      0, event_local_and_global_users },
    { "266", "",                        NO_WINDOW,      0, event_local_and_global_users },
    { "275", "",                        NO_WINDOW,      0, event_whois_ssl },
    { "276", "",                        NO_WINDOW,      0, event_whois_cert },
    { "301", "RPL_AWAY",                NO_WINDOW,      0, event_whois_away },
    { "305", "RPL_UNAWAY",              NO_WINDOW,      0, event_unaway },
    { "306", "RPL_NOWAWAY",             NO_WINDOW,      0, event_nowAway },
    { "307", "",                        NO_WINDOW,      0, event_whois_service },
    { "308", "",                        ACTIVE_WINDOW,  1, NULL },
    { "309", "",                        ACTIVE_WINDOW,  1, NULL },
    { "310", "",                        NO_WINDOW,      0, event_whois_modes },
    { "311", "RPL_WHOISUSER",           NO_WINDOW,      0, event_whois_user },
    { "312", "RPL_WHOISSERVER",         NO_WINDOW,      0, event_whois_server },
    { "313", "RPL_WHOISOPERATOR",       NO_WINDOW,      0, event_whois_ircOp },
    { "314", "RPL_WHOWASUSER",          NO_WINDOW,      0, event_whois_user },
    { "315", "RPL_ENDOFWHO",            STATUS_WINDOW,  2, NULL },
    { "317", "RPL_WHOISIDLE",           NO_WINDOW,      0, event_whois_idle },
    { "318", "RPL_ENDOFWHOIS",          ACTIVE_WINDOW,  2, NULL },
    { "319", "RPL_WHOISCHANNELS",       NO_WINDOW,      0, event_whois_channels },
    { "321", "RPL_LISTSTART",           NO_WINDOW,      0, event_liststart },
    { "322", "RPL_LIST",                NO_WINDOW,      0, event_list },
    { "323", "RPL_LISTEND",             STATUS_WINDOW,  1, NULL },
    { "324", "RPL_CHANNELMODEIS",       NO_WINDOW,      0, event_channelModeIs },
    { "328", "",                        NO_WINDOW,      0, event_chan_hp },
    { "329", "",                        NO_WINDOW,      0, event_channelCreatedWhen },
    { "330", "",                        NO_WINDOW,      0, event_whois_acc },
    { "331", "RPL_NOTOPIC",             ACTIVE_WINDOW,  2, NULL },
    { "332", "RPL_TOPIC",               NO_WINDOW,      0, event_topic },
    { "333", "",                        NO_WINDOW,      0, event_topic_creator },
    { "334", "",                        STATUS_WINDOW,  1, NULL },
    { "338", "",                        NO_WINDOW,      0, event_whois_host },
    { "341", "RPL_INVITING",            NO_WINDOW,      0, event_inviting },
    { "346", "RPL_INVITELIST",          NO_WINDOW,      0, event_inviteList },
    { "347", "RPL_ENDOFINVITELIST",     NO_WINDOW,      0, event_eof_inviteList },
    { "348", "RPL_EXCEPTLIST",          NO_WINDOW,      0, event_exceptList },
    { "349", "RPL_ENDOFEXCEPTLIST",     NO_WINDOW,      0, event_eof_exceptList },
    { "352", "RPL_WHOREPLY",            NO_WINDOW,      0, event_whoReply },
    { "353", "RPL_NAMREPLY",            NO_WINDOW,      0, event_names },
    { "366", "RPL_ENDOFNAMES",          NO_WINDOW,      0, event_eof_names },
    { "367", "RPL_BANLIST",             NO_WINDOW,      0, event_banlist },
    { "368", "RPL_ENDOFBANLIST",        NO_WINDOW,      0, event_eof_banlist },
    { "369", "RPL_ENDOFWHOWAS",         ACTIVE_WINDOW,  2, NULL },
    { "372", "RPL_MOTD",                NO_WINDOW,      0, event_motd },
    { "375", "RPL_MOTDSTART",           NO_WINDOW,      0, event_motd },
    { "376", "RPL_ENDOFMOTD",           NO_WINDOW,      0, event_motd },
    { "378", "",                        NO_WINDOW,      0, event_whois_conn },
    { "379", "",                        NO_WINDOW,      0, event_whois_modes },
    { "381", "RPL_YOUREOPER",           NO_WINDOW,      0, event_youAreOper },
    { "396", "",                        NO_WINDOW,      0, event_allaround_extract_remove_colon },
    { "401", "ERR_NOSUCHNICK",          ACTIVE_WINDOW,  2, NULL },
    { "402", "ERR_NOSUCHSERVER",        ACTIVE_WINDOW,  2, NULL },
    { "403", "ERR_NOSUCHCHANNEL",       ACTIVE_WINDOW,  2, NULL },
    { "404", "ERR_CANNOTSENDTOCHAN",    ACTIVE_WINDOW,  2, NULL },
    { "405", "ERR_TOOMANYCHANNELS",     ACTIVE_WINDOW,  2, NULL },
    { "406", "ERR_WASNOSUCHNICK",       ACTIVE_WINDOW,  2, NULL },
    { "407", "ERR_TOOMANYTARGETS",      ACTIVE_WINDOW,  2, NULL },
    { "408", "ERR_NOSUCHSERVICE",       ACTIVE_WINDOW,  2, NULL },
    { "412", "ERR_NOTEXTTOSEND",        ACTIVE_WINDOW,  1, NULL },
    { "416", "ERR_",                    ACTIVE_WINDOW,  2, NULL },
    { "421", "ERR_UNKNOWNCOMMAND",      ACTIVE_WINDOW,  2, NULL },
    { "422", "ERR_NOMOTD",              STATUS_WINDOW,  1, NULL },
    { "432", "ERR_ERRONEUSNICKNAME",    ACTIVE_WINDOW,  2, NULL },
    { "433", "ERR_NICKNAMEINUSE",       NO_WINDOW,      0, event_nicknameInUse },
    { "435", "ERR_",                    ACTIVE_WINDOW,  3, NULL },
    { "437", "ERR_UNAVAILRESOURCE",     ACTIVE_WINDOW,  2, NULL },
    { "439", "",                        STATUS_WINDOW,  1, NULL },
    { "441", "ERR_USERNOTINCHANNEL",    ACTIVE_WINDOW,  3, NULL },
    { "442", "ERR_NOTONCHANNEL",        ACTIVE_WINDOW,  2, NULL },
    { "443", "ERR_USERONCHANNEL",       ACTIVE_WINDOW,  3, NULL },
    { "444", "ERR_NOLOGIN",             NO_WINDOW,      0, event_allaround_extract_find_colon },
    { "447", "",                        ACTIVE_WINDOW,  1, NULL },
    { "451", "ERR_NOTREGISTERED",       STATUS_WINDOW,  1, NULL },
    { "461", "ERR_NEEDMOREPARAMS",      ACTIVE_WINDOW,  2, NULL },
    { "462", "ERR_ALREADYREGISTRED",    ACTIVE_WINDOW,  1, NULL },
    { "465", "ERR_YOUREBANNEDCREEP",    STATUS_WINDOW,  1, NULL },
    { "466", "ERR_YOUWILLBEBANNED",     NO_WINDOW,      0, event_allaround_extract_find_colon },
    { "467", "ERR_KEYSET",              ACTIVE_WINDOW,  2, NULL },
    { "470", "",                        NO_WINDOW,      0, event_channel_forward },
    { "471", "ERR_CHANNELISFULL",       ACTIVE_WINDOW,  2, NULL },
    { "472", "ERR_UNKNOWNMODE",         ACTIVE_WINDOW,  2, NULL },
    { "473", "ERR_INVITEONLYCHAN",      ACTIVE_WINDOW,  2, NULL },
    { "474", "ERR_BANNEDFROMCHAN",      ACTIVE_WINDOW,  2, NULL },
    { "475", "ERR_BADCHANNELKEY",       ACTIVE_WINDOW,  2, NULL },
    { "477", "ERR_NOCHANMODES",         ACTIVE_WINDOW,  2, NULL },
    { "479", "ERR_",                    ACTIVE_WINDOW,  2, NULL },
    { "481", "ERR_NOPRIVILEGES",        ACTIVE_WINDOW,  1, NULL },
    { "482", "ERR_CHANOPRIVSNEEDED",    ACTIVE_WINDOW,  2, NULL },
    { "484", "ERR_RESTRICTED",          NO_WINDOW,      0, event_allaround_extract_find_colon },
    { "486", "ERR_",                    ACTIVE_WINDOW,  2, NULL },
    { "487", "ERR_",                    ACTIVE_WINDOW,  1, NULL },
    { "491", "ERR_NOOPERHOST",          ACTIVE_WINDOW,  1, NULL },
    { "492", "ERR_NOSERVICEHOST",       ACTIVE_WINDOW,  2, NULL },
    { "500", "ERR_",                    ACTIVE_WINDOW,  1, NULL },
    { "501", "ERR_UMODEUNKNOWNFLAG",    ACTIVE_WINDOW,  1, NULL },
    { "502", "ERR_USERSDONTMATCH",      ACTIVE_WINDOW,  1, NULL },
    { "520", "",                        ACTIVE_WINDOW,  2, NULL },
    { "615", "",                        NO_WINDOW,      0, event_whois_modes },
    { "616", "",                        NO_WINDOW,      0, event_whois_host },
    { "671", "",                        NO_WINDOW,      0, event_whois_ssl },
    { "696", "ERR_",                    ACTIVE_WINDOW,  4, NULL },
    { "698", "ERR_",                    ACTIVE_WINDOW,  4, NULL },
    { "716", "ERR_",                    ACTIVE_WINDOW,  2, NULL },
    { "717", "ERR_",                    ACTIVE_WINDOW,  2, NULL },
    { "742", "ERR_",                    ACTIVE_WINDOW,  4, NULL },
    { "900", "RPL_LOGGEDIN",            STATUS_WINDOW,  3, NULL },
    { "901", "RPL_LOGGEDOUT",           STATUS_WINDOW,  2, NULL },
    { "902", "ERR_NICKLOCKED",          NO_WINDOW,      0, handle_sasl_auth_fail },
    { "903", "RPL_SASLSUCCESS",         NO_WINDOW,      0, sasl_auth_success },
    { "904", "ERR_SASLFAIL",            NO_WINDOW,      0, handle_sasl_auth_fail },
    { "905", "ERR_SASLTOOLONG",         NO_WINDOW,      0, handle_sasl_auth_fail },
    { "906", "ERR_SASLABORTED",         STATUS_WINDOW,  1, NULL },
    { "907", "ERR_SASLALREADY",         NO_WINDOW,      0, handle_sasl_auth_fail },
    { "908", "RPL_SASLMECHS",           NO_WINDOW,      0, handle_sasl_auth_fail },
};

/****************************************************************
*                                                               *
*  ---------------------    Functions    ---------------------  *
*                                                               *
****************************************************************/

/**
 * Initialize irc module
 */
void
irc_init(void)
{
    if (g_cmdline_opts->nickname)
	irc_set_my_nickname(g_cmdline_opts->nickname);
    else if (Config_mod("nickname"))
	irc_set_my_nickname(Config_mod("nickname"));
    else
	err_quit("fatal: in irc_init: no nickname");
    event_names_init();
}

/**
 * Deinitialize irc module
 */
void
irc_deinit(void)
{
    g_alt_nick_tested = false;
    g_am_irc_op       = false;
    g_is_away         = false;

    free_and_null(&g_my_nickname);
    free_and_null(&g_server_hostname);
    BZERO(g_user_modes, sizeof g_user_modes);

    event_names_deinit();

    statusbar_update_display_beta();
    readline_top_panel();
}

/* -------------------------------------------------- */

bool
has_server_time(const struct irc_message_compo *compo)
{
    if (compo == NULL)
	return false;

    return (
	compo->year      != -1 &&
	compo->month     != -1 &&
	compo->day       != -1 &&
	compo->hour      != -1 &&
	compo->minute    != -1 &&
	compo->second    != -1 &&
	compo->precision != -1);
}

/**
 * Extract a message with help of given parameters
 */
void
irc_extract_msg(struct irc_message_compo *compo, PIRC_WINDOW to_window,
		int ext_bits, bool is_error)
{
    int i = 0;
    char *cp = NULL, *savp = "";

    if (strFeed(compo->params, ext_bits) != ext_bits) {
	PRINTTEXT_CONTEXT ctx;

	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_FAILURE, true);
	printtext(&ctx, "In irc_extract_msg: strFeed(..., %d) != %d", ext_bits,
	    ext_bits);
	return;
    }

    for (i = 0, cp = &compo->params[0];; i++, cp = NULL) {
	char *token = NULL;

	if ((token = strtok_r(cp, "\n", &savp)) == NULL) {
	    break;
	} else if (i == ext_bits) {
	    if (*token == ':') {
		token++;
	    }

	    if (*token) {
		PRINTTEXT_CONTEXT ctx;

		printtext_context_init(&ctx, to_window,
		    is_error ? TYPE_SPEC1_FAILURE : TYPE_SPEC1, true);
		printtext(&ctx, "%s", token);
		return;
	    }
	} else {
	    /* null */;
	}
    }
}

/**
 * Sort message components - into prefix, command and params.
 */
/*lint -sem(SortMsgCompo, r_null) */
static struct irc_message_compo *
SortMsgCompo(const char *protocol_message)
{
    char *cp = NULL, *savp = "";
    char *remaining_data = NULL;
    int message_has_prefix = 0, requested_feeds = -1;
    short int loop_run = 0;
    size_t bytes = 0;
    struct irc_message_compo *compo = xcalloc(sizeof *compo, 1);

    compo->year = compo->month = compo->day = -1;
    compo->hour = compo->minute = compo->second = compo->precision = -1;

    compo->prefix  = NULL;
    compo->command = NULL;
    compo->params  = NULL;

    if (*protocol_message == '@') {
	char *substring = NULL;

	bytes = strcspn(protocol_message, " ");
	substring = xcalloc(bytes, 1);
	snprintf(substring, bytes, "%s", protocol_message);

/*
 * sscanf() is safe in this context
 */
#if WIN32
#pragma warning(disable: 4996)
#endif

	if (!strncmp(substring, "@time=", 6)) {
	    if (sscanf(substring, "@time=%d-%d-%dT%d:%d:%d.%dZ",
		& (compo->year),
		& (compo->month),
		& (compo->day),
		& (compo->hour),
		& (compo->minute),
		& (compo->second),
		& (compo->precision)) != 7) {
		free(compo);
		print_and_free("In SortMsgCompo: IRCv3: server time error",
			       substring);
		return NULL;
	    }
	} else {
	    free(compo);
	    print_and_free("In SortMsgCompo: IRCv3: unsupported extension",
			   substring);
	    return NULL;
	}

	free(substring);

/*
 * Reset warning behavior to its default value
 */
#if WIN32
#pragma warning(default: 4996)
#endif
    } /* ===== EOF IRCv3 extensions ===== */

    const char *ccp = &protocol_message[bytes];
    while (*ccp == ' ')
	ccp++;
    message_has_prefix = (*ccp == ':');
    requested_feeds = message_has_prefix ? 2 : 1;
    remaining_data = sw_strdup(ccp);

    if (strFeed(remaining_data, requested_feeds) != requested_feeds &&
	strstr(remaining_data, "\nAWAY") == NULL) {
	free(compo);
	print_and_free("In SortMsgCompo: strFeed: "
	    "requested feeds mismatch feeds written",
	    remaining_data);
	return NULL;
    }

    for (loop_run = 0, cp = &remaining_data[0];; loop_run++, cp = NULL) {
	char *token = NULL;

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

    free(remaining_data);
    sw_assert(compo->command != NULL);
    sw_assert(compo->params != NULL || strings_match(compo->command, "AWAY"));
    return compo;
}

/**
 * Free message components
 */
static void
FreeMsgCompo(struct irc_message_compo *compo)
{
    free_not_null(compo->prefix);
    free_not_null(compo->command);
    free_not_null(compo->params);

    free(compo);
}

/**
 * Search and route event
 */
static void
irc_search_and_route_event(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);

    if (is_alphabetic(compo->command)) {
	struct normal_events_tag *sp = NULL;
	const size_t size = ARRAY_SIZE(normal_events);

	for (sp = &normal_events[0]; sp < &normal_events[size]; sp++) {
	    if (strings_match(sp->normal_event, compo->command)) {
		sp->event_handler(compo);
		return;
	    }
	}

	printtext(&ctx, "Unknown normal event: %s", compo->command);
#if UNKNOWN_EVENT_DISPLAY_EXTENDED_INFO
	printtext(&ctx, "params = %s", compo->params);
	printtext(&ctx, "prefix = %s", compo->prefix ? compo->prefix : "none");
#endif
    } else if (is_numeric(compo->command) && strlen(compo->command) == 3) {
	struct numeric_events_tag *sp = NULL;
	const size_t size = ARRAY_SIZE(numeric_events);

	for (sp = &numeric_events[0]; sp < &numeric_events[size]; sp++) {
	    if (strings_match(sp->numeric_event, compo->command)) {
		if (sp->event_handler != NULL) {
		    sp->event_handler(compo);
		} else {
		    if (sp->window == STATUS_WINDOW)
			irc_extract_msg(compo, g_status_window, sp->ext_bits,
			    !strncmp(sp->official_name, "ERR_", 4));
		    else if (sp->window == ACTIVE_WINDOW)
			irc_extract_msg(compo, g_active_window, sp->ext_bits,
			    !strncmp(sp->official_name, "ERR_", 4));
		    else
			sw_assert_not_reached();
		}

		return;
	    }
	}

	printtext(&ctx, "Unknown numeric event: %s", compo->command);
#if UNKNOWN_EVENT_DISPLAY_EXTENDED_INFO
	printtext(&ctx, "params = %s", compo->params);
	printtext(&ctx, "prefix = %s", compo->prefix ? compo->prefix : "none");
#endif
    } else {
	printtext(&ctx, "Erroneous event: %s", compo->command);
    }
}

/**
 * Process protocol message
 */
static void
ProcessProtoMsg(const char *token)
{
    struct irc_message_compo *compo = NULL;

    if ((compo = SortMsgCompo(token)) == NULL)
	return;
    irc_search_and_route_event(compo);
    FreeMsgCompo(compo);
}

/**
 * If a received chunk of data aren't terminated with carriage return
 * plus line feed - this function gets the last message - so it can be
 * concatenated in a later stage.
 */
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

/**
 * Handle and interpret irc events
 */
void
irc_handle_interpret_events(char *recvbuffer,
			    char **message_concat,
			    enum message_concat_state *state)
{
    bool terminated_recvchunk = false;
    char *cp = NULL, *savp = "";
    char *last_token = NULL;
    const char separators[] = "\r\n";
    long int loop_count = 0;

    if (recvbuffer == NULL || (*state != CONCAT_BUFFER_IS_EMPTY &&
			       *state != CONCAT_BUFFER_CONTAIN_DATA)) {
	err_exit(EINVAL, "irc_handle_interpret_events");
    } else if (strings_match(recvbuffer, "") ||
	       strpbrk(recvbuffer, separators) == NULL) {
	return;
    } else {
	/*empty*/;
    }

    if (*state == CONCAT_BUFFER_CONTAIN_DATA &&
	recvbuffer[0] == '\r' && recvbuffer[1] == '\n') {
	ProcessProtoMsg(*message_concat);
	free_and_null(&(*message_concat));
	*state = CONCAT_BUFFER_IS_EMPTY;
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
	char *token = NULL;

	if ((token = strtok_r(cp, separators, &savp)) == NULL) {
	    break; /* No more tokens  --  end loop... */
	} else if (last_token != NULL && *state == CONCAT_BUFFER_IS_EMPTY &&
		   strings_match(token, last_token)) {
	    free_and_null(&(*message_concat));
	    *message_concat = sw_strdup(last_token);
	    free_and_null(&last_token);
	    *state = CONCAT_BUFFER_CONTAIN_DATA;
	    /*
	     * On the next call to this function the (incomplete) irc
	     * message will be concatenated
	     */
	    return;
	} else if (loop_count == 0 && *state == CONCAT_BUFFER_CONTAIN_DATA) {
	    realloc_strcat(&(*message_concat), token);
	    token = *message_concat;
	    *state = CONCAT_BUFFER_IS_EMPTY;
	    /*
	     * The special token can now be passed to the handler
	     */
	} else {
	    /*no action*/;
	}

	ProcessProtoMsg(token);
    }

    free_not_null(last_token);
}

/**
 * Set user nickname
 */
void
irc_set_my_nickname(const char *nick)
{
    if (nick == NULL || strings_match(nick, ""))
	err_exit(EINVAL, "irc_set_my_nickname");
    free(g_my_nickname);
    g_my_nickname = sw_strdup(nick);
    statusbar_update_display_beta();
    readline_top_panel();
}

/**
 * Set server hostname
 */
void
irc_set_server_hostname(const char *srv_host)
{
    if (srv_host == NULL || strings_match(srv_host, ""))
	err_exit(EINVAL, "irc_set_server_hostname");
    free(g_server_hostname);
    g_server_hostname = sw_strdup(srv_host);
}
