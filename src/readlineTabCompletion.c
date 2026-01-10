/* Readline tab completion
   Copyright (C) 2020-2026 Markus Uhlin. All rights reserved.

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
#include "commands/dcc.h"
#include "commands/ftp.h"
#include "commands/log.h"
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

static CSTRING service_hosts[] = {
	//lint -e786
	servhost_macro(chanserv_host),
	servhost_macro(nickserv_host),
	//lint +e786
};

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
auto_complete_dcc(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/dcc ", s);
}

static void
auto_complete_deop(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/deop ", s);
}

static void
auto_complete_devoice(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/devoice ", s);
}

static void
auto_complete_ftp(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/ftp ", s);
}

static void
auto_complete_help(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/help ", s);
}

static void
auto_complete_kick(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/kick ", s);
}

static void
auto_complete_kickban(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/kickban ", s);
}

static void
auto_complete_log(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/log ", s);
}

static void
auto_complete_mode(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/mode ", s);
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
auto_complete_op(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/op ", s);
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
auto_complete_voice(volatile struct readline_session_context *ctx,
    CSTRING s)
{
	do_work(ctx, L"/voice ", s);
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

	ctx.isInCirculationModeFor.ChanUsers =
	ctx.isInCirculationModeFor.Cmds =
	ctx.isInCirculationModeFor.Connect =
	ctx.isInCirculationModeFor.Cs =
	ctx.isInCirculationModeFor.Dcc =
	ctx.isInCirculationModeFor.Deop =
	ctx.isInCirculationModeFor.Devoice =
	ctx.isInCirculationModeFor.Ftp =
	ctx.isInCirculationModeFor.Help =
	ctx.isInCirculationModeFor.Kick =
	ctx.isInCirculationModeFor.Kickban =
	ctx.isInCirculationModeFor.Log =
	ctx.isInCirculationModeFor.Mode =
	ctx.isInCirculationModeFor.Msg =
	ctx.isInCirculationModeFor.Notice =
	ctx.isInCirculationModeFor.Ns =
	ctx.isInCirculationModeFor.Op =
	ctx.isInCirculationModeFor.Query =
	ctx.isInCirculationModeFor.Sasl =
	ctx.isInCirculationModeFor.Settings =
	ctx.isInCirculationModeFor.Squery =
	ctx.isInCirculationModeFor.Theme =
	ctx.isInCirculationModeFor.Time =
	ctx.isInCirculationModeFor.Version =
	ctx.isInCirculationModeFor.Voice =
	ctx.isInCirculationModeFor.Whois =
	ctx.isInCirculationModeFor.ZncCmds = false;

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

		ctx->isInCirculationModeFor.ChanUsers =
		ctx->isInCirculationModeFor.Cmds =
		ctx->isInCirculationModeFor.Connect =
		ctx->isInCirculationModeFor.Cs =
		ctx->isInCirculationModeFor.Dcc =
		ctx->isInCirculationModeFor.Deop =
		ctx->isInCirculationModeFor.Devoice =
		ctx->isInCirculationModeFor.Ftp =
		ctx->isInCirculationModeFor.Help =
		ctx->isInCirculationModeFor.Kick =
		ctx->isInCirculationModeFor.Kickban =
		ctx->isInCirculationModeFor.Log =
		ctx->isInCirculationModeFor.Mode =
		ctx->isInCirculationModeFor.Msg =
		ctx->isInCirculationModeFor.Notice =
		ctx->isInCirculationModeFor.Ns =
		ctx->isInCirculationModeFor.Op =
		ctx->isInCirculationModeFor.Query =
		ctx->isInCirculationModeFor.Sasl =
		ctx->isInCirculationModeFor.Settings =
		ctx->isInCirculationModeFor.Squery =
		ctx->isInCirculationModeFor.Theme =
		ctx->isInCirculationModeFor.Time =
		ctx->isInCirculationModeFor.Version =
		ctx->isInCirculationModeFor.Voice =
		ctx->isInCirculationModeFor.Whois =
		ctx->isInCirculationModeFor.ZncCmds = false;

		if (ctx->matches != NULL)
			textBuf_destroy(ctx->matches);

		ctx->matches = NULL;
		ctx->elmt = NULL;
	}
}

static void
init_mode_for_connect(volatile struct readline_session_context *ctx)
{
	immutable_cp_t cp = addrof(ctx->tc->search_var[9]);

	if ((ctx->tc->matches = get_list_of_matching_connect_cmds(cp)) ==
	    NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_connect(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Connect = true;
}

static void
init_mode_for_cs(volatile struct readline_session_context *ctx,
    const int offset)
{
	immutable_cp_t cp = addrof(ctx->tc->search_var[offset]);

	if ((ctx->tc->matches = get_list_of_matching_cs_cmds(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_cs(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Cs = true;
}

static void
init_mode_for_dcc(volatile struct readline_session_context *ctx)
{
	immutable_cp_t cp = addrof(ctx->tc->search_var[5]);

	if ((ctx->tc->matches = get_list_of_matching_dcc_cmds(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_dcc(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Dcc = true;
}

static void
init_mode_for_deop(volatile struct readline_session_context *ctx)
{
	const char *cp;

	if (!is_irc_channel(ACTWINLABEL)) {
		output_error("not in irc channel");
		return;
	}

	cp = addrof(ctx->tc->search_var[6]);

	if ((ctx->tc->matches = get_list_of_matching_channel_users(ACTWINLABEL,
	    cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_deop(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Deop = true;
}

static void
init_mode_for_devoice(volatile struct readline_session_context *ctx)
{
	const char *cp;

	if (!is_irc_channel(ACTWINLABEL)) {
		output_error("not in irc channel");
		return;
	}

	cp = addrof(ctx->tc->search_var[9]);

	if ((ctx->tc->matches = get_list_of_matching_channel_users(ACTWINLABEL,
	    cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_devoice(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Devoice = true;
}

static void
init_mode_for_ftp(volatile struct readline_session_context *ctx)
{
	immutable_cp_t cp = addrof(ctx->tc->search_var[5]);

	if ((ctx->tc->matches = get_list_of_matching_ftp_cmds(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_ftp(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Ftp = true;
}

static void
init_mode_for_help(volatile struct readline_session_context *ctx)
{
	immutable_cp_t cp = addrof(ctx->tc->search_var[6]);

	if ((ctx->tc->matches = get_list_of_matching_commands(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_help(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Help = true;
}

static void
init_mode_for_kick(volatile struct readline_session_context *ctx)
{
	const char *cp;

	if (!is_irc_channel(ACTWINLABEL)) {
		output_error("not in irc channel");
		return;
	}

	cp = addrof(ctx->tc->search_var[6]);

	if ((ctx->tc->matches = get_list_of_matching_channel_users(ACTWINLABEL,
	    cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_kick(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Kick = true;
}

static void
init_mode_for_kickban(volatile struct readline_session_context *ctx)
{
	const char *cp;

	if (!is_irc_channel(ACTWINLABEL)) {
		output_error("not in irc channel");
		return;
	}

	cp = addrof(ctx->tc->search_var[9]);

	if ((ctx->tc->matches = get_list_of_matching_channel_users(ACTWINLABEL,
	    cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_kickban(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Kickban = true;
}

static void
init_mode_for_log(volatile struct readline_session_context *ctx)
{
	immutable_cp_t cp = addrof(ctx->tc->search_var[5]);

	if ((ctx->tc->matches = get_list_of_matching_log_cmds(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_log(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Log = true;
}

static void
init_mode_for_mode(volatile struct readline_session_context *ctx)
{
	immutable_cp_t cp = addrof(ctx->tc->search_var[6]);

	if ((ctx->tc->matches = get_matches_common(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_mode(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Mode = true;
}

static void
init_mode_for_msg(volatile struct readline_session_context *ctx)
{
	immutable_cp_t cp = addrof(ctx->tc->search_var[5]);

	if ((ctx->tc->matches = get_matches_common(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_msg(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Msg = true;
}

static void
init_mode_for_notice(volatile struct readline_session_context *ctx)
{
	immutable_cp_t cp = addrof(ctx->tc->search_var[8]);

	if ((ctx->tc->matches = get_matches_common(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_notice(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Notice = true;
}

static void
init_mode_for_ns(volatile struct readline_session_context *ctx,
    const int offset)
{
	immutable_cp_t cp = addrof(ctx->tc->search_var[offset]);

	if ((ctx->tc->matches = get_list_of_matching_ns_cmds(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_ns(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Ns = true;
}

static void
init_mode_for_op(volatile struct readline_session_context *ctx)
{
	const char *cp;

	if (!is_irc_channel(ACTWINLABEL)) {
		output_error("not in irc channel");
		return;
	}

	cp = addrof(ctx->tc->search_var[4]);

	if ((ctx->tc->matches = get_list_of_matching_channel_users(ACTWINLABEL,
	    cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_op(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Op = true;
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
	ctx->tc->isInCirculationModeFor.Query = true;
}

static void
init_mode_for_sasl(volatile struct readline_session_context *ctx)
{
	immutable_cp_t cp = addrof(ctx->tc->search_var[6]);

	if ((ctx->tc->matches = get_list_of_matching_sasl_cmds(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_sasl(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Sasl = true;
}

//lint -sem(get_service_hosts, r_null)
static PTEXTBUF
get_service_hosts(const char *search_var)
{
	PTEXTBUF	matches = textBuf_new();
	const size_t	varlen = strlen(search_var);

	for (size_t i = 0; i < ARRAY_SIZE(service_hosts); i++) {
		if (!strncmp(search_var, service_hosts[i], varlen)) {
			textBuf_emplace_back(__func__, matches,
			    service_hosts[i], 0);
		}
	}

	if (textBuf_size(matches) == 0) {
		textBuf_destroy(matches);
		return NULL;
	}

	return matches;
}

static void
init_mode_for_set(volatile struct readline_session_context *ctx)
{
	immutable_cp_t cp = addrof(ctx->tc->search_var[5]);

	if (strncmp(cp, "chanserv_host ", 14) == 0 ||
	    strncmp(cp, "nickserv_host ", 14) == 0) {
		if ((ctx->tc->matches = get_service_hosts(cp)) == NULL) {
			output_error("no magic");
			return;
		}
	} else if ((ctx->tc->matches = get_list_of_matching_settings(cp)) ==
	    NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_setting(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Settings = true;
}

static void
init_mode_for_squery(volatile struct readline_session_context *ctx)
{
	immutable_cp_t cp = addrof(ctx->tc->search_var[8]);

	if ((ctx->tc->matches = get_list_of_matching_squery_commands(cp)) ==
	    NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_squery(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Squery = true;
}

static void
init_mode_for_theme(volatile struct readline_session_context *ctx)
{
	immutable_cp_t cp = addrof(ctx->tc->search_var[7]);

	if ((ctx->tc->matches = get_list_of_matching_theme_cmds(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_theme(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Theme = true;
}

static void
init_mode_for_time(volatile struct readline_session_context *ctx)
{
	immutable_cp_t cp = addrof(ctx->tc->search_var[6]);

	if ((ctx->tc->matches = get_matches_common(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_time(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Time = true;
}

static void
init_mode_for_version(volatile struct readline_session_context *ctx)
{
	immutable_cp_t cp = addrof(ctx->tc->search_var[9]);

	if ((ctx->tc->matches = get_matches_common(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_version(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Version = true;
}

static void
init_mode_for_voice(volatile struct readline_session_context *ctx)
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
	auto_complete_voice(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Voice = true;
}

static void
init_mode_for_whois(volatile struct readline_session_context *ctx)
{
	immutable_cp_t cp = addrof(ctx->tc->search_var[7]);

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
	ctx->tc->isInCirculationModeFor.Whois = true;
}

static void
init_mode_for_znc_cmds(volatile struct readline_session_context *ctx)
{
	immutable_cp_t cp = addrof(ctx->tc->search_var[5]);

	if ((ctx->tc->matches = get_list_of_matching_znc_commands(cp)) ==
	    NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_znc_cmd(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.ZncCmds = true;
}

static void
init_mode_for_commands(volatile struct readline_session_context *ctx,
    const bool numins_greater_than_one)
{
	immutable_cp_t cp = addrof(ctx->tc->search_var[1]);

	if (!numins_greater_than_one || (ctx->tc->matches =
	    get_list_of_matching_commands(cp)) == NULL) {
		output_error("no magic");
		return;
	}

	ctx->tc->elmt = textBuf_head(ctx->tc->matches);
	auto_complete_command(ctx, ctx->tc->elmt->text);
	ctx->tc->isInCirculationModeFor.Cmds = true;
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
	ctx->tc->isInCirculationModeFor.ChanUsers = true;
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
	if (strncmp(sv, "/chanserv ", 10) == 0)
		*iout = 10;
	else if (strncmp(sv, "/cs ", 4) == 0)
		*iout = 4;
	else
		*iout = 0;
	return (strncmp(sv, "/chanserv ", 10) == 0 ||
		strncmp(sv, "/cs ", 4) == 0);
}

static bool
var_matches_ns(CSTRING sv, int *iout)
{
	if (strncmp(sv, "/nickserv ", 10) == 0)
		*iout = 10;
	else if (strncmp(sv, "/ns ", 4) == 0)
		*iout = 4;
	else
		*iout = 0;
	return (strncmp(sv, "/nickserv ", 10) == 0 ||
		strncmp(sv, "/ns ", 4) == 0);
}

static void
init_mode(volatile struct readline_session_context *ctx)
{
	immutable_cp_t	sv = get_search_var(ctx);
	int		off1, off2;

	if (!strncmp(sv, "/connect ", 9))
		init_mode_for_connect(ctx);
	else if (var_matches_cs(sv, &off1))
		init_mode_for_cs(ctx, off1);
	else if (!strncmp(sv, "/dcc ", 5))
		init_mode_for_dcc(ctx);
	else if (!strncmp(sv, "/deop ", 6))
		init_mode_for_deop(ctx);
	else if (!strncmp(sv, "/devoice ", 9))
		init_mode_for_devoice(ctx);
	else if (!strncmp(sv, "/ftp ", 5))
		init_mode_for_ftp(ctx);
	else if (!strncmp(sv, "/help ", 6))
		init_mode_for_help(ctx);
	else if (!strncmp(sv, "/kick ", 6))
		init_mode_for_kick(ctx);
	else if (!strncmp(sv, "/kickban ", 9))
		init_mode_for_kickban(ctx);
	else if (!strncmp(sv, "/log ", 5))
		init_mode_for_log(ctx);
	else if (!strncmp(sv, "/mode ", 6))
		init_mode_for_mode(ctx);
	else if (!strncmp(sv, "/msg ", 5))
		init_mode_for_msg(ctx);
	else if (!strncmp(sv, "/notice ", 8))
		init_mode_for_notice(ctx);
	else if (var_matches_ns(sv, &off2))
		init_mode_for_ns(ctx, off2);
	else if (!strncmp(sv, "/op ", 4))
		init_mode_for_op(ctx);
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
	else if (!strncmp(sv, "/voice ", 7))
		init_mode_for_voice(ctx);
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
	} else if (ctx->tc->isInCirculationModeFor.Connect) {
		ac_doit(auto_complete_connect, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Cs) {
		ac_doit(auto_complete_cs, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Dcc) {
		ac_doit(auto_complete_dcc, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Deop) {
		ac_doit(auto_complete_deop, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Devoice) {
		ac_doit(auto_complete_devoice, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Ftp) {
		ac_doit(auto_complete_ftp, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Help) {
		ac_doit(auto_complete_help, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Kick) {
		ac_doit(auto_complete_kick, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Kickban) {
		ac_doit(auto_complete_kickban, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Log) {
		ac_doit(auto_complete_log, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Mode) {
		ac_doit(auto_complete_mode, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Msg) {
		ac_doit(auto_complete_msg, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Notice) {
		ac_doit(auto_complete_notice, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Ns) {
		ac_doit(auto_complete_ns, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Op) {
		ac_doit(auto_complete_op, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Query) {
		ac_doit(auto_complete_query, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Sasl) {
		ac_doit(auto_complete_sasl, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Settings) {
		ac_doit(auto_complete_setting, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Squery) {
		ac_doit(auto_complete_squery, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Theme) {
		ac_doit(auto_complete_theme, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Time) {
		ac_doit(auto_complete_time, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Version) {
		ac_doit(auto_complete_version, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Voice) {
		ac_doit(auto_complete_voice, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Whois) {
		ac_doit(auto_complete_whois, ctx);
	} else if (ctx->tc->isInCirculationModeFor.ZncCmds) {
		ac_doit(auto_complete_znc_cmd, ctx);
	} else if (ctx->tc->isInCirculationModeFor.Cmds) {
		ac_doit(auto_complete_command, ctx);
	} else if (ctx->tc->isInCirculationModeFor.ChanUsers) {
		ac_doit(auto_complete_channel_user, ctx);
	} else if (store_search_var(ctx) == -1) {
		output_error("cannot store search variable");
		return;
	} else
		init_mode(ctx);
}
