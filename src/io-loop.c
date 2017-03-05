/* Input output loop
   Copyright (C) 2014-2017 Markus Uhlin. All rights reserved.

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

#include <sys/stat.h>

#include "assertAPI.h"
#include "config.h"
#include "cursesInit.h"
#include "dataClassify.h"
#include "errHand.h"
#include "io-loop.h"
#include "irc.h"
#include "main.h"
#include "nestHome.h"
#include "network.h"
#include "printtext.h"
#include "readline.h"
#include "strHand.h"
#include "strdup_printf.h"
#include "terminal.h"
#include "theme.h"

#include "commands/connect.h"
#include "commands/invite.h"
#include "commands/jp.h"
#include "commands/kick.h"
#include "commands/me.h"
#include "commands/misc.h"
#include "commands/msg.h"
#include "commands/nick.h"
#include "commands/notice.h"
#include "commands/say.h"
#include "commands/services.h"
#include "commands/topic.h"

#include "events/names.h"

bool g_io_loop = true;

static PTEXTBUF		history = NULL;
static PTEXTBUF_ELMT	element = NULL;

static struct cmds_tag {
    char		*cmd;
    CMD_HANDLER_FN	 fn;
    bool		 requires_connection;
    char		*usage;
} cmds[] = {
    { "away",       cmd_away,       true,  "/away [reason]" },
    { "banlist",    cmd_banlist,    true,  "/banlist [channel]" },
    { "chanserv",   cmd_chanserv,   true,  "/chanserv "
      "<service hostname | --> <command> [...]" },
    { "close",      cmd_close,      false, "/close" },
    { "connect",    cmd_connect,    false, "/connect [-ssl] <server[:port]>" },
    { "cycle",      cmd_cycle,      true,  "/cycle [channel]" },
    { "disconnect", cmd_disconnect, true,  "/disconnect [message]" },
    { "exlist",     cmd_exlist,     true,  "/exlist [channel]" },
    { "help",       cmd_help,       false, "/help [command]" },
    { "ilist",      cmd_ilist,      true,  "/ilist [channel]" },
    { "invite",     cmd_invite,     true,  "/invite <targ_nick> <channel>" },
    { "join",       cmd_join,       true,  "/join <channel> [key]" },
    { "kick",       cmd_kick,       true,  "/kick "
      "<nick1[,nick2][,nick3][...]> [reason]" },
    { "list",       cmd_list,       true,  "/list "
      "[<max_users[,>min_users][,pattern][...]]" },
    { "me",         cmd_me,         true,  "/me <message>" },
    { "mode",       cmd_mode,       true,  "/mode <modes> [...]" },
    { "msg",        cmd_msg,        true,  "/msg <recipient> <message>" },
    { "n",          cmd_names,      true,  "/n [channel]" },
    { "nick",       cmd_nick,       true,  "/nick <new nickname>" },
    { "nickserv",   cmd_nickserv,   true,  "/nickserv "
      "<service hostname | --> <command> [...]" },
    { "notice",     cmd_notice,     true,  "/notice <recipient> <message>" },
    { "part",       cmd_part,       true,  "/part [channel] [message]" },
    { "query",      cmd_query,      false, "/query [nick]" },
    { "quit",       cmd_quit,       false, "/quit [message]" },
    { "resize",     cmd_resize,     false, "/resize" },
    { "rules",      cmd_rules,      true,  "/rules" },
    { "say",        cmd_say,        true,  "/say <message>" },
    { "time",       cmd_time,       true,  "/time <target>" },
    { "topic",      cmd_topic,      true,  "/topic [new topic]" },
    { "version",    cmd_version,    true,  "/version <target>" },
    { "who",        cmd_who,        true,  "/who <mask>" },
    { "whois",      cmd_whois,      true,  "/whois <nick>" },
};

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

    *size = (double) sb.st_size / 1000;
    return true;
}

static void
swirc_greeting()
{
#define USE_LARRY3D_LOGO 1
    struct printtext_context ptext_ctx;
    const char **ppcc;
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

    ptext_ctx.window     = g_status_window;
    ptext_ctx.spec_type  = TYPE_SPEC1;
    ptext_ctx.include_ts = true;

    for (ppcc = &logo[0]; ppcc < &logo[logo_size]; ppcc++) {
	const char *color = Theme("sw_ascLogotype_color");
	char       *str   = sw_strdup(*ppcc);

	printtext(&ptext_ctx, "%s%s", color, trim(str));
	free(str);
    }

    printtext(&ptext_ctx, " ");

    printtext(&ptext_ctx, "    Swirc %s by %s", g_swircVersion, g_swircAuthor);
    printtext(&ptext_ctx, "    Compiled on %s%s %s%s",
	      LEFT_BRKT, __DATE__, __TIME__, RIGHT_BRKT);

    if (g_initialized_pairs < 0) {
	g_initialized_pairs = 0;
    }

    printtext(&ptext_ctx, " ");
    printtext(&ptext_ctx, "Program settings are stored in %s%s%s",
	      LEFT_BRKT, g_home_dir, RIGHT_BRKT);
    printtext(&ptext_ctx, "%c%hd%c color pairs have been initialized",
	      BOLD, g_initialized_pairs, BOLD);
    printtext(&ptext_ctx, "Type /help for a list of commands; or /help "
	"<command>\n(for a brief usage of that command)");
    if (get_error_log_size(&log_size_kb))
	printtext(&ptext_ctx, "Error log size %s%.1f KB%s",
		  LEFT_BRKT, log_size_kb, RIGHT_BRKT);
    printtext(&ptext_ctx, " ");
}

/* must be freed */
static char *
get_prompt()
{
    if (Strings_match_ignore_case(g_active_window->label,
				  g_status_window_label)) {
	return (sw_strdup(""));
    } else if (is_irc_channel(g_active_window->label)) {
	return (Strdup_printf("%s: ", g_active_window->label));
    } else {
	return (Strdup_printf("%s> ", g_active_window->label)); /* a query */
    }

    /*NOTREACHED*/
    sw_assert_not_reached();
    return (sw_strdup(""));
}

void
unget_string(char *string)
{
    char *cp = NULL;

    if (!string)
	return;
    while ((cp = strchr(string, BOLD)) != NULL)
	*cp = BOLD_ALIAS;
    for (cp = &string[strlen(string) - 1]; cp >= &string[0]; cp--)
	ungetch(*cp);
}

static void
history_next()
{
    if (textBuf_size(history) == 0)
	return;

    if (element != textBuf_tail(history)) {
	element = element->next;
	unget_string(element->text);
    }
}

static void
history_prev()
{
    if (textBuf_size(history) == 0)
	return;

    unget_string(element->text);

    if (element != textBuf_head(history))
	element = element->prev;
}

static void
handle_cmds(const char *data)
{
    struct cmds_tag *sp;
    const size_t ar_sz = ARRAY_SIZE(cmds);
    char *cp;
    struct printtext_context ctx = {
	.window     = g_active_window,
	.spec_type  = TYPE_SPEC1_FAILURE,
	.include_ts = true,
    };

    for (sp = &cmds[0]; sp < &cmds[ar_sz]; sp++) {
	cp = Strdup_printf("%s ", sp->cmd);

	if (Strings_match(data, sp->cmd)) {
	    if (sp->requires_connection && !g_on_air)
		printtext(&ctx, "command requires irc connection");
	    else
		sp->fn("");
	    free(cp);
	    break;
	} else if (!strncmp( data, cp, strlen(cp) )) {
	    if (sp->requires_connection && !g_on_air)
		printtext(&ctx, "command requires irc connection");
	    else
		sp->fn(&data[strlen(cp)]);
	    free(cp);
	    break;
	} else {
	    free(cp);
	}
    }
}

void
transmit_user_input(const char *win_label, const char *input)
{
    struct printtext_context ctx = {
	.window     = window_by_label(win_label),
	.spec_type  = TYPE_SPEC_NONE,
	.include_ts = true,
    };

    if (ctx.window == NULL) {
	err_log(0, "In transmit_user_input: window %s not found", win_label);
	return;
    }

    if (net_send("PRIVMSG %s :%s", win_label, input) < 0) {
	g_on_air = false;
	return;
    }

    if (!is_irc_channel(win_label))
	printtext(&ctx, "%s%s%s%c%s %s",
		  Theme("nick_s1"),
		  COLOR1, g_my_nickname, NORMAL,
		  Theme("nick_s2"),
		  input);
    else {
	PNAMES	n = NULL;
	char	c = ' ';

	if ((n = event_names_htbl_lookup(g_my_nickname, win_label)) == NULL) {
	    err_log(0, "In transmit_user_input: hash table lookup error");
	    return;
	}

	if (n->is_owner)        c = '~';
	else if (n->is_superop) c = '&';
	else if (n->is_op)      c = '@';
	else if (n->is_halfop)  c = '%';
	else if (n->is_voice)   c = '+';
	else c = ' ';

	printtext(&ctx, "%s%c%s%s%c%s %s",
		  Theme("nick_s1"),
		  c, COLOR1, g_my_nickname, NORMAL,
		  Theme("nick_s2"),
		  input);
    }
}

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
	!strncasecmp(string, "/nickserv -- identify", 21))
	return;

    if (tbszp1 > config_integer_unparse(&unparse_ctx)) {
	/* Buffer full. Remove head... */

	if ((errno = textBuf_remove( history, textBuf_head(history) )) != 0)
	    err_sys("textBuf_remove");
    }

    if (textBuf_size(history) == 0) {
	if ((errno = textBuf_ins_next(history, NULL, string, -1)) != 0)
	    err_sys("textBuf_ins_next");
    } else {
	if ((errno = textBuf_ins_next(history, textBuf_tail(history), string,
				      -1)) != 0)
	    err_sys("textBuf_ins_next");
    }
}

void
enter_io_loop(void)
{
    new_window_title(g_status_window_label, g_swircWebAddr);

    if (config_bool_unparse("startup_greeting", true)) {
	swirc_greeting();
    }

    if (g_auto_connect) {
	do_connect(g_cmdline_opts->server, g_cmdline_opts->port);
    }

    history = textBuf_new();

    do {
	char *prompt, *line;
	const char cmd_char = '/';

	prompt = get_prompt();
	line   = readline(prompt);
	free(prompt);

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
	    if (g_on_air && !Strings_match(g_active_window->label,
					   g_status_window_label)) {
		struct printtext_context ptext_ctx = {
		    .window     = g_active_window,
		    .spec_type  = TYPE_SPEC_NONE,
		    .include_ts = true,
		};

		if (config_bool_unparse("recode", true)) {
		    ptext_ctx.spec_type = TYPE_SPEC1_FAILURE;
		    printtext(&ptext_ctx, "Can't recode user input "
			"before transmit (yet unsupported)");
		} else { /* don't recode... */
		    transmit_user_input(g_active_window->label, line);
		}
	    }
	}

	add_to_history(line);
	element = textBuf_tail(history);

	free(line);
    } while (g_io_loop);

    textBuf_destroy(history);
}

static void
output_help_for_command(const char *command)
{
    struct cmds_tag *sp;
    const size_t ar_sz = ARRAY_SIZE(cmds);
    struct printtext_context ctx = {
	.window     = g_active_window,
	.spec_type  = TYPE_SPEC2,
	.include_ts = true,
    };

    for (sp = &cmds[0]; sp < &cmds[ar_sz]; sp++) {
	if (Strings_match(command, sp->cmd)) {
	    printtext(&ctx, "usage: %s", sp->usage);
	    return;
	}
    }

    ctx.spec_type = TYPE_SPEC1_FAILURE;
    printtext(&ctx, "no such command");
}

static void
list_all_commands()
{
    struct printtext_context ctx = {
	.window     = g_active_window,
	.spec_type  = TYPE_SPEC_NONE,
	.include_ts = true,
    };
    struct cmds_tag *sp;
    const size_t ar_sz = ARRAY_SIZE(cmds);

    printtext(&ctx, "--------------- Commands ---------------");

    for (sp = &cmds[0]; sp < &cmds[ar_sz]; sp++) {
	const char *cmd1 = sp->cmd;
	char *cmd2, *cmd3;

	if ((sp + 1) < &cmds[ar_sz] && (sp + 2) < &cmds[ar_sz]) {
	    sp++, cmd2 = sp->cmd;
	    sp++, cmd3 = sp->cmd;
	} else if ((sp + 1) < &cmds[ar_sz]) {
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

/* usage: /help [command] */
void
cmd_help(const char *data)
{
    const bool has_command = !Strings_match(data, "");

    if (has_command)
	output_help_for_command(data);
    else
	list_all_commands();
}
