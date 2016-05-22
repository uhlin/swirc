/* Handle event names (353) and event EOF names (366)
   Copyright (C) 2015, 2016 Markus Uhlin. All rights reserved.

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

#include "../dataClassify.h"
#include "../errHand.h"
#include "../irc.h"
#include "../libUtils.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../strdup_printf.h"
#include "../theme.h"

#include "names.h"

/* Structure definitions
   ===================== */

struct hInstall_context {
    char	*channel;
    char	*nick;
    bool	 is_op;
    bool	 is_halfop;
    bool	 is_voice;
};

struct names_chunk {
    char *nick1;
    char *nick2;
    char *nick3;
    char *nick4;
    char *nick5;
};

struct name_tag {
    char *s;
};

/* Objects with internal linkage
   ============================= */

static char names_channel[200] = "";

/* Function declarations
   ===================== */

/*lint -sem(next_names, r_null) */

static int			 names_cmp_fn     (const void *obj1, const void *obj2);
static struct names_chunk	*next_names       (PIRC_WINDOW, int *counter);
static unsigned int		 hash             (const char *nick);
static void			 free_names_chunk (struct names_chunk *);
static void			 hInstall         (const struct hInstall_context *);
static void			 hUndef           (PIRC_WINDOW, PNAMES);

void
event_names_init(void)
{
    BZERO(names_channel, sizeof names_channel);
}

void
event_names_deinit(void)
{
    window_foreach_destroy_names();
}

/* event_names: 353

   Example:
     :irc.server.com 353 <recipient> <chan type> <channel> :<nick 1> <nick 2> ...

   <chan type> can be either:
     @ for secret channels
     * for private channels
     = for public channels */
void
event_names(struct irc_message_compo *compo)
{
    char *state1, *state2;
    char *chan_type, *channel, *names;
    char *names_copy, *cp, *token;

    state1 = state2 = "";

    if (Strfeed(compo->params, 3) != 3) {
	goto bad;
    }

    (void) strtok_r(compo->params, "\n", &state1); /* recipient */
    chan_type = strtok_r(NULL, "\n", &state1);
    channel   = strtok_r(NULL, "\n", &state1);
    names     = strtok_r(NULL, "\n", &state1);

    if (chan_type == NULL || channel == NULL || names == NULL) {
	goto bad;
    }

    if (isEmpty(names_channel) && sw_strcpy(names_channel, channel, sizeof names_channel) != 0) {
	goto bad;
    } else if (!Strings_match_ignore_case(names_channel, channel)) {
	/* Unable to parse names of two (or more) channels simultaneously */
	goto bad;
    } else {
	names_copy = sw_strdup(*names == ':' ? &names[1] : &names[0]);
    }

    for (cp = &names_copy[0];; cp = NULL) {
	struct hInstall_context ctx;

	if ((token = strtok_r(cp, " ", &state2)) == NULL) {
	    break;
	}

	ctx.channel   = channel;
	ctx.nick      =
	    ((*token == '~' || *token == '&' || *token == '@' || *token == '%' || *token == '+')
	     ? &token[1]
	     : &token[0]);
	ctx.is_op     = (*token == '~' || *token == '&' || *token == '@');
	ctx.is_halfop = (*token == '%');
	ctx.is_voice  = (*token == '+');

	hInstall(&ctx);
    }

    free(names_copy);
    return;

  bad:
    err_msg("In event_names: FATAL ERROR. Aborting.");
    abort();
}

int
event_names_htbl_insert(const char *nick, const char *channel)
{
    struct hInstall_context ctx;

    if (isNull(nick) || isEmpty(nick)) {
	return ERR;
    }

    ctx.channel	  = (char *) channel;
    ctx.nick	  = (char *) nick;
    ctx.is_op	  = false;
    ctx.is_halfop = false;
    ctx.is_voice  = false;

    hInstall(&ctx);

    return OK;
}

static void
hInstall(const struct hInstall_context *ctx)
{
    PIRC_WINDOW		window_entry = window_by_label(ctx->channel);
    PNAMES		names_entry;
    const unsigned int	hashval      = hash(ctx->nick);

    names_entry		   = xcalloc(sizeof *names_entry, 1);
    names_entry->nick	   = sw_strdup(ctx->nick);
    names_entry->is_op	   = ctx->is_op;
    names_entry->is_halfop = ctx->is_halfop;
    names_entry->is_voice  = ctx->is_voice;

    if (window_entry) {
	names_entry->next		  = window_entry->names_hash[hashval];
	window_entry->names_hash[hashval] = names_entry;

	if (ctx->is_op)
	    window_entry->num_ops++;
	else if (ctx->is_halfop)
	    window_entry->num_halfops++;
	else if (ctx->is_voice)
	    window_entry->num_voices++;
	else
	    window_entry->num_normal++;

	window_entry->num_total++;
    } else {
	err_msg("FATAL: In events/names.c: Can't find a window with label %s", ctx->channel);
	abort();
    }
}

int
event_names_htbl_modify(const char *nick, const char *channel,
			bool is_op, bool is_halfop, bool is_voice)
{
    PIRC_WINDOW window;
    PNAMES	names;

    if (isNull(nick) || isEmpty(nick) || (window = window_by_label(channel)) == NULL) {
	return ERR;
    }

    for (names = window->names_hash[hash(nick)]; names != NULL; names = names->next) {
	if (Strings_match_ignore_case(nick, names->nick)) {
	    names->is_op     = is_op;
	    names->is_halfop = is_halfop;
	    names->is_voice  = is_voice;
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

    if (isNull(nick) || isEmpty(nick) || (window = window_by_label(channel)) == NULL) {
	return ERR;
    }

    for (names = window->names_hash[hash(nick)]; names != NULL; names = names->next) {
	if (Strings_match_ignore_case(nick, names->nick)) {
	    hUndef(window, names);
	    return OK;
	}
    }

    return ERR;
}

void
event_names_htbl_remove_all(PIRC_WINDOW window)
{
    PNAMES *entry_p;
    PNAMES p, tmp;

    if (window == NULL)
	return;

    for (entry_p = &window->names_hash[0]; entry_p < &window->names_hash[NAMES_HASH_TABLE_SIZE]; entry_p++) {
	for (p = *entry_p; p != NULL; p = tmp) {
	    tmp = p->next;
	    hUndef(window, p);
	}
    }
}

static void
hUndef(PIRC_WINDOW window, PNAMES entry)
{
    PNAMES tmp;
    const unsigned int hashval = hash(entry->nick);

    if ((tmp = window->names_hash[hashval]) == entry) {
	window->names_hash[hashval] = entry->next;
    } else {
	while (tmp->next != entry) {
	    tmp = tmp->next;
	}

	tmp->next = entry->next;
    }

    free_and_null(&entry->nick);

    if (entry->is_op)
	window->num_ops--;
    else if (entry->is_halfop)
	window->num_halfops--;
    else if (entry->is_voice)
	window->num_voices--;
    else
	window->num_normal--;

    window->num_total--;

#if 1
    entry->is_op = entry->is_halfop = entry->is_voice = 0;
#endif

    free_not_null(entry);
    entry = NULL;
}

static unsigned int
hash(const char *nick)
{
    char		 c;
    char		*nick_copy = str_tolower(sw_strdup(nick));
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

/* event_eof_names: 366

   Example:
     :irc.server.com 366 <recipient> <channel> :End of /NAMES list. */
void
event_eof_names(struct irc_message_compo *compo)
{
    PIRC_WINDOW window;
    char *channel;
    char *eof_msg;
    char *state = "";

    if (Strfeed(compo->params, 2) != 2) {
	goto bad;
    }

    (void) strtok_r(compo->params, "\n", &state);
    channel = strtok_r(NULL, "\n", &state);
    eof_msg = strtok_r(NULL, "\n", &state);

    if (channel == NULL || eof_msg == NULL || !Strings_match_ignore_case(channel, names_channel)) {
	goto bad;
    } else {
	BZERO(names_channel, sizeof names_channel);
    }

    if ((window = window_by_label(channel)) == NULL) {
	goto bad;
    } else {
	;
    }

    if (event_names_print_all(channel) != OK) {
	goto bad;
    }

    return;

  bad:
    err_msg("In event_eof_names: FATAL ERROR. Aborting...");
    abort();
}

int
event_names_print_all(const char *channel)
{
    PIRC_WINDOW			 window;
    int				 counter, i;
    int				 ntp1;
    struct name_tag		*names_array;
    struct names_chunk		*names;
    struct printtext_context	 ptext_ctx;

    if ((window = window_by_label(channel)) == NULL) {
	return ERR;
    }

    ptext_ctx.window     = window;
    ptext_ctx.spec_type  = TYPE_SPEC_NONE;
    ptext_ctx.include_ts = true;
    printtext(&ptext_ctx, "%s%sUsers %s%c%s", LEFT_BRKT, COLOR1, channel, NORMAL, RIGHT_BRKT);

    ntp1	= window->num_total + 1;
    names_array = xcalloc(ntp1, sizeof (struct name_tag));

    for (counter = i = 0; counter < NAMES_HASH_TABLE_SIZE; counter++) {
	if ((names = next_names(window, &counter)) != NULL) {
	    if (!isNull(names->nick1)) {
		names_array[i++].s = sw_strdup(names->nick1);
	    }

	    if (!isNull(names->nick2)) {
		names_array[i++].s = sw_strdup(names->nick2);
	    }

	    if (!isNull(names->nick3)) {
		names_array[i++].s = sw_strdup(names->nick3);
	    }

	    if (!isNull(names->nick4)) {
		names_array[i++].s = sw_strdup(names->nick4);
	    }

	    if (!isNull(names->nick5)) {
		names_array[i++].s = sw_strdup(names->nick5);
	    }
	} else {
	    return ERR;
	}

	free_names_chunk(names);
    }

    qsort(&names_array[0], ntp1, sizeof (struct name_tag), names_cmp_fn);

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
	    printtext(&ptext_ctx, "%s%-20s%s %s%-20s%s %s%-20s%s",
		      LEFT_BRKT, nick1, RIGHT_BRKT,
		      LEFT_BRKT, nick2, RIGHT_BRKT,
		      LEFT_BRKT, nick3, RIGHT_BRKT);
	} else if (nick1 && nick2) {
	    printtext(&ptext_ctx, "%s%-20s%s %s%-20s%s",
		      LEFT_BRKT, nick1, RIGHT_BRKT,
		      LEFT_BRKT, nick2, RIGHT_BRKT);
	} else if (nick1) {
	    printtext(&ptext_ctx, "%s%-20s%s", LEFT_BRKT, nick1, RIGHT_BRKT);
	} else {
	    ;
	}
    }

    for (i = 0; i < ntp1; i++) {
	free_not_null(names_array[i].s);
	names_array[i].s = NULL;
    }

    free(names_array);

    ptext_ctx.spec_type = TYPE_SPEC1;
    printtext(&ptext_ctx, "%s%s%s%c%s: Total of %c%d%c nicks %s%c%d%c ops, %c%d%c halfops, %c%d%c voices, %c%d%c normal%s",
	      LEFT_BRKT, COLOR1, channel, NORMAL, RIGHT_BRKT, BOLD, window->num_total, BOLD,
	      LEFT_BRKT, BOLD, window->num_ops, BOLD, BOLD, window->num_halfops, BOLD,
	      BOLD, window->num_voices, BOLD, BOLD, window->num_normal, BOLD, RIGHT_BRKT);

    return OK;
}

static struct names_chunk *
next_names(PIRC_WINDOW window, int *counter)
{
    struct names_chunk	*names	 = xcalloc(sizeof *names, 1);
    PNAMES		*entry_p = & (window->names_hash[*counter]);

    names->nick1 = names->nick2 = names->nick3 = names->nick4 = names->nick5 = NULL;

    for (PNAMES p = *entry_p; p != NULL; p = p->next) {
	char c;

	if (p->is_op) {
	    c = '@';
	} else if (p->is_halfop) {
	    c = '%';
	} else if (p->is_voice) {
	    c = '+';
	} else {
	    c = ' ';
	}

	if (isNull(names->nick1)) {
	    names->nick1 = Strdup_printf("%c%s", c, p->nick);
	} else if (isNull(names->nick2)) {
	    names->nick2 = Strdup_printf("%c%s", c, p->nick);
	} else if (isNull(names->nick3)) {
	    names->nick3 = Strdup_printf("%c%s", c, p->nick);
	} else if (isNull(names->nick4)) {
	    names->nick4 = Strdup_printf("%c%s", c, p->nick);
	} else if (isNull(names->nick5)) {
	    names->nick5 = Strdup_printf("%c%s", c, p->nick);
	} else { /* All busy. It's unlikely but it CAN happen. */
	    free_names_chunk(names);
	    return (NULL);
	}
    }

    return (names);
}

static void
free_names_chunk(struct names_chunk *names)
{
    free_not_null(names->nick1);
    free_not_null(names->nick2);
    free_not_null(names->nick3);
    free_not_null(names->nick4);
    free_not_null(names->nick5);

    free(names);
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

	if (*nick1 == '@' && (*nick2 == '%' || *nick2 == '+')) {
	    nick2++;
	} else if (*nick2 == '@' && (*nick1 == '%' || *nick1 == '+')) {
	    nick1++;
	} else if (*nick1 == '%' && *nick2 == '@') {
	    nick1++;
	} else if (*nick2 == '%' && *nick1 == '@') {
	    nick2++;
	} else if (*nick1 == '%' && *nick2 == '+') {
	    nick2++;
	} else if (*nick2 == '%' && *nick1 == '+') {
	    nick1++;
	} else if (*nick1 == '+' && (*nick2 == '@' || *nick2 == '%')) {
	    nick1++;
	} else if (*nick2 == '+' && (*nick1 == '@' || *nick1 == '%')) {
	    nick2++;
	} else {
	    ;
	}
    }

    return (strcmp(nick1, nick2));
}
