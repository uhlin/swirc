/* Input output loop
   Copyright (C) 2014-2021 Markus Uhlin. All rights reserved.

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

#include <sys/stat.h>

#include "assertAPI.h"
#include "config.h"
#include "cursesInit.h"
#include "dataClassify.h"
#include "errHand.h"
#include "icb.h"
#include "io-loop.h"
#include "irc.h"
#include "libUtils.h"
#include "main.h"
#include "nestHome.h"
#include "network.h"
#include "printtext.h"
#include "readline.h"
#include "strHand.h"
#include "strdup_printf.h"
#include "terminal.h"
#include "theme.h"

#include "commands/ban.h"
#include "commands/cleartoasts.h"
#include "commands/colormap.h"
#include "commands/connect.h"
#include "commands/echo.h"
#include "commands/ignore.h"
#include "commands/invite.h"
#include "commands/jp.h"
#include "commands/kick.h"
#include "commands/me.h"
#include "commands/misc.h"
#include "commands/msg.h"
#include "commands/nick.h"
#include "commands/notice.h"
#include "commands/op.h"
#include "commands/sasl.h"
#include "commands/say.h"
#include "commands/services.h"
#include "commands/servlist.h"
#include "commands/squery.h"
#include "commands/theme.h"
#include "commands/topic.h"
#include "commands/znc.h"

#include "events/names.h"

wchar_t g_push_back_buf[MAX_PBB] = { 0L };
bool g_io_loop = true;

static PTEXTBUF		history = NULL;
static PTEXTBUF_ELMT	element = NULL;

static size_t bytes_convert = 0;
static const size_t ARSZ = ARRAY_SIZE(g_push_back_buf);

#include "commandhelp.h"

static struct cmds_tag {
    char *cmd;
    CMD_HANDLER_FN fn;
    bool requires_connection;
    const char **usage;
    const size_t size;
    bool irc_only;
} cmds[] = {
    { "away",        cmd_away,        true,  away_usage,        ARRAY_SIZE(away_usage),        true  },
    { "ban",         cmd_ban,         true,  ban_usage,         ARRAY_SIZE(ban_usage),         true  },
    { "banlist",     cmd_banlist,     true,  banlist_usage,     ARRAY_SIZE(banlist_usage),     true  },
    { "beep",        cmd_beep,        true,  beep_usage,        ARRAY_SIZE(beep_usage),        false },
    { "boot",        cmd_boot,        true,  boot_usage,        ARRAY_SIZE(boot_usage),        false },
    { "chanserv",    cmd_chanserv,    true,  chanserv_usage,    ARRAY_SIZE(chanserv_usage),    true  },
    { "cleartoasts", cmd_cleartoasts, false, cleartoasts_usage, ARRAY_SIZE(cleartoasts_usage), false },
    { "close",       cmd_close,       false, close_usage,       ARRAY_SIZE(close_usage),       false },
    { "colormap",    cmd_colormap,    false, colormap_usage,    ARRAY_SIZE(colormap_usage),    false },
    { "connect",     cmd_connect,     false, connect_usage,     ARRAY_SIZE(connect_usage),     false },
    { "cs",          cmd_chanserv,    true,  chanserv_usage,    ARRAY_SIZE(chanserv_usage),    true  },
    { "cycle",       cmd_cycle,       true,  cycle_usage,       ARRAY_SIZE(cycle_usage),       true  },
    { "deop",        cmd_deop,        true,  deop_usage,        ARRAY_SIZE(deop_usage),        true  },
    { "disconnect",  cmd_disconnect,  false, disconnect_usage,  ARRAY_SIZE(disconnect_usage),  false },
    { "echo",        cmd_echo,        false, echo_usage,        ARRAY_SIZE(echo_usage),        false },
    { "exlist",      cmd_exlist,      true,  exlist_usage,      ARRAY_SIZE(exlist_usage),      true  },
    { "group",       cmd_group,       true,  group_usage,       ARRAY_SIZE(group_usage),       false },
    { "help",        cmd_help,        false, help_usage,        ARRAY_SIZE(help_usage),        false },
#if NOT_YET
    { "ignore",      cmd_ignore,      true,  ignore_usage,      ARRAY_SIZE(ignore_usage),      false },
#endif
    { "ilist",       cmd_ilist,       true,  ilist_usage,       ARRAY_SIZE(ilist_usage),       true  },
    { "invite",      cmd_invite,      true,  invite_usage,      ARRAY_SIZE(invite_usage),      true  },
    { "join",        cmd_join,        true,  join_usage,        ARRAY_SIZE(join_usage),        true  },
    { "kick",        cmd_kick,        true,  kick_usage,        ARRAY_SIZE(kick_usage),        true  },
    { "kickban",     cmd_kickban,     true,  kickban_usage,     ARRAY_SIZE(kickban_usage),     true  },
    { "kill",        cmd_kill,        true,  kill_usage,        ARRAY_SIZE(kill_usage),        true  },
    { "list",        cmd_list,        true,  list_usage,        ARRAY_SIZE(list_usage),        false },
    { "me",          cmd_me,          true,  me_usage,          ARRAY_SIZE(me_usage),          true  },
    { "mode",        cmd_mode,        true,  mode_usage,        ARRAY_SIZE(mode_usage),        true  },
    { "msg",         cmd_msg,         true,  msg_usage,         ARRAY_SIZE(msg_usage),         true  },
    { "nick",        cmd_nick,        true,  nick_usage,        ARRAY_SIZE(nick_usage),        false },
    { "nickserv",    cmd_nickserv,    true,  nickserv_usage,    ARRAY_SIZE(nickserv_usage),    true  },
    { "notice",      cmd_notice,      true,  notice_usage,      ARRAY_SIZE(notice_usage),      true  },
    { "ns",          cmd_nickserv,    true,  nickserv_usage,    ARRAY_SIZE(nickserv_usage),    true  },
    { "op",          cmd_op,          true,  op_usage,          ARRAY_SIZE(op_usage),          true  },
    { "oper",        cmd_oper,        true,  oper_usage,        ARRAY_SIZE(oper_usage),        true  },
    { "part",        cmd_part,        true,  part_usage,        ARRAY_SIZE(part_usage),        true  },
    { "passmod",     cmd_passmod,     true,  passmod_usage,     ARRAY_SIZE(passmod_usage),     false },
    { "query",       cmd_query,       false, query_usage,       ARRAY_SIZE(query_usage),       false },
    { "quit",        cmd_quit,        false, quit_usage,        ARRAY_SIZE(quit_usage),        false },
    { "resize",      cmd_resize,      false, resize_usage,      ARRAY_SIZE(resize_usage),      false },
    { "rules",       cmd_rules,       true,  rules_usage,       ARRAY_SIZE(rules_usage),       true  },
    { "sasl",        cmd_sasl,        false, sasl_usage,        ARRAY_SIZE(sasl_usage),        true  },
    { "say",         cmd_say,         true,  say_usage,         ARRAY_SIZE(say_usage),         false },
    { "servlist",    cmd_servlist,    true,  servlist_usage,    ARRAY_SIZE(servlist_usage),    true  },
    { "set",         cmd_set,         false, set_usage,         ARRAY_SIZE(set_usage),         false },
    { "squery",      cmd_squery,      true,  squery_usage,      ARRAY_SIZE(squery_usage),      true  },
    { "theme",       cmd_theme,       false, theme_usage,       ARRAY_SIZE(theme_usage),       false },
    { "time",        cmd_time,        true,  time_usage,        ARRAY_SIZE(time_usage),        true  },
    { "topic",       cmd_topic,       true,  topic_usage,       ARRAY_SIZE(topic_usage),       false },
    { "unban",       cmd_unban,       true,  unban_usage,       ARRAY_SIZE(unban_usage),       true  },
    { "version",     cmd_version,     true,  version_usage,     ARRAY_SIZE(version_usage),     true  },
    { "who",         cmd_who,         true,  who_usage,         ARRAY_SIZE(who_usage),         true  },
    { "whois",       cmd_whois,       true,  whois_usage,       ARRAY_SIZE(whois_usage),       true  },
    { "znc",         cmd_znc,         true,  znc_usage,         ARRAY_SIZE(znc_usage),         true  },
};

#define FOREACH_COMMAND() \
	for (struct cmds_tag *sp = &cmds[0]; sp < &cmds[ARRAY_SIZE(cmds)]; sp++)

static void
add_to_history(const char *string)
{
    struct integer_unparse_context unparse_ctx = {
	.setting_name     = "cmd_hist_size",
	.fallback_default = 50,
	.lo_limit         = 0,
	.hi_limit         = 300,
    };
    const int tbszp1 = textBuf_size(history) + 1;

    if (config_integer_unparse(&unparse_ctx) == 0 ||
	!strncasecmp(string, "/nickserv -- identify", 21) ||
	!strncasecmp(string, "/ns -- identify", 15))
	return;

    if (tbszp1 > config_integer_unparse(&unparse_ctx)) {
	/* Buffer full. Remove head... */

	if ((errno = textBuf_remove(history, textBuf_head(history))) != 0)
	    err_sys("add_to_history: textBuf_remove");
    }

    if (textBuf_size(history) == 0) {
	if ((errno = textBuf_ins_next(history, NULL, string, -1)) != 0)
	    err_sys("add_to_history: textBuf_ins_next");
    } else {
	if ((errno = textBuf_ins_next(history, textBuf_tail(history), string, -1)) != 0)
	    err_sys("add_to_history: textBuf_ins_next");
    }
}

static void
bold_fix(char *string)
{
    char *cp = NULL;

    if (!string)
	return;
    while ((cp = strchr(string, BOLD)) != NULL)
	*cp = BOLD_ALIAS;
}

#if WIN32
#define stat _stat
#endif

static bool
get_error_log_size(double *size)
{
    char path[1300] = "";
    struct stat sb = { 0 };

    if (!g_log_dir || sw_strcpy(path, g_log_dir, sizeof path) != 0) {
	*size = 0;
	return false;
    }

#if defined(UNIX)
    if (sw_strcat(path, "/error.log", sizeof path) != 0)
#elif defined(WIN32)
    if (sw_strcat(path, "\\error.log", sizeof path) != 0)
#endif
	{
	    *size = 0;
	    return false;
	}

    if (stat(path, &sb) == -1) {
	*size = 0;
	return false;
    }

    *size = (double) (sb.st_size / 1000);
    return true;
}

static bool
got_hits(const char *search_var)
{
    FOREACH_COMMAND() {
	if (!strncmp(search_var, sp->cmd, strlen(search_var)))
	    return true;
    }

    return false;
}

static void
handle_cmds(const char *data)
{
    PRINTTEXT_CONTEXT ctx;
    char *cp = NULL;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_FAILURE, true);

    FOREACH_COMMAND() {
	cp = strdup_printf("%s ", sp->cmd);

	if (strings_match(data, sp->cmd)) {
	    if (sp->requires_connection && !g_on_air)
		printtext(&ctx, "command requires irc connection");
	    else if (sp->irc_only && g_icb_mode)
		printtext(&ctx, "command is irc only");
	    else
		sp->fn("");
	    free(cp);
	    return;
	} else if (!strncmp(data, cp, strlen(cp))) {
	    if (sp->requires_connection && !g_on_air)
		printtext(&ctx, "command requires irc connection");
	    else if (sp->irc_only && g_icb_mode)
		printtext(&ctx, "command is irc only");
	    else
		sp->fn(&data[strlen(cp)]);
	    free(cp);
	    return;
	} else {
	    free(cp);
	}
    }

    printtext(&ctx, "unknown command");
}

static void
history_next(void)
{
    if (textBuf_size(history) == 0)
	return;

    if (element != textBuf_tail(history)) {
	element = element->next;
	bold_fix(element->text);
	bytes_convert = xmbstowcs(g_push_back_buf, element->text, ARSZ - 1);
	if (bytes_convert == g_conversion_failed)
	    wmemset(g_push_back_buf, 0L, ARSZ);
	else if (bytes_convert == ARSZ - 1)
	    g_push_back_buf[ARSZ - 1] = 0L;
    }
}

static void
history_prev(void)
{
    if (textBuf_size(history) == 0)
	return;

    bold_fix(element->text);
    bytes_convert = xmbstowcs(g_push_back_buf, element->text, ARSZ - 1);
    if (bytes_convert == g_conversion_failed)
	wmemset(g_push_back_buf, 0L, ARSZ);
    else if (bytes_convert == ARSZ - 1)
	g_push_back_buf[ARSZ - 1] = 0L;

    if (element != textBuf_head(history))
	element = element->prev;
}

static void
list_all_commands(void)
{
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC_NONE, true);
    printtext(&ctx, "--------------- Commands ---------------");

    FOREACH_COMMAND() {
	const char *cmd1 = sp->cmd;
	char *cmd2, *cmd3;

	if ((sp + 1) < &cmds[ARRAY_SIZE(cmds)] &&
	    (sp + 2) < &cmds[ARRAY_SIZE(cmds)]) {
	    sp++, cmd2 = sp->cmd;
	    sp++, cmd3 = sp->cmd;
	} else if ((sp + 1) < &cmds[ARRAY_SIZE(cmds)]) {
	    sp++, cmd2 = sp->cmd;
	    cmd3 = NULL;
	} else {
	    cmd2 = cmd3 = NULL;
	}

	if (cmd1 && cmd2 && cmd3)
	    printtext(&ctx, "%-15s %-15s %s", cmd1, cmd2, cmd3);
	else if (cmd1 && cmd2)
	    printtext(&ctx, "%-15s %s", cmd1, cmd2);
	else if (cmd1)
	    printtext(&ctx, "%s", cmd1);
	else
	    sw_assert_not_reached();
    }
}

static void
output_help_for_command(const char *command)
{
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_active_window, TYPE_SPEC2, true);

    FOREACH_COMMAND() {
	if (strings_match(command, sp->cmd)) {
	    const char **lines = & (sp->usage[0]);
	    const size_t size = sp->size;

	    while (lines < & (sp->usage[size])) {
		printtext(&ctx, "%s", *lines);
		lines++;
	    }

	    return;
	}
    }

    ctx.spec_type = TYPE_SPEC1_FAILURE;
    printtext(&ctx, "no such command");
}

static void
swirc_greeting(void)
{
#define USE_LARRY3D_LOGO 1
    PRINTTEXT_CONTEXT ctx;
    const char **ppcc = NULL;
    const char *logo[] = {
#if USE_LARRY3D_LOGO
	"                     __                              ",
	"    ____  __  __  __/\\_\\  _ __   ___               ",
	"   /',__\\/\\ \\/\\ \\/\\ \\/\\ \\/\\`'__\\/'___\\   ",
	"  /\\__, `\\ \\ \\_/ \\_/ \\ \\ \\ \\ \\//\\ \\__/   ",
	"  \\/\\____/\\ \\___x___/'\\ \\_\\ \\_\\\\ \\____\\  ",
	"   \\/___/  \\/__//__/   \\/_/\\/_/ \\/____/         ",
#else
	" _______          _________ _______  _______     ",
	"(  ____ \\|\\     /|\\__   __/(  ____ )(  ____ \\",
	"| (    \\/| )   ( |   ) (   | (    )|| (    \\/  ",
	"| (_____ | | _ | |   | |   | (____)|| |          ",
	"(_____  )| |( )| |   | |   |     __)| |          ",
	"      ) || || || |   | |   | (\\ (   | |         ",
	"/\\____) || () () |___) (___| ) \\ \\__| (____/\\",
	"\\_______)(_______)\\_______/|/   \\__/(_______/ ",
#endif
    };
    const size_t logo_size = ARRAY_SIZE(logo);
    double log_size_kb;

    printtext_context_init(&ctx, g_status_window, TYPE_SPEC1, true);

    for (ppcc = &logo[0]; ppcc < &logo[logo_size]; ppcc++) {
	const char *color = Theme("logo_color");
	char *str = sw_strdup(*ppcc);

	printtext(&ctx, "%s%s", color, trim(str));
	free(str);
    }

    printtext(&ctx, " ");

    printtext(&ctx, "    Swirc %s by %s", g_swircVersion, g_swircAuthor);
    printtext(&ctx, "    Compiled on %s%s %s%s",
	      LEFT_BRKT, __DATE__, __TIME__, RIGHT_BRKT);

    if (g_initialized_pairs < 0) {
	g_initialized_pairs = 0;
    }

    printtext(&ctx, " ");
    printtext(&ctx, "Program settings are stored in %s%s%s",
	      LEFT_BRKT, g_home_dir, RIGHT_BRKT);
    printtext(&ctx, "%c%hd%c color pairs have been initialized",
	      BOLD, g_initialized_pairs, BOLD);
    printtext(&ctx, "Type /help for a list of commands; or /help <command>");
    printtext(&ctx, "for help of a specific command");
    if (get_error_log_size(&log_size_kb))
	printtext(&ctx, "Error log size %s%.1f KB%s",
	    LEFT_BRKT, log_size_kb, RIGHT_BRKT);
    printtext(&ctx, " ");
}

PTEXTBUF
get_list_of_matching_commands(const char *search_var)
{
    if (!got_hits(search_var))
	return NULL;

    PTEXTBUF matches = textBuf_new();

    FOREACH_COMMAND() {
	if (!strncmp(search_var, sp->cmd, strlen(search_var))) {
	    if (textBuf_size(matches) == 0) {
		if ((errno = textBuf_ins_next(matches, NULL, sp->cmd, -1)) != 0)
		    err_sys("get_list_of_matching_commands: textBuf_ins_next");
	    } else {
		if ((errno = textBuf_ins_next(matches, textBuf_tail(matches), sp->cmd, -1)) != 0)
		    err_sys("get_list_of_matching_commands: textBuf_ins_next");
	    }
	}
    }

    return matches;
}

/* must be freed */
char *
get_prompt(void)
{
    const char AFK[] = "(away)";
    const size_t minimum_cols = sizeof "#abc...: ";

    if (strings_match_ignore_case(ACTWINLABEL, g_status_window_label) ||
	COLS < size_to_int(minimum_cols))
	return sw_strdup("");

    const size_t prompt_maxlen = (size_t) (COLS / 2);

    if (prompt_maxlen < minimum_cols)
	return sw_strdup("");

    char *prompt;
    int ret;

    prompt = xmalloc(prompt_maxlen);
    ret = snprintf(prompt, prompt_maxlen, "%s%s%c ",
	ACTWINLABEL,
	(g_is_away ? AFK : ""),
	(is_irc_channel(ACTWINLABEL) ? ':' : '>'));
    if (ret < 0 || ((size_t) ret) >= prompt_maxlen) {
	free(prompt);
	return sw_strdup("");
    }

    for (const char *cp = prompt; *cp != '\0'; cp++) {
	if (!sw_isprint(*cp)) {
	    free(prompt);
	    return sw_strdup("");
	}
    }

    return prompt;
}

/* usage: /help [command] */
void
cmd_help(const char *data)
{
    const bool has_command = !strings_match(data, "");

    if (has_command)
	output_help_for_command(data);
    else
	list_all_commands();
}

void
enter_io_loop(void)
{
    new_window_title(g_status_window_label, g_swircWebAddr);

    if (config_bool("startup_greeting", true)) {
	swirc_greeting();
    }

    if (g_auto_connect) {
	char buf[1000];

	snprintf(buf, ARRAY_SIZE(buf), "%s:%s", g_cmdline_opts->server,
	    g_cmdline_opts->port);
	/*
	 * napms: let i/o finish
	 */
	napms(444);
	cmd_connect(buf);
    }

    history = textBuf_new();

    do {
	char *prompt, *line;
	const char cmd_char = '/';

	prompt = get_prompt();
	line   = readline(prompt);
	free(prompt);
	wmemset(g_push_back_buf, 0L, ARSZ);

	if (line == NULL) {
	    if (g_resize_requested) {
		term_resize_all();
	    } else if (g_hist_next) {
		history_next();
	    } else if (g_hist_prev) {
		history_prev();
	    }

	    continue;
	} else if (*line == cmd_char) {
	    handle_cmds(&line[1]);
	} else {
	    if (g_on_air &&
		!strings_match(g_active_window->label, g_status_window_label))
		transmit_user_input(g_active_window->label, line);
	}

	add_to_history(line);
	element = textBuf_tail(history);

	free(line);
    } while (g_io_loop);

    textBuf_destroy(history);
}

#define S1 Theme("nick_s1")
#define S2 Theme("nick_s2")

void
transmit_user_input(const char *win_label, const char *input)
{
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, window_by_label(win_label), TYPE_SPEC_NONE,
	true);

    if (ctx.window == NULL) {
	err_log(0, "transmit_user_input: window %s not found", win_label);
	return;
    }

    if (g_icb_mode) {
	if (is_irc_channel(win_label))
	    icb_send_open_msg(input);
	else
	    icb_send_pm(win_label, input);
    } else {
	if (net_send("PRIVMSG %s :%s", win_label, input) < 0) {
	    g_connection_lost = true;
	    return;
	}
    }

    if (!is_irc_channel(win_label))
	printtext(&ctx, "%s%s%s%c%s %s",
	    S1, COLOR1, g_my_nickname, NORMAL, S2, input);
    else {
	PNAMES	n = NULL;
	char	c = ' ';

	if ((n = event_names_htbl_lookup(g_my_nickname, win_label)) == NULL) {
	    err_log(0, "transmit_user_input: hash table lookup error");
	    return;
	}

	if (n->is_owner)        c = '~';
	else if (n->is_superop) c = '&';
	else if (n->is_op)      c = '@';
	else if (n->is_halfop)  c = '%';
	else if (n->is_voice)   c = '+';
	else c = ' ';

	printtext(&ctx, "%s%c%s%s%c%s %s",
	    S1, c, COLOR1, g_my_nickname, NORMAL, S2, input);
    }
}
