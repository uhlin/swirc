/* Readline tab completion
   Copyright (C) 2020-2022 Markus Uhlin. All rights reserved.

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

#include "commands/sasl.h"
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
    const char *s)
{
	size_t	i;

	while (ctx->n_insert != 0)
		readline_handle_backspace(ctx);
	for (i = 0; i < wcslen(cmd); i++)
		readline_handle_key_exported(ctx, cmd[i]);
	for (i = 0; i < strlen(s); i++)
		readline_handle_key_exported(ctx, btowc(s[i]));
}

static void
auto_complete_help(volatile struct readline_session_context *ctx,
    const char *s)
{
	do_work(ctx, L"/help ", s);
}

static void
auto_complete_query(volatile struct readline_session_context *ctx,
    const char *s)
{
	do_work(ctx, L"/query ", s);
}

static void
auto_complete_sasl(volatile struct readline_session_context *ctx,
    const char *s)
{
	do_work(ctx, L"/sasl ", s);
}

static void
auto_complete_setting(volatile struct readline_session_context *ctx,
    const char *s)
{
	do_work(ctx, L"/set ", s);
}

static void
auto_complete_whois(volatile struct readline_session_context *ctx,
    const char *s)
{
	do_work(ctx, L"/whois ", s);
}

static void
auto_complete_znc_cmd(volatile struct readline_session_context *ctx,
    const char *s)
{
	do_work(ctx, L"/znc ", s);
}

static void
auto_complete_command(volatile struct readline_session_context *ctx,
    const char *s)
{
	while (ctx->n_insert != 0)
		readline_handle_backspace(ctx);

	readline_handle_key_exported(ctx, btowc('/'));

	for (size_t i = 0; i < strlen(s); i++)
		readline_handle_key_exported(ctx, btowc(s[i]));
}

static void
auto_complete_channel_user(volatile struct readline_session_context *ctx,
    const char *s)
{
	while (ctx->n_insert != 0)
		readline_handle_backspace(ctx);
	for (size_t i = 0; i < strlen(s); i++)
		readline_handle_key_exported(ctx, btowc(s[i]));
	readline_handle_key_exported(ctx, btowc(':'));
	readline_handle_key_exported(ctx, btowc(' '));
}

static bool
buf_contains_disallowed_chars(const volatile struct readline_session_context *ctx)
{
	char *s = readline_finalize_out_string_exported(ctx->buffer);
	const bool yes_no = (strpbrk(s, g_textdeco_chars) != NULL);
	free(s);
	return yes_no;
}

static inline char *
get_search_var(const volatile struct readline_session_context *ctx)
{
	return addrof(ctx->tc->search_var[0]);
}

static void
output_error(const char *msg)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_WARN, true);
	printtext(&ctx, "%s", msg);
	term_beep();
}

static int
store_search_var(const volatile struct readline_session_context *ctx)
{
	char *s = readline_finalize_out_string_exported(ctx->buffer);

	const int store_res = sw_strcpy(ctx->tc->search_var, s,
	    ARRAY_SIZE(ctx->tc->search_var));

	free(s);
	return (store_res != 0 ? -1 : 0);
}

PTAB_COMPLETION
readline_tab_comp_ctx_new(void)
{
	static TAB_COMPLETION	ctx;

	BZERO(ctx.search_var, sizeof ctx.search_var);

	ctx.isInCirculationModeForHelp		= false;
	ctx.isInCirculationModeForQuery		= false;
	ctx.isInCirculationModeForSasl		= false;
	ctx.isInCirculationModeForSettings	= false;
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

		ctx->isInCirculationModeForHelp		= false;
		ctx->isInCirculationModeForQuery	= false;
		ctx->isInCirculationModeForSasl		= false;
		ctx->isInCirculationModeForSettings	= false;
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
init_mode_for_help(volatile struct readline_session_context *ctx)
{
	char	*p = addrof(ctx->tc->search_var[6]);

	if ((ctx->tc->matches = get_list_of_matching_commands(p)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_help(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForHelp = true;
}

static void
init_mode_for_query(volatile struct readline_session_context *ctx)
{
	char	*p;

	if (!is_irc_channel(ACTWINLABEL)) {
		output_error("not in irc channel");
		return;
	}

	p = addrof(ctx->tc->search_var[7]);

	if ((ctx->tc->matches = get_list_of_matching_channel_users(ACTWINLABEL,
	    p)) == NULL) {
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
	char	*p;

	p = addrof(ctx->tc->search_var[6]);

	if ((ctx->tc->matches = get_list_of_matching_sasl_cmds(p)) == NULL) {
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
	char	*p = addrof(ctx->tc->search_var[5]);

	if ((ctx->tc->matches = get_list_of_matching_settings(p)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_setting(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForSettings = true;
}

static void
init_mode_for_whois(volatile struct readline_session_context *ctx)
{
	char	*p;

	if (!is_irc_channel(ACTWINLABEL)) {
		output_error("not in irc channel");
		return;
	}

	p = addrof(ctx->tc->search_var[7]);

	if ((ctx->tc->matches = get_list_of_matching_channel_users(ACTWINLABEL,
	    p)) == NULL) {
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
	char	*p = addrof(ctx->tc->search_var[5]);

	if ((ctx->tc->matches = get_list_of_matching_znc_commands(p)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_znc_cmd(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeForZncCmds = true;
}

static void
init_mode_for_commands(volatile struct readline_session_context *ctx,
    const bool n_insert_greater_than_one)
{
	char	*p = addrof(ctx->tc->search_var[1]);

	if (!n_insert_greater_than_one || (ctx->tc->matches =
	    get_list_of_matching_commands(p)) == NULL) {
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

static char *
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

void
readline_handle_tab(volatile struct readline_session_context *ctx)
{
	if (ctx->n_insert == 0 || ctx->insert_mode ||
	    buf_contains_disallowed_chars(ctx)) {
		output_error("no magic");
		readline_tab_comp_ctx_reset(ctx->tc);
		return;
	} else if (ctx->tc->isInCirculationModeForHelp) {
		if (ctx->tc->elmt == textBuf_tail(ctx->tc->matches))
			no_more_matches(ctx);
		else
			auto_complete_help(ctx, next_text(ctx->tc));
		return;
	} else if (ctx->tc->isInCirculationModeForQuery) {
		if (ctx->tc->elmt == textBuf_tail(ctx->tc->matches))
			no_more_matches(ctx);
		else
			auto_complete_query(ctx, next_text(ctx->tc));
		return;
	} else if (ctx->tc->isInCirculationModeForSasl) {
		if (ctx->tc->elmt == textBuf_tail(ctx->tc->matches))
			no_more_matches(ctx);
		else
			auto_complete_sasl(ctx, next_text(ctx->tc));
		return;
	} else if (ctx->tc->isInCirculationModeForSettings) {
		if (ctx->tc->elmt == textBuf_tail(ctx->tc->matches))
			no_more_matches(ctx);
		else
			auto_complete_setting(ctx, next_text(ctx->tc));
		return;
	} else if (ctx->tc->isInCirculationModeForWhois) {
		if (ctx->tc->elmt == textBuf_tail(ctx->tc->matches))
			no_more_matches(ctx);
		else
			auto_complete_whois(ctx, next_text(ctx->tc));
		return;
	} else if (ctx->tc->isInCirculationModeForZncCmds) {
		if (ctx->tc->elmt == textBuf_tail(ctx->tc->matches))
			no_more_matches(ctx);
		else
			auto_complete_znc_cmd(ctx, next_text(ctx->tc));
		return;
	} else if (ctx->tc->isInCirculationModeForCmds) {
		if (ctx->tc->elmt == textBuf_tail(ctx->tc->matches))
			no_more_matches(ctx);
		else
			auto_complete_command(ctx, next_text(ctx->tc));
		return;
	} else if (ctx->tc->isInCirculationModeForChanUsers) {
		if (ctx->tc->elmt == textBuf_tail(ctx->tc->matches))
			no_more_matches(ctx);
		else
			auto_complete_channel_user(ctx, next_text(ctx->tc));
		return;
	}

	if (store_search_var(ctx) == -1) {
		output_error("cannot store search variable");
		return;
	}

	const bool is_command = (ctx->tc->search_var[0] == '/');

	if (!strncmp(get_search_var(ctx), "/help ", 6))
		init_mode_for_help(ctx);
	else if (!strncmp(get_search_var(ctx), "/query ", 7))
		init_mode_for_query(ctx);
	else if (!strncmp(get_search_var(ctx), "/sasl ", 6))
		init_mode_for_sasl(ctx);
	else if (!strncmp(get_search_var(ctx), "/set ", 5))
		init_mode_for_set(ctx);
	else if (!strncmp(get_search_var(ctx), "/whois ", 7))
		init_mode_for_whois(ctx);
	else if (!strncmp(get_search_var(ctx), "/znc ", 5))
		init_mode_for_znc_cmds(ctx);
	else if (is_command)
		init_mode_for_commands(ctx, (ctx->n_insert > 1));
	else if (is_irc_channel(ACTWINLABEL))
		init_mode_for_channel_users(ctx);
}
