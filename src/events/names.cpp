/* Handle event names (353) and event EOF names (366)
   Copyright (C) 2015-2022 Markus Uhlin. All rights reserved.

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

#include "../assertAPI.h"
#include "../dataClassify.h"
#include "../errHand.h"
#include "../irc.h"
#include "../libUtils.h"
#include "../main.h"
#include "../network.h"
#include "../nicklist.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "names.h"

#define DJB2_HASHING_TECHNIQUE 1

/****************************************************************
*                                                               *
*  ------------------ Structure definitions ------------------  *
*                                                               *
****************************************************************/

struct hInstall_context {
    char	*channel;
    char	*nick;
    bool	 is_owner;
    bool	 is_superop;
    bool	 is_op;
    bool	 is_halfop;
    bool	 is_voice;

    hInstall_context();
    hInstall_context(char *, char *, const char);
};

hInstall_context::hInstall_context()
{
    this->channel = NULL;
    this->nick = NULL;
    this->is_owner   = false;
    this->is_superop = false;
    this->is_op      = false;
    this->is_halfop  = false;
    this->is_voice   = false;
}

hInstall_context::hInstall_context(char *channel, char *nick, const char c)
{
    this->channel = channel;
    this->nick = nick;
    this->is_owner   = (c == '~');
    this->is_superop = (c == '&');
    this->is_op      = (c == '@');
    this->is_halfop  = (c == '%');
    this->is_voice   = (c == '+');
}

/****************************************************************
*                                                               *
*  -------------- Objects with internal linkage --------------  *
*                                                               *
****************************************************************/

static char names_channel[1000] = "";

/****************************************************************
*                                                               *
*  ---------------------    Functions    ---------------------  *
*                                                               *
****************************************************************/

static void
add_match(PTEXTBUF matches, const char *user)
{
    if (textBuf_size(matches) == 0) {
	if ((errno = textBuf_ins_next(matches, NULL, user, -1)) != 0)
	    err_sys("get_list_of_matching_channel_users: textBuf_ins_next");
    } else {
	if ((errno = textBuf_ins_next(matches, textBuf_tail(matches), user, -1)) != 0)
	    err_sys("get_list_of_matching_channel_users: textBuf_ins_next");
    }
}

static bool
got_hits(const IRC_WINDOW *window, const char *search_var)
{
    for (size_t n = 0; n < ARRAY_SIZE(window->names_hash); n++) {
	for (PNAMES names = window->names_hash[n]; names != NULL;
	     names = names->next) {
	    if (!strncmp(search_var, names->nick, strlen(search_var)))
		return true;
	}
    }

    return false;
}

#if DJB2_HASHING_TECHNIQUE
/*
 * DJB2
 */

static inline unsigned int
hash_djb2(const char *nick)
{
#define MAGIC_NUMBER 5381
    char          c         = '\0';
    char         *nick_copy = strToLower(sw_strdup(nick));
    char         *nick_p    = &nick_copy[0];
    unsigned int  hashval   = MAGIC_NUMBER;

    while ((c = *nick_p++) != '\0')
	hashval = ((hashval << 5) + hashval) + c;
    free(nick_copy);
    return (hashval % NAMES_HASH_TABLE_SIZE);
}
#else
/*
 * P.J. Weinberger hashing
 */

static inline unsigned int
hash_pjw(const char *nick)
{
    char		 c;
    char		*nick_copy = strToLower(sw_strdup(nick));
    char		*nick_p	   = nick_copy;
    unsigned int	 hashval   = 0;
    unsigned int	 tmp;

    while ((c = *nick_p++) != '\0') {
	hashval = (hashval << 4) + c;
	tmp = hashval & 0xf0000000;

	if (tmp) {
	    hashval ^= (tmp >> 24);
	    hashval ^= tmp;
	}
    }

    free(nick_copy);
    return (hashval % NAMES_HASH_TABLE_SIZE);
}
#endif

static unsigned int
hash(const char *nick)
{
#if DJB2_HASHING_TECHNIQUE
//#pragma message("Using DJB2 hashing technique")
    return hash_djb2(nick);
#else
    return hash_pjw(nick);
#endif
}

static inline bool
already_is_in_names_hash(const char *nick, PIRC_WINDOW window)
{
    for (PNAMES names = window->names_hash[hash(nick)];
	 names != NULL;
	 names = names->next) {
	if (strings_match_ignore_case(nick, names->nick))
	    return true;
    }

    return false;
}

static int
hInstall(const struct hInstall_context *ctx)
{
    PIRC_WINDOW window_entry = NULL;
    PNAMES names_entry = NULL;

    if (isNull(ctx) || isNull(ctx->channel))
	return ERR;
    else if ((window_entry = window_by_label(ctx->channel)) == NULL) {
	debug("events/names.cpp: hInstall: cannot find window labelled \"%s\"",
	    ctx->channel);
	return ERR;
    } else if (isNull(ctx->nick) || isEmpty(ctx->nick)) {
	debug("events/names.cpp: hInstall: no nickname (channel=%s)",
	    ctx->channel);
	return ERR;
    } else if (already_is_in_names_hash(ctx->nick, window_entry)) {
	debug("events/names.cpp: hInstall: busy nickname: "
	    "\"%s\" (channel=%s)", ctx->nick, ctx->channel);
	return ERR;
    }

    names_entry = static_cast<PNAMES>(xcalloc(sizeof *names_entry, 1));
    names_entry->nick       = sw_strdup(ctx->nick);
    names_entry->is_owner   = ctx->is_owner;
    names_entry->is_superop = ctx->is_superop;
    names_entry->is_op      = ctx->is_op;
    names_entry->is_halfop  = ctx->is_halfop;
    names_entry->is_voice   = ctx->is_voice;

    const unsigned int hashval = hash(ctx->nick);
    names_entry->next = window_entry->names_hash[hashval];
    window_entry->names_hash[hashval] = names_entry;

    if (ctx->is_owner)
	window_entry->num_owners++;
    else if (ctx->is_superop)
	window_entry->num_superops++;
    else if (ctx->is_op)
	window_entry->num_ops++;
    else if (ctx->is_halfop)
	window_entry->num_halfops++;
    else if (ctx->is_voice)
	window_entry->num_voices++;
    else
	window_entry->num_normal++;
    window_entry->num_total++;
    return OK;
}

static void hUndef(PIRC_WINDOW, PNAMES) PTR_ARGS_NONNULL;

static void
hUndef(PIRC_WINDOW window, PNAMES entry)
{
    if (isNull(window) || isNull(entry) || isNull(entry->nick) ||
	isEmpty(entry->nick))
	return;

    PNAMES *indirect = & (window->names_hash[hash(entry->nick)]);

    while (sw_assert(indirect != NULL), *indirect != entry)
	indirect = & ((*indirect)->next);

    *indirect = entry->next;

    free_and_null(&entry->nick);

    if (entry->is_owner)
	window->num_owners--;
    else if (entry->is_superop)
	window->num_superops--;
    else if (entry->is_op)
	window->num_ops--;
    else if (entry->is_halfop)
	window->num_halfops--;
    else if (entry->is_voice)
	window->num_voices--;
    else
	window->num_normal--;

    window->num_total--;

    free(entry);
}

static void
output_statistics(PRINTTEXT_CONTEXT ctx, const char *channel,
    const IRC_WINDOW *window)
{
    ctx.spec_type = TYPE_SPEC1;
    printtext(&ctx, "%s%s%s%c%s: Total of %c%d%c nicks "
	"%s%c%d%c ops, %c%d%c halfops, %c%d%c voices, %c%d%c normal%s",
	LEFT_BRKT, COLOR1, channel, NORMAL, RIGHT_BRKT,
	BOLD, window->num_total, BOLD,
	LEFT_BRKT,
	BOLD, window->num_ops, BOLD, BOLD, window->num_halfops, BOLD,
	BOLD, window->num_voices, BOLD, BOLD, window->num_normal, BOLD,
	RIGHT_BRKT);
    if (window->num_owners)
	printtext(&ctx, "-- Additionally: %c%d%c channel owner%s",
	    BOLD, window->num_owners, BOLD,
	    (window->num_owners > 1 ? "s" : ""));
    if (window->num_superops)
	printtext(&ctx, "-- Additionally: %c%d%c superop%s",
	    BOLD, window->num_superops, BOLD,
	    (window->num_superops > 1 ? "s" : ""));
}

static void
reset_counters(PIRC_WINDOW window)
{
    window->num_owners   = 0;
    window->num_superops = 0;
    window->num_ops      = 0;
    window->num_halfops  = 0;
    window->num_voices   = 0;
    window->num_normal   = 0;
    window->num_total    = 0;
}

PTEXTBUF
get_list_of_matching_channel_users(const char *chan, const char *search_var)
{
    PIRC_WINDOW window = NULL;

    if ((window = window_by_label(chan)) == NULL ||
	!got_hits(window, search_var))
	return NULL;

    PTEXTBUF matches = textBuf_new();

    for (size_t n = 0; n < ARRAY_SIZE(window->names_hash); n++) {
	for (PNAMES names = window->names_hash[n]; names != NULL;
	     names = names->next) {
	    if (!strncmp(search_var, names->nick, strlen(search_var)))
		add_match(matches, names->nick);
	}
    }

    return matches;
}

/**
 * Initialize the module
 */
void
event_names_init(void)
{
    BZERO(names_channel, sizeof names_channel);
}

/**
 * Deinitialize the module
 */
void
event_names_deinit(void)
{
    window_foreach_destroy_names();
}

PNAMES
event_names_htbl_lookup(const char *nick, const char *channel)
{
    PIRC_WINDOW window;
    PNAMES	names;

    if (isNull(nick) || isEmpty(nick) ||
	(window = window_by_label(channel)) == NULL) {
	return NULL;
    }

    for (names = window->names_hash[hash(nick)];
	 names != NULL;
	 names = names->next) {
	if (strings_match_ignore_case(nick, names->nick)) {
	    return names;
	}
    }

    return NULL;
}

int
event_names_htbl_insert(const char *nick, const char *channel)
{
    if (isNull(nick) || isEmpty(nick)) {
	return ERR;
    }

    struct hInstall_context ctx; // calls constructor

    ctx.channel = const_cast<char *>(channel);
    ctx.nick = const_cast<char *>(nick);

    if (hInstall(&ctx) == ERR)
	return ERR;
    if (nicklist_update(window_by_label(channel)) != 0)
	debug("event_names_htbl_insert: nicklist_update: error");
    return OK;
}

int
event_names_htbl_remove(const char *nick, const char *channel)
{
    PIRC_WINDOW window;
    PNAMES	names;

    if (isNull(nick) || isEmpty(nick) ||
	(window = window_by_label(channel)) == NULL) {
	return ERR;
    }

    for (names = window->names_hash[hash(nick)];
	 names != NULL;
	 names = names->next) {
	if (strings_match_ignore_case(nick, names->nick)) {
	    hUndef(window, names);
	    if (nicklist_update(window) != 0)
		debug("event_names_htbl_remove: nicklist_update: error");
	    return OK;
	}
    }

    return ERR;
}

/* event_eof_names: 366

   Example:
     :irc.server.com 366 <recipient> <channel> :End of /NAMES list. */
void
event_eof_names(struct irc_message_compo *compo)
{
    PRINTTEXT_CONTEXT ptext_ctx;

    try {
	PIRC_WINDOW win = NULL;
	char *state = const_cast<char *>("");

	if (strFeed(compo->params, 2) != 2)
	    throw std::runtime_error("strFeed");

	(void) strtok_r(compo->params, "\n", &state);
	char *channel = strtok_r(NULL, "\n", &state);
	char *eof_msg = strtok_r(NULL, "\n", &state);

	if (isNull(channel))
	    throw std::runtime_error("null channel");
	else if (!is_irc_channel(channel) ||
		 strpbrk(channel + 1, g_forbidden_chan_name_chars) != NULL)
	    throw std::runtime_error("invalid channel");
	else if (isNull(eof_msg))
	    throw std::runtime_error("null message");
	else if (!strings_match_ignore_case(channel, names_channel)) {
	    throw std::runtime_error("unable to parse names of two (or more) "
		"channels simultaneously");
	} else {
	    BZERO(names_channel, sizeof names_channel);
	}

	if ((win = window_by_label(channel)) == NULL)
	    throw std::runtime_error("window lookup error");
	else if (win->received_names) {
	    err_log(0, "warning: server sent event 366 (RPL_ENDOFNAMES): "
		"already received names for channel %s", channel);
	    return;
	} else {
	    win->received_names = true;
	}

	if (nicklist_new(win) != 0)
	    debug("event_eof_names: cannot create nicklist");
	if (!g_icb_mode)
	    (void) net_send("MODE %s", channel);

	printtext_context_init(&ptext_ctx, win, TYPE_SPEC3, true);
	output_statistics(ptext_ctx, channel, win);

	return;
    } catch (const std::runtime_error &e) {
	printtext_context_init(&ptext_ctx, g_active_window, TYPE_SPEC1_FAILURE,
	    true);
	printtext(&ptext_ctx, "event_eof_names: fatal: %s", e.what());

	if (strstr(e.what(), "parse names of two (or more)") ||
	    strings_match(e.what(), "window lookup error")) {
	    printtext(&ptext_ctx, "must shutdown irc connection immediately...");
	    net_kill_connection();
	}
    }
}

/* event_names: 353

   Example:
     :irc.server.com 353 <recipient> <chan type> <channel> :<nick 1> <nick 2>
                                                            [...]

   <chan type> can be either:
     @ for secret channels
     * for private channels
     = for public channels */
void
event_names(struct irc_message_compo *compo)
{
    char *names_copy = NULL;

    try {
	PIRC_WINDOW win = NULL;
	char *state1 = const_cast<char *>("");
	char *state2 = const_cast<char *>("");

	if (strFeed(compo->params, 3) != 3)
	    throw std::runtime_error("strFeed");

	(void) strtok_r(compo->params, "\n", &state1); /* recipient */
	char *chan_type = strtok_r(NULL, "\n", &state1);
	char *channel   = strtok_r(NULL, "\n", &state1);
	char *names     = strtok_r(NULL, "\n", &state1);

	if (isNull(chan_type))
	    throw std::runtime_error("no channel type");
	else if (isNull(channel))
	    throw std::runtime_error("no channel");
	else if (!is_irc_channel(channel) ||
		 strpbrk(channel + 1, g_forbidden_chan_name_chars) != NULL)
	    throw std::runtime_error("invalid channel");
	else if (isNull(names))
	    throw std::runtime_error("no names");
	else if (strings_match(names_channel, "") &&
	    sw_strcpy(names_channel, channel, ARRAY_SIZE(names_channel)) != 0)
	    throw std::runtime_error("unable to store names channel");
	else if (!strings_match_ignore_case(names_channel, channel)) {
	    throw std::runtime_error("unable to parse names of two (or more) "
		"channels simultaneously");
	} else if ((win = window_by_label(channel)) == NULL) {
	    throw std::runtime_error("window lookup error");
	} else if (win->received_names) {
	    err_log(0, "warning: server sent event 353 (RPL_NAMREPLY): "
		"already received names for channel %s", channel);
	    return;
	} else {
	    names_copy = sw_strdup(*names == ':' ? &names[1] : &names[0]);
	}

	char *nick = NULL;
	char *token = NULL;

	for (char *cp = &names_copy[0];; cp = NULL) {
	    if ((token = strtok_r(cp, " ", &state2)) == NULL)
		break;

	    nick = ((*token == '~' || *token == '&' || *token == '@' ||
		     *token == '%' || *token == '+') ? &token[1] : &token[0]);

	    struct hInstall_context ctx(channel, nick, *token);

	    if (hInstall(&ctx) != OK)
		continue;
	}
    } catch (const std::runtime_error &e) {
	PRINTTEXT_CONTEXT ptext_ctx;

	printtext_context_init(&ptext_ctx, g_active_window, TYPE_SPEC1_FAILURE,
	    true);
	printtext(&ptext_ctx, "event_names: fatal: %s", e.what());

	if (strstr(e.what(), "store names channel") ||
	    strstr(e.what(), "parse names of two (or more)")) {
	    printtext(&ptext_ctx, "must shutdown irc connection immediately...");
	    net_kill_connection();
	}
    }

    free(names_copy);
}

void
event_names_htbl_remove_all(PIRC_WINDOW window)
{
	PNAMES *entry_p;
	PNAMES p, tmp;

	if (window == NULL || !is_irc_channel(window->label))
		return;
	for (entry_p = &window->names_hash[0];
	    entry_p < &window->names_hash[NAMES_HASH_TABLE_SIZE];
	    entry_p++) {
		for (p = *entry_p; p != NULL; p = tmp) {
			tmp = p->next;
			hUndef(window, p);
		}
	}

	window->received_names = false;
	reset_counters(window);
}
/* EOF */
