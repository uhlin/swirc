/* Handle event names (353) and event EOF names (366)
   Copyright (C) 2015-2020 Markus Uhlin. All rights reserved.

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

#include "../assertAPI.h"
#include "../dataClassify.h"
#include "../errHand.h"
#include "../irc.h"
#include "../libUtils.h"
#include "../main.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../strdup_printf.h"
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
};

typedef struct tagCHUNK {
    char *nick;
    struct tagCHUNK *next;
} CHUNK, *PCHUNK;

struct name_tag {
    char *s;
};

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

static void
destroy_names_array(const int ntp1, struct name_tag *names_array)
{
    for (int i = 0; i < ntp1; i++) {
	free_not_null(names_array[i].s);
	names_array[i].s = NULL;
    }

    free(names_array);
}

static void
free_names_chunk(PCHUNK head)
{
    PCHUNK p, tmp;

    for (p = head; p; p = tmp) {
	tmp = p->next;
	free(p->nick);
	free(p);
    }
}

struct column_lengths {
    int col1;
    int col2;
    int col3;
};

static struct column_lengths
get_column_lengths(const int ntp1, struct name_tag *names_array)
{
    struct column_lengths cl = {
	.col1 = 0,
	.col2 = 0,
	.col3 = 0,
    };

    for (int i = 0; i < ntp1; i++) {
	const char *nick1 = names_array[i].s;
	char *nick2, *nick3;

	if ((i + 1) < ntp1 && (i + 2) < ntp1) {
	    nick2 = names_array[++i].s;
	    nick3 = names_array[++i].s;
	} else if ((i + 1) < ntp1) {
	    nick2 = names_array[++i].s;
	    nick3 = NULL;
	} else {
	    nick2 = nick3 = NULL;
	}

	if (nick1 && strlen(nick1) > cl.col1)
	    cl.col1 = (int) strlen(nick1);
	if (nick2 && strlen(nick2) > cl.col2)
	    cl.col2 = (int) strlen(nick2);
	if (nick3 && strlen(nick3) > cl.col3)
	    cl.col3 = (int) strlen(nick3);
    }

    return cl;
}

/*lint -sem(next_names, r_null) */
static PCHUNK
next_names(PIRC_WINDOW window, int *idx)
{
    PCHUNK  head        = NULL;
    PCHUNK  new_element = NULL;
    PCHUNK  temp        = NULL;
    PNAMES *entry_p     = & (window->names_hash[*idx]);

    for (PNAMES p = *entry_p; p != NULL; p = p->next) {
	char c;

	if (p->is_owner) {
	    c = '~';
	} else if (p->is_superop) {
	    c = '&';
	} else if (p->is_op) {
	    c = '@';
	} else if (p->is_halfop) {
	    c = '%';
	} else if (p->is_voice) {
	    c = '+';
	} else {
	    c = ' ';
	}

	if (!head) {
	    head = xmalloc(sizeof (CHUNK));
	    head->nick = strdup_printf("%c%s", c, p->nick);
	    head->next = NULL;
	    continue;
	}

	new_element = xmalloc(sizeof (CHUNK));
	new_element->nick = strdup_printf("%c%s", c, p->nick);
	new_element->next = NULL;

	temp = head;
	while (temp->next)
	    temp = temp->next;
	temp->next = new_element;
    }

    return (head);
}

static struct name_tag *
get_names_array(const int ntp1, PIRC_WINDOW window)
{
    int i = 0, j = 0;
    struct name_tag *names_array = xcalloc(ntp1, sizeof (struct name_tag));

    for (i = j = 0; i < NAMES_HASH_TABLE_SIZE; i++) {
	PCHUNK head, element;

	if ((head = next_names(window, &i)) == NULL)
	    continue;

	for (element = head; element; element = element->next)
	    names_array[j++].s = sw_strdup(element->nick);

	free_names_chunk(head);
    }

    return names_array;
}

static bool
got_hits(PIRC_WINDOW window, const char *search_var)
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

static int
hInstall(const struct hInstall_context *ctx)
{
    PIRC_WINDOW window_entry = NULL;
    PNAMES names_entry = NULL;

    if ((window_entry = window_by_label(ctx->channel)) == NULL) {
	err_log(0, "In events/names.c: "
	    "Can't find a window with label %s during hInstall()",
	    ctx->channel);
	return ERR;
    }

    names_entry             = xcalloc(sizeof *names_entry, 1);
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

static PTR_ARGS_NONNULL void
hUndef(PIRC_WINDOW window, PNAMES entry)
{
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

    free_not_null(entry);
    entry = NULL;
}

static int
names_cmp_fn(const void *obj1, const void *obj2)
{
    const struct name_tag	*p1    = obj1;
    const struct name_tag	*p2    = obj2;
    const char			*nick1 = p1 && p1->s ? p1->s : NULL;
    const char			*nick2 = p2 && p2->s ? p2->s : NULL;

    if (nick1 == NULL) {
	return (1);
    } else if (nick2 == NULL) {
	return (-1);
    } else {
	if (*nick1 == ' ') {
	    nick1++;
	}

	if (*nick2 == ' ') {
	    nick2++;
	}

	switch (*nick1) {
	case '~':
	    if (*nick2 == '&' || *nick2 == '@' || *nick2 == '%' ||
		*nick2 == '+' || *nick2 != '~')
		return (-1);
	    break;
	case '&':
	    if (*nick2 == '@' || *nick2 == '%' || *nick2 == '+')
		return (-1);
	    break;
	case '@':
	    if (*nick2 == '%' || *nick2 == '+')
		return (-1);
	    break;
	case '%':
	    if (*nick2 == '+')
		return (-1);
	    break;
	case '+':
	    break;
	}

	switch (*nick2) {
	case '~':
	    if (*nick1 == '&' || *nick1 == '@' || *nick1 == '%' ||
		*nick1 == '+' || *nick1 != '~')
		return (1);
	    break;
	case '&':
	    if (*nick1 == '@' || *nick1 == '%' || *nick1 == '+')
		return (1);
	    break;
	case '@':
	    if (*nick1 == '%' || *nick1 == '+')
		return (1);
	    break;
	case '%':
	    if (*nick1 == '+')
		return (1);
	    break;
	case '+':
	    break;
	}
    }

    return (strcasecmp(nick1, nick2));
}

static void
output_statistics(PRINTTEXT_CONTEXT ctx, const char *channel,
		  PIRC_WINDOW window)
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
	    window->num_owners > 1 ? "s" : "");
    if (window->num_superops)
	printtext(&ctx, "-- Additionally: %c%d%c superop%s",
	    BOLD, window->num_superops, BOLD,
	    window->num_superops > 1 ? "s" : "");
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

static bool
set_format1(char *dest, size_t destsize, struct column_lengths cl)
{
    int ret = snprintf(dest, destsize, "%%s%%-%ds%%s %%s%%-%ds%%s %%s%%-%ds%%s",
		       cl.col1, cl.col2, cl.col3);

    return ((ret == -1 || ret >= destsize) ? false : true);
}

static bool
set_format2(char *dest, size_t destsize, struct column_lengths cl)
{
    int ret = snprintf(dest, destsize, "%%s%%-%ds%%s %%s%%-%ds%%s",
		       cl.col1, cl.col2);

    return ((ret == -1 || ret >= destsize) ? false : true);
}

static bool
set_format3(char *dest, size_t destsize, struct column_lengths cl)
{
    int ret = snprintf(dest, destsize, "%%s%%-%ds%%s",
		       cl.col1);

    return ((ret == -1 || ret >= destsize) ? false : true);
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
    struct hInstall_context ctx;

    if (isNull(nick) || isEmpty(nick)) {
	return ERR;
    }

    ctx.channel    = (char *) channel;
    ctx.nick       = (char *) nick;
    ctx.is_owner   = false;
    ctx.is_superop = false;
    ctx.is_op      = false;
    ctx.is_halfop  = false;
    ctx.is_voice   = false;

    return hInstall(&ctx);
}

int
event_names_htbl_modify_halfop(const char *nick, const char *channel,
			       bool is_halfop)
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
	    if (names->is_halfop && is_halfop)
		return OK;
	    else
		names->is_halfop = is_halfop;

	    if (names->is_owner || names->is_superop || names->is_op)
		return OK;
	    else if (! (names->is_halfop)) {
		window->num_halfops--;

		if (names->is_voice)
		    window->num_voices++;
		else
		    window->num_normal++;
	    } else { /* not halfop */
		window->num_halfops++;

		if (names->is_voice)
		    window->num_voices--;
		else
		    window->num_normal--;
	    }

	    return OK;
	}
    }

    return ERR;
}

int
event_names_htbl_modify_op(const char *nick, const char *channel, bool is_op)
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
	    if (names->is_op && is_op)
		return OK;
	    else
		names->is_op = is_op;

	    if (names->is_owner || names->is_superop)
		return OK;
	    else if (! (names->is_op)) {
		window->num_ops--;

		if (names->is_halfop)     window->num_halfops++;
		else if (names->is_voice) window->num_voices++;
		else window->num_normal++;
	    } else { /* not op */
		window->num_ops++;

		if (names->is_halfop)     window->num_halfops--;
		else if (names->is_voice) window->num_voices--;
		else window->num_normal--;
	    }

	    return OK;
	}
    }

    return ERR;
}

int
event_names_htbl_modify_owner(const char *nick, const char *channel,
			      bool is_owner)
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
	    if (names->is_owner && is_owner)
		return OK;
	    else
		names->is_owner = is_owner;

	    if (! (names->is_owner)) {
		window->num_owners--;

		if (names->is_superop)     window->num_superops++;
		else if (names->is_op)     window->num_ops++;
		else if (names->is_halfop) window->num_halfops++;
		else if (names->is_voice)  window->num_voices++;
		else window->num_normal++;
	    } else { /* not owner */
		window->num_owners++;

		if (names->is_superop)     window->num_superops--;
		else if (names->is_op)     window->num_ops--;
		else if (names->is_halfop) window->num_halfops--;
		else if (names->is_voice)  window->num_voices--;
		else window->num_normal--;
	    }

	    return OK;
	}
    }

    return ERR;
}

int
event_names_htbl_modify_superop(const char *nick, const char *channel,
				bool is_superop)
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
	    if (names->is_superop && is_superop)
		return OK;
	    else
		names->is_superop = is_superop;

	    if (names->is_owner)
		return OK;
	    else if (! (names->is_superop)) {
		window->num_superops--;

		if (names->is_op)          window->num_ops++;
		else if (names->is_halfop) window->num_halfops++;
		else if (names->is_voice)  window->num_voices++;
		else window->num_normal++;
	    } else { /* not superop */
		window->num_superops++;

		if (names->is_op)          window->num_ops--;
		else if (names->is_halfop) window->num_halfops--;
		else if (names->is_voice)  window->num_voices--;
		else window->num_normal--;
	    }

	    return OK;
	}
    }

    return ERR;
}

int
event_names_htbl_modify_voice(const char *nick, const char *channel,
			      bool is_voice)
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
	    if (names->is_voice && is_voice)
		return OK;
	    else
		names->is_voice = is_voice;

	    if (names->is_owner || names->is_superop || names->is_op ||
		names->is_halfop)
		return OK;
	    else if (! (names->is_voice)) {
		window->num_voices--;
		window->num_normal++;
	    } else { /* not voice */
		window->num_voices++;
		window->num_normal--;
	    }

	    return OK;
	}
    }

    return ERR;
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
	    return OK;
	}
    }

    return ERR;
}

#define FORMAT_SIZE 120

int
event_names_print_all(const char *channel)
{
    PIRC_WINDOW window = NULL;
    PRINTTEXT_CONTEXT ptext_ctx;
    char fmt1[FORMAT_SIZE] = "";
    char fmt2[FORMAT_SIZE] = "";
    char fmt3[FORMAT_SIZE] = "";
    int i = 0;
    struct name_tag *names_array = NULL;

    if ((window = window_by_label(channel)) == NULL) {
	return ERR;
    }

    printtext_context_init(&ptext_ctx, window, TYPE_SPEC_NONE, true);
    printtext(&ptext_ctx, "%s%sUsers %s%c%s",
	LEFT_BRKT, COLOR1, channel, NORMAL, RIGHT_BRKT);

    const int ntp1 = window->num_total + 1;
    names_array = get_names_array(ntp1, window);
    qsort(&names_array[0], ntp1, sizeof (struct name_tag), names_cmp_fn);
    struct column_lengths cl = get_column_lengths(ntp1, names_array);

    if (!set_format1(fmt1, sizeof fmt1, cl) ||
	!set_format2(fmt2, sizeof fmt2, cl) ||
	!set_format3(fmt3, sizeof fmt3, cl)) {
	destroy_names_array(ntp1, names_array);
	return ERR;
    }

    for (i = 0, ptext_ctx.spec_type = TYPE_SPEC3; i < ntp1; i++) {
	const char *nick1 = names_array[i].s;
	char *nick2, *nick3;

	if ((i + 1) < ntp1 && (i + 2) < ntp1) {
	    nick2 = names_array[++i].s;
	    nick3 = names_array[++i].s;
	} else if ((i + 1) < ntp1) {
	    nick2 = names_array[++i].s;
	    nick3 = NULL;
	} else {
	    nick2 = nick3 = NULL;
	}

	if (nick1 && nick2 && nick3) {
	    printtext(&ptext_ctx, fmt1,
		      LEFT_BRKT, nick1, RIGHT_BRKT,
		      LEFT_BRKT, nick2, RIGHT_BRKT,
		      LEFT_BRKT, nick3, RIGHT_BRKT);
	} else if (nick1 && nick2) {
	    printtext(&ptext_ctx, fmt2,
		      LEFT_BRKT, nick1, RIGHT_BRKT,
		      LEFT_BRKT, nick2, RIGHT_BRKT);
	} else if (nick1) {
	    printtext(&ptext_ctx, fmt3, LEFT_BRKT, nick1, RIGHT_BRKT);
	}
    }

    destroy_names_array(ntp1, names_array);
    output_statistics(ptext_ctx, channel, window);
    return OK;
}

/* event_eof_names: 366

   Example:
     :irc.server.com 366 <recipient> <channel> :End of /NAMES list. */
void
event_eof_names(struct irc_message_compo *compo)
{
    PIRC_WINDOW win = NULL;
    PRINTTEXT_CONTEXT ptext_ctx;
    char *err_reason = "";
    char *state = "";

    if (strFeed(compo->params, 2) != 2) {
	err_reason = "strFeed() has failed";
	goto bad;
    }

    (void) strtok_r(compo->params, "\n", &state);
    char *channel = strtok_r(NULL, "\n", &state);
    char *eof_msg = strtok_r(NULL, "\n", &state);

    if (channel == NULL || eof_msg == NULL) {
	err_reason = "failed to tokenize event";
	goto bad;
    } else if (!strings_match_ignore_case(channel, names_channel)) {
	err_reason =
	    "unable to parse names of two (or more) channels simultaneously";
	goto bad;
    } else {
	BZERO(names_channel, sizeof names_channel);
    }

    if ((win = window_by_label(channel)) == NULL) {
	err_reason = "window lookup error";
	goto bad;
    } else if (win->received_names) {
	err_log(0, "warning: server sent event 366 (RPL_ENDOFNAMES): "
	    "already received names for channel %s", channel);
	return;
    } else {
	win->received_names = true;
    }

    if (event_names_print_all(channel) != OK) {
	err_reason = "print all names error";
	goto bad;
    }

    if (!g_icb_mode)
	net_send("MODE %s", channel);
    return;

  bad:
    printtext_context_init(&ptext_ctx, g_active_window, TYPE_SPEC1_FAILURE,
	true);
    printtext(&ptext_ctx, "event_eof_names: fatal: %s", err_reason);
    printtext(&ptext_ctx, "must shutdown irc connection immediately...");
    net_kill_connection();
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
    PIRC_WINDOW win = NULL;
    PRINTTEXT_CONTEXT ptext_ctx;
    char *err_reason = "";
    char *names_copy = NULL;
    char *state1 = "";
    char *state2 = "";

    if (strFeed(compo->params, 3) != 3) {
	err_reason = "strFeed() has failed";
	goto bad;
    }

    (void) strtok_r(compo->params, "\n", &state1); /* recipient */
    char *chan_type = strtok_r(NULL, "\n", &state1);
    char *channel   = strtok_r(NULL, "\n", &state1);
    char *names     = strtok_r(NULL, "\n", &state1);

    if (chan_type == NULL || channel == NULL || names == NULL) {
	err_reason = "failed to tokenize event";
	goto bad;
    }

    if (isEmpty(names_channel) && sw_strcpy(names_channel, channel,
	sizeof names_channel) != 0) {
	err_reason = "failed to store names channel";
	goto bad;
    } else if (!strings_match_ignore_case(names_channel, channel)) {
	err_reason =
	    "unable to parse names of two (or more) channels simultaneously";
	goto bad;
    } else if ((win = window_by_label(channel)) == NULL) {
	err_reason = "window lookup error";
	goto bad;
    } else if (win->received_names) {
	err_log(0, "warning: server sent event 353 (RPL_NAMREPLY): "
	    "already received names for channel %s", channel);
	return;
    } else {
	names_copy = sw_strdup(*names == ':' ? &names[1] : &names[0]);
    }

    for (char *cp = &names_copy[0];; cp = NULL) {
	char *token = strtok_r(cp, " ", &state2);

	if (!token)
	    break;

	struct hInstall_context ctx = {
	    .channel    = channel,
	    .nick       = ((*token == '~' || *token == '&' || *token == '@' ||
			    *token == '%' || *token == '+')
			   ? &token[1]
			   : &token[0]),
	    .is_owner   = (*token == '~'),
	    .is_superop = (*token == '&'),
	    .is_op      = (*token == '@'),
	    .is_halfop  = (*token == '%'),
	    .is_voice   = (*token == '+'),
	};

	if (hInstall(&ctx) != OK)
	    break;
    }

    free(names_copy);
    return;

  bad:
    printtext_context_init(&ptext_ctx, g_active_window, TYPE_SPEC1_FAILURE,
	true);
    printtext(&ptext_ctx, "event_names: fatal: %s", err_reason);
    printtext(&ptext_ctx, "must shutdown irc connection immediately...");
    net_kill_connection();
    free(names_copy);
}

void
event_names_htbl_remove_all(PIRC_WINDOW window)
{
    PNAMES *entry_p;
    PNAMES p, tmp;

    if (window == NULL)
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
