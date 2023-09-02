/* Readline tab completion
   Copyright (C) 2020-2023 Markus Uhlin. All rights reserved.

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

#include "commands/connect.h"
#include "commands/sasl.h"
#include "commands/services.h"
#include "commands/squery.h"
#include "commands/theme.h"
#include "commands/znc.h"

/* names.h wants this header before itself */
#include "irc.h"
#include "events/names.h"

#include "config.h"
#include "dataClassify.h"
#include "io-loop.h"
#include "printtext.h"
#include "readline.h"
#include "readlineTabCompletion.h"
#include "strHand.h"
#include "terminal.h"

static void
do_work(volatile struct readline_session_context *ctx, const wchar_t *cmd,
    CSTRING s)
{
	const size_t cmdlen = wcslen(cmd);
	const size_t slen = strlen(s);
	size_t i;

	while (ctx->numins != 0)
		readline_handle_backspace(ctx);
	for (i = 0; i < cmdlen; i++)
		readline_handle_key_exported(ctx, cmd[i]);
	for (i = 0; i < slen; i++)
		readline_handle_key_exported(ctx, btowc(s[i]));
}

static void
auto_complete_connect(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/connect ", s);
}

static void
auto_complete_cs(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/cs ", s);
}

static void
auto_complete_help(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/help ", s);
}

static void
auto_complete_msg(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/msg ", s);
}

static void
auto_complete_notice(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/notice ", s);
}

static void
auto_complete_ns(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/ns ", s);
}

static void
auto_complete_query(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/query ", s);
}

static void
auto_complete_sasl(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/sasl ", s);
}

static void
auto_complete_setting(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/set ", s);
}

static void
auto_complete_squery(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/squery ", s);
}

static void
auto_complete_theme(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/theme ", s);
}

static void
auto_complete_time(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/time ", s);
}

static void
auto_complete_version(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/version ", s);
}

static void
auto_complete_whois(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/whois ", s);
}

static void
auto_complete_znc_cmd(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/znc ", s);
}

static void
auto_complete_command(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	const size_t slen = strlen(s);

	while (ctx->numins != 0)
		readline_handle_backspace(ctx);

	readline_handle_key_exported(ctx, L'/');

	for (size_t i = 0; i < slen; i++)
		readline_handle_key_exported(ctx, btowc(s[i]));
}

static void
auto_complete_channel_user(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	const size_t slen = strlen(s);

	while (ctx->numins != 0)
		readline_handle_backspace(ctx);
	for (size_t i = 0; i < slen; i++)
		readline_handle_key_exported(ctx, btowc(s[i]));
	readline_handle_key_exported(ctx, L':');
	readline_handle_key_exported(ctx, L' ');
}

static bool
buf_contains_disallowed_chars(const volatile struct readline_session_context *ctx)
{
	STRING s = readline_finalize_out_string_exported(ctx->buffer);
	const bool yes_no = (strpbrk(s, g_textdeco_chars) != NULL);
	free(s);
	return yes_no;
}

static PTEXTBUF
get_matches_common(CSTRING srch)
{
	if (!is_irc_channel(ACTWINLABEL)) {
		if (!is_irc_channel(srch))
			return get_list_of_matching_queries(srch);
		else
			return get_list_of_matching_channels(srch);
	}
	if (is_irc_channel(srch))
		return get_list_of_matching_channels(srch);
	return get_list_of_matching_channel_users(ACTWINLABEL, srch);
}

static inline STRING
get_search_var(const volatile struct readline_session_context *ctx)
{
	return addrof(ctx->tc->search_var[0]);
}

static void
output_error(CSTRING msg)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_WARN, true);
	printtext(&ctx, "%s", msg);
	term_beep();
}

static int
store_search_var(const volatile struct readline_session_context *ctx)
{
	STRING s = readline_finalize_out_string_exported(ctx->buffer);

	const int store_res = sw_strcpy(ctx->tc->search_var, s,
	    ARRAY_SIZE(ctx->tc->search_var));

	free(s);
	return (store_res != 0 ? -1 : 0);
}

PTAB_COMPLETION
readline_tab_comp_ctx_new(void)
{
	static TAB_COMPLETION	ctx = { 0 };

	BZERO(ctx.search_var, sizeof ctx.search_var);

	ctx.isInCirculationModeForConnect	= false;
	ctx.isInCirculationModeForCs		= false;
	ctx.isInCirculationModeForHelp		= false;
	ctx.isInCirculationModeForMsg		= false;
	ctx.isInCirculationModeForNotice	= false;
	ctx.isInCirculationModeForNs		= false;
	ctx.isInCirculationModeForQuery		= false;
	ctx.isInCirculationModeForSasl		= false;
	ctx.isInCirculationModeForSettings	= false;
	ctx.isInCirculationModeForSquery	= false;
	ctx.isInCirculationModeForTheme		= false;
	ctx.isInCirculationModeForTime		= false;
	ctx.isInCirculationModeForVersion	= false;
	ctx.isInCirculationModeForWhois		= false;
	ctx.isInCirculationModeForZncCmds	= false;
	ctx.isInCirculationModeForCmds		= false;
	ctx.isInCirculationModeForChanUsers	= false;

	ctx.matches = NULL;
	ctx.elmt = NULL;

	return (&ctx);
}

void
readline_tab_comp_ctx_destroy(PTAB_COMPLETION ctx)
{
	if (ctx != NULL && ctx->matches != NULL)
		textBuf_destroy(ctx->matches);
}

void
readline_tab_comp_ctx_reset(PTAB_COMPLETION ctx)
{
	if (ctx) {
		BZERO(ctx->search_var, sizeof ctx->search_var);

		ctx->isInCirculationModeForConnect	= false;
		ctx->isInCirculationModeForCs		= false;
		ctx->isInCirculationModeForHelp		= false;
		ctx->isInCirculationModeForMsg		= false;
		ctx->isInCirculationModeForNotice	= false;
		ctx->isInCirculationModeForNs		= false;
		ctx->isInCirculationModeForQuery	= false;
		ctx->isInCirculationModeForSasl		= false;
		ctx->isInCirculationModeForSettings	= false;
		ctx->isInCirculationModeForSquery	= false;
		ctx->isInCirculationModeForTheme	= false;
		ctx->isInCirculationModeForTime		= false;
		ctx->isInCirculationModeForVersion	= false;
		ctx->isInCirculationModeForWhois	= false;
		ctx->isInCirculationModeForZncCmds	= false;
		ctx->isInCirculationModeForCmds		= false;
		ctx->isInCirculationModeForChanUsers	= false;

		if (ctx->matches != NULL)
			textBuf_destroy(ctx->matches);

		ctx->matches = NULL;
		ctx->elmt = NULL;
	}
}

static void
init_mode_for_connect(volatile struct readline_session_context *ctx)
{
	const char *cp = addrof(ctx->tc->search_var[9]);

	if ((ctx->tc->matches = get_list_of_matching_connect_cmds(cp)) ==
	    NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_connect(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForConnect = true;
}

static void
init_mode_for_cs(volatile struct readline_session_context *ctx,
    const int offset)
{
	const char *cp = addrof(ctx->tc->search_var[offset]);

	if ((ctx->tc->matches = get_list_of_matching_cs_cmds(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_cs(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForCs = true;
}

static void
init_mode_for_help(volatile struct readline_session_context *ctx)
{
	const char *cp = addrof(ctx->tc->search_var[6]);

	if ((ctx->tc->matches = get_list_of_matching_commands(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_help(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForHelp = true;
}

static void
init_mode_for_msg(volatile struct readline_session_context *ctx)
{
	const char *cp = addrof(ctx->tc->search_var[5]);

	if ((ctx->tc->matches = get_matches_common(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_msg(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForMsg = true;
}

static void
init_mode_for_notice(volatile struct readline_session_context *ctx)
{
	const char *cp = addrof(ctx->tc->search_var[8]);

	if ((ctx->tc->matches = get_matches_common(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_notice(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForNotice = true;
}

static void
init_mode_for_ns(volatile struct readline_session_context *ctx,
    const int offset)
{
	const char *cp = addrof(ctx->tc->search_var[offset]);

	if ((ctx->tc->matches = get_list_of_matching_ns_cmds(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_ns(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForNs = true;
}

static void
init_mode_for_query(volatile struct readline_session_context *ctx)
{
	const char *cp;

	if (!is_irc_channel(ACTWINLABEL)) {
		output_error("not in irc channel");
		return;
	}

	cp = addrof(ctx->tc->search_var[7]);

	if ((ctx->tc->matches = get_list_of_matching_channel_users(ACTWINLABEL,
	    cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_query(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForQuery = true;
}

static void
init_mode_for_sasl(volatile struct readline_session_context *ctx)
{
	const char *cp = addrof(ctx->tc->search_var[6]);

	if ((ctx->tc->matches = get_list_of_matching_sasl_cmds(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_sasl(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForSasl = true;
}

static void
init_mode_for_set(volatile struct readline_session_context *ctx)
{
	const char *cp = addrof(ctx->tc->search_var[5]);

	if ((ctx->tc->matches = get_list_of_matching_settings(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_setting(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForSettings = true;
}

static void
init_mode_for_squery(volatile struct readline_session_context *ctx)
{
	const char *cp = addrof(ctx->tc->search_var[8]);

	if ((ctx->tc->matches = get_list_of_matching_squery_commands(cp)) ==
	    NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_squery(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForSquery = true;
}

static void
init_mode_for_theme(volatile struct readline_session_context *ctx)
{
	const char *cp = addrof(ctx->tc->search_var[7]);

	if ((ctx->tc->matches = get_list_of_matching_theme_cmds(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_theme(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForTheme = true;
}

static void
init_mode_for_time(volatile struct readline_session_context *ctx)
{
	const char *cp = addrof(ctx->tc->search_var[6]);

	if ((ctx->tc->matches = get_matches_common(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_time(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForTime = true;
}

static void
init_mode_for_version(volatile struct readline_session_context *ctx)
{
	const char *cp = addrof(ctx->tc->search_var[9]);

	if ((ctx->tc->matches = get_matches_common(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_version(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForVersion = true;
}

static void
init_mode_for_whois(volatile struct readline_session_context *ctx)
{
	const char *cp = addrof(ctx->tc->search_var[7]);

	if (!is_irc_channel(ACTWINLABEL)) {
		ctx->tc->matches = get_list_of_matching_queries(cp);
	} else {
		ctx->tc->matches = get_list_of_matching_channel_users
		    (ACTWINLABEL, cp);
	}

	if (ctx->tc->matches == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_whois(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForWhois = true;
}

static void
init_mode_for_znc_cmds(volatile struct readline_session_context *ctx)
{
	const char *cp = addrof(ctx->tc->search_var[5]);

	if ((ctx->tc->matches = get_list_of_matching_znc_commands(cp)) ==
	    NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_znc_cmd(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForZncCmds = true;
}

static void
init_mode_for_commands(volatile struct readline_session_context *ctx,
    const bool numins_greater_than_one)
{
	const char *cp = addrof(ctx->tc->search_var[1]);

	if (!numins_greater_than_one || (ctx->tc->matches =
	    get_list_of_matching_commands(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_command(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForCmds = true;
}

static void
init_mode_for_channel_users(volatile struct readline_session_context *ctx)
{
	if ((ctx->tc->matches = get_list_of_matching_channel_users(ACTWINLABEL,
	    get_search_var(ctx))) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_channel_user(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForChanUsers = true;
}

static STRING
next_text(PTAB_COMPLETION tc)
{
	tc->elmt = tc->elmt->next;
	return tc->elmt->text;
}

static void
no_more_matches(volatile struct readline_session_context *ctx)
{
	output_error("no more matches");
	readline_tab_comp_ctx_reset(ctx->tc);
}

static bool
var_matches_cs(CSTRING sv, int *iout)
{
	if (!strncmp(sv, "/chanserv ", 10))
		*iout = 10;
	else if (!strncmp(sv, "/cs ", 4))
		*iout = 4;
	else
		*iout = 0;
	return (!strncmp(sv, "/chanserv ", 10) || !strncmp(sv, "/cs ", 4));
}

static bool
var_matches_ns(CSTRING sv, int *iout)
{
	if (!strncmp(sv, "/nickserv ", 10))
		*iout = 10;
	else if (!strncmp(sv, "/ns ", 4))
		*iout = 4;
	else
		*iout = 0;
	return (!strncmp(sv, "/nickserv ", 10) || !strncmp(sv, "/ns ", 4));
}

static void
init_mode(volatile struct readline_session_context *ctx)
{
	CSTRING sv = get_search_var(ctx);
	int off1, off2;

	if (!strncmp(sv, "/connect ", 9))
		init_mode_for_connect(ctx);
	else if (var_matches_cs(sv, &off1))
		init_mode_for_cs(ctx, off1);
	else if (!strncmp(sv, "/help ", 6))
		init_mode_for_help(ctx);
	else if (!strncmp(sv, "/msg ", 5))
		init_mode_for_msg(ctx);
	else if (!strncmp(sv, "/notice ", 8))
		init_mode_for_notice(ctx);
	else if (var_matches_ns(sv, &off2))
		init_mode_for_ns(ctx, off2);
	else if (!strncmp(sv, "/query ", 7))
		init_mode_for_query(ctx);
	else if (!strncmp(sv, "/sasl ", 6))
		init_mode_for_sasl(ctx);
	else if (!strncmp(sv, "/set ", 5))
		init_mode_for_set(ctx);
	else if (!strncmp(sv, "/squery ", 8))
		init_mode_for_squery(ctx);
	else if (!strncmp(sv, "/theme ", 7))
		init_mode_for_theme(ctx);
	else if (!strncmp(sv, "/time ", 6))
		init_mode_for_time(ctx);
	else if (!strncmp(sv, "/version ", 9))
		init_mode_for_version(ctx);
	else if (!strncmp(sv, "/whois ", 7))
		init_mode_for_whois(ctx);
	else if (!strncmp(sv, "/znc ", 5))
		init_mode_for_znc_cmds(ctx);
	else if (ctx->tc->search_var[0] == '/')
		init_mode_for_commands(ctx, (ctx->numins > 1));
	else if (is_irc_channel(ACTWINLABEL))
		init_mode_for_channel_users(ctx);
}

static void
ac_doit(AC_FUNC ac, volatile struct readline_session_context *ctx)
{
	if (ctx->tc->elmt == textBuf_tail(ctx->tc->matches))
		no_more_matches(ctx);
	else
		ac(ctx, next_text(ctx->tc));
}

/*
 * Handles a TAB key press. It initially checks if tabbing is
 * possible. After that it checks if it's already in circulation for a
 * mode and auto completes. Else it attempts to store the search
 * variable and initializes completion for a mode depending on what's
 * stored in the search variable.
 */
void
readline_handle_tab(volatile struct readline_session_context *ctx)
{
	if (ctx->numins == 0 || ctx->insert_mode ||
	    buf_contains_disallowed_chars(ctx)) {
		output_error("no magic");
		readline_tab_comp_ctx_reset(ctx->tc);
		return;
	} else if (ctx->tc->isInCirculationModeForConnect) {
		ac_doit(auto_complete_connect, ctx);
	} else if (ctx->tc->isInCirculationModeForCs) {
		ac_doit(auto_complete_cs, ctx);
	} else if (ctx->tc->isInCirculationModeForHelp) {
		ac_doit(auto_complete_help, ctx);
	} else if (ctx->tc->isInCirculationModeForMsg) {
		ac_doit(auto_complete_msg, ctx);
	} else if (ctx->tc->isInCirculationModeForNotice) {
		ac_doit(auto_complete_notice, ctx);
	} else if (ctx->tc->isInCirculationModeForNs) {
		ac_doit(auto_complete_ns, ctx);
	} else if (ctx->tc->isInCirculationModeForQuery) {
		ac_doit(auto_complete_query, ctx);
	} else if (ctx->tc->isInCirculationModeForSasl) {
		ac_doit(auto_complete_sasl, ctx);
	} else if (ctx->tc->isInCirculationModeForSettings) {
		ac_doit(auto_complete_setting, ctx);
	} else if (ctx->tc->isInCirculationModeForSquery) {
		ac_doit(auto_complete_squery, ctx);
	} else if (ctx->tc->isInCirculationModeForTheme) {
		ac_doit(auto_complete_theme, ctx);
	} else if (ctx->tc->isInCirculationModeForTime) {
		ac_doit(auto_complete_time, ctx);
	} else if (ctx->tc->isInCirculationModeForVersion) {
		ac_doit(auto_complete_version, ctx);
	} else if (ctx->tc->isInCirculationModeForWhois) {
		ac_doit(auto_complete_whois, ctx);
	} else if (ctx->tc->isInCirculationModeForZncCmds) {
		ac_doit(auto_complete_znc_cmd, ctx);
	} else if (ctx->tc->isInCirculationModeForCmds) {
		ac_doit(auto_complete_command, ctx);
	} else if (ctx->tc->isInCirculationModeForChanUsers) {
		ac_doit(auto_complete_channel_user, ctx);
	} else if (store_search_var(ctx) == -1) {
		output_error("cannot store search variable");
		return;
	} else
		init_mode(ctx);
}
