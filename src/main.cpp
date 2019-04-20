/* main.cpp  --  starts execution
   Copyright (C) 2012-2019 Markus Uhlin. All rights reserved.

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

#if __OpenBSD__
#include <sys/param.h>
#endif

#include <locale.h>

#ifndef RESTRICT_SYSOPS
#define RESTRICT_SYSOPS 1
#endif

#if UNIX
#include <unistd.h> /* geteuid() pledge() */
#endif

#include "assertAPI.h"
#include "curses-funcs.h"
#include "cursesInit.h"
#include "errHand.h"
#include "io-loop.h"
#include "libUtils.h"
#include "main.h"
#include "nestHome.h"
#include "network.h"
#include "options.h"
#include "readline.h"
#include "sig.h"
#include "statusbar.h"
#include "strHand.h"
#include "strdup_printf.h"
#include "terminal.h"
#include "titlebar.h"
#include "window.h"

#if defined(WIN32) && defined(TOAST_NOTIFICATIONS)
#include "DesktopNotificationManagerCompat.hpp"
#include "ToastsAPI.hpp"
#endif

#define DUP_OPTION_ERR(opt)	err_quit("A duplicate of option -%c found", opt)
#define PUTCHAR(c)		((void) fputc(c, stdout))
#define PUTS(string)		((void) fputs(string, stdout))

/* Things with external linkage
   ============================ */

const char g_swircVersion[] = "v2.7.0";
const char g_swircYear[]    = "2012-2019";
const char g_swircAuthor[]  = "Markus Uhlin";
const char g_swircWebAddr[] = "https://www.nifty-networks.net/swirc/";

bool g_auto_connect         = false;
bool g_icb_mode             = false;
bool g_connection_password  = false;
bool g_bind_hostname        = false;
bool g_explicit_config_file = false;

struct cmdline_opt_values *g_cmdline_opts = new cmdline_opt_values();

/* Things with internal linkage
   ============================ */

static const char *SoftwareDisclaimer[] = {
    "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n",
    "\"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n",
    "LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n",
    "A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT\n",
    "HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,\n",
    "INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,\n",
    "BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS\n",
    "OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND\n",
    "ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR\n",
    "TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE\n",
    "USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH\n",
    "DAMAGE.\n",
};

static const char *OptionDesc[] = {
    "\nCommon\n",
    "    -c <server[:port]>    Auto-connect to given server\n",
    "    -n <nickname>         Online nickname\n",
    "    -u <username>         User identity\n",
    "    -r <rl name>          Specifies the real name\n",
#ifdef NOT_YET
    "    -i                    ICB mode\n",
#endif
    "    -p                    Server password (for private servers)\n",
    "    -h <hostname>         Use this hostname on connect\n",
    "    -x <config>           Explicit config file\n",
    "\nExtras\n",
    "    -v, -version          View the current client ver\n",
    "    -?, -help             Print the usage\n",
#ifdef OUTPUT_INTERNAL_OPTIONS
    "    -T                    Internal option. Windows: Invoked when\n",
    "                          launched by a toast.\n",
#endif
};

/**
 * View Swirc version
 */
static void
view_version()
{
    char *MessageVersion = strdup_printf("Swirc %s\nCopyright (C) %s %s\n",
	g_swircVersion, g_swircYear, g_swircAuthor);

    PUTS(MessageVersion);
    free(MessageVersion);

    while (true) {
	char answer[100];
	int c;

	PUTS("Output disclaimer? [y/N]: ");
	(void) fflush(stdout);

	BZERO(answer, sizeof answer);

	if (fgets(answer, sizeof answer - 1, stdin) == NULL) {
	    PUTCHAR('\n');
	    continue;
	} else if (strchr(answer, '\n') == NULL) {
	    /* input too big */
	    while (c = getchar(), c != '\n' && c != EOF)
		/* discard */;
	} else if (strings_match(trim(answer), "y") ||
		   strings_match(answer, "Y")) {
	    const char **ppcc = NULL;
	    const size_t ar_sz = ARRAY_SIZE(SoftwareDisclaimer);

	    for (ppcc = &SoftwareDisclaimer[0];
		 ppcc < &SoftwareDisclaimer[ar_sz];
		 ppcc++) {
		PUTS(*ppcc);
	    }

	    break;
	} else if (strings_match(answer, "") ||
		   strings_match(answer, "n") || strings_match(answer, "N")) {
	    break;
	} else {
	    continue;
	}
    }
}

/**
 * Print help
 */
static void
print_help(const char *exe)
{
    char *MessageUsage = strdup_printf("Usage: %s [OPTION] ...\n", exe);
    const char **ppcc;
    const size_t ar_sz = ARRAY_SIZE(OptionDesc);

    PUTS(MessageUsage);
    free(MessageUsage);

    for (ppcc = &OptionDesc[0]; ppcc < &OptionDesc[ar_sz]; ppcc++) {
	PUTS(*ppcc);
    }
}

/**
 * -c <server[:port]>
 */
static void
case_connect()
{
    static bool	 been_case = false;
    char	*token1, *token2;
    char	*savp	   = (char *) "";

    if (been_case) {
	DUP_OPTION_ERR('c');
    }

    token1 = strtok_r(g_option_arg, ":", &savp); /* can't be NULL */
    g_cmdline_opts->server = sw_strdup(token1);

    if ((token2 = strtok_r(NULL, ":", &savp)) == NULL) {
	const char irc_port_default[] = "6667";

	g_cmdline_opts->port = sw_strdup(irc_port_default);
    } else if (token2 != NULL) {
	g_cmdline_opts->port = sw_strdup(token2);
    } else {
	sw_assert_not_reached();
    }

    g_auto_connect = been_case = true;
}

/**
 * -n <nickname>
 */
static void
case_nickname()
{
    static bool been_case = false;

    if (been_case) {
	DUP_OPTION_ERR('n');
    }

    g_cmdline_opts->nickname = sw_strdup(g_option_arg);
    been_case = true;
}

/**
 * -u <username>
 */
static void
case_username()
{
    static bool been_case = false;

    if (been_case) {
	DUP_OPTION_ERR('u');
    }

    g_cmdline_opts->username = sw_strdup(g_option_arg);
    been_case = true;
}

/**
 * -r <rl name>
 */
static void
case_rl_name()
{
    static bool been_case = false;

    if (been_case) {
	DUP_OPTION_ERR('r');
    }

    g_cmdline_opts->rl_name = sw_strdup(g_option_arg);
    been_case = true;
}

/**
 * Option -i
 */
static void
case_icb()
{
    static bool been_case = false;

    if (been_case) {
	DUP_OPTION_ERR('i');
    }

    g_icb_mode = been_case = true;
}

/**
 * Option -p
 */
static void
case_password()
{
    static bool been_case = false;

    if (been_case) {
	DUP_OPTION_ERR('p');
    }

    g_connection_password = been_case = true;
}

/**
 * -h <hostname>
 */
static void
case_hostname()
{
    static bool been_case = false;

    if (been_case) {
	DUP_OPTION_ERR('h');
    }

    g_cmdline_opts->hostname = sw_strdup(g_option_arg);
    g_bind_hostname = been_case = true;
}

/**
 * -x <config>
 */
static void
case_config()
{
    static bool been_case = false;

    if (been_case) {
	DUP_OPTION_ERR('x');
    }

    g_cmdline_opts->config_file = sw_strdup(g_option_arg);
    g_explicit_config_file = been_case = true;
}

/**
 * Command line option -T. The option is automatically invoked when
 * launched by a toast.
 */
static void
case_launched_by_toast_hook()
{
}

/**
 * Process options
 */
static void
process_options(int argc, char *argv[], const char *optstring)
{
    int opt = EOF;

    while ((opt = options(argc, argv, optstring)) != EOF) {
	switch (opt) {
	case 'c':
	    case_connect();
	    break;
	case 'n':
	    case_nickname();
	    break;
	case 'u':
	    case_username();
	    break;
	case 'r':
	    case_rl_name();
	    break;
	case 'i':
	    case_icb();
	    break;
	case 'p':
	    case_password();
	    break;
	case 'h':
	    case_hostname();
	    break;
	case 'x':
	    case_config();
	    break;
	case 'T':
	    case_launched_by_toast_hook();
	    break;
	case UNRECOGNIZED_OPTION:
	    err_msg("%s: -%c: unrecognized option", argv[0], g_option_save);
	    print_help(argv[0]);
	    exit(EXIT_FAILURE);
	case OPTION_ARG_MISSING:
	    err_msg("%s: -%c: option argument missing", argv[0], g_option_save);
	    /*FALLTHROUGH*/
	default:
	    print_help(argv[0]);
	    exit(EXIT_FAILURE);
	}
    }
}

/**
 * Starts execution
 */
int
main(int argc, char *argv[])
{
#if __OpenBSD__
    extern char *malloc_options;

    malloc_options = (char *) "S";
#endif

    (void) setlocale(LC_ALL, "");

    if (!sigHand_init()) {
	err_msg("FATAL: Failed to initialize signal handling");
	return EXIT_FAILURE;
    }

    if (argc == 2) {
	if (strncmp(argv[1], "-v", 3) == 0 ||
	    strncmp(argv[1], "-version", 9) == 0 ||
	    strncmp(argv[1], "--version", 10) == 0) {
	    view_version();
	    return EXIT_SUCCESS;
	} else if (strncmp(argv[1], "-?", 3) == 0 ||
		   strncmp(argv[1], "-help", 6) == 0 ||
		   strncmp(argv[1], "--help", 7) == 0) {
	    print_help(argv[0]);
	    return EXIT_SUCCESS;
	} else {
	    /* empty */;
	}
    }

#if UNIX
    if (geteuid() == 0) {
	err_msg("fatal: "
	    "running the program with root privileges is prohibited");
	return EXIT_FAILURE;
    }
#endif

    process_options(argc, argv, "c:n:u:r:iph:x:T");
    srand(time(NULL));

    term_init();
    nestHome_init();

    if (curses_init() != OK) {
	err_msg("Initialization of the Curses library not possible");
	return EXIT_FAILURE;
    }

    titlebar_init();
    statusbar_init();
    windowSystem_init();
    readline_init();
    net_ssl_init();

#if defined(WIN32) && defined(TOAST_NOTIFICATIONS)
    /*
     * Initializes the COM library for use by the calling thread
     */
    (void) CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    /*
     * Register AUMID and COM server (for Desktop Bridge apps, this no-ops)
     */
    if (DesktopNotificationManagerCompat::RegisterAumidAndComServer(
	    L"SwircDevelopmentTeam.Swirc",
	    __uuidof(NotificationActivator)) != S_OK)
	err_log(0, "Failed to register AUMID and COM server");

    /*
     * Register COM activator
     */
    if (DesktopNotificationManagerCompat::RegisterActivator() != S_OK)
	err_log(0, "Failed to register COM activator");

    Toasts::SendTestNotification();
#endif

#if defined(OpenBSD) && OpenBSD >= 201811
    if (unveil(g_home_dir, "rwc") == -1) {
	err_ret("fatal: unveil(%s, ...)", g_home_dir);
	return EXIT_FAILURE;
    }

    /**
     * Read access to cert.pem is needed by libcurl
     */
    if (unveil("/etc/ssl/cert.pem", "r") == -1) {
	err_ret("fatal: unveil(%s, ...)", "/etc/ssl/cert.pem");
	return EXIT_FAILURE;
    }
#endif

#if defined(OpenBSD) && OpenBSD >= 201605 && RESTRICT_SYSOPS
    if (pledge("cpath dns getpw inet rpath stdio tty wpath", NULL) == -1) {
	err_ret("pledge");
	return EXIT_FAILURE;
    }
#endif

    enter_io_loop();

    /* XXX: Reverse order... */
    net_ssl_deinit();
    readline_deinit();
    windowSystem_deinit();
    statusbar_deinit();
    titlebar_deinit();
    escape_curses();
    nestHome_deinit();
    term_deinit();

    cmdline_options_destroy();
    say("- Exit Success! -\n");
    return EXIT_SUCCESS;
}

/**
 * Get locale info
 */
struct locale_info *
get_locale_info(int category)
{
    struct locale_info *li = (struct locale_info *) xcalloc(sizeof *li, 1);
    char  buf[200] = { 0 };
    char *tok      = NULL;
    char *savp     = (char *) "";

    li->lang_and_territory = li->codeset = NULL;

    if (sw_strcpy(buf, setlocale(category, NULL), sizeof buf) != 0)
	return (li);

    if ((tok = strtok_r(buf, ".", &savp)) != NULL)
	li->lang_and_territory = sw_strdup(tok);
    if ((tok = strtok_r(NULL, ".", &savp)) != NULL)
	li->codeset = sw_strdup(tok);

    return (li);
}

/**
 * Free locale info
 */
void
free_locale_info(struct locale_info *li)
{
    free_not_null(li->lang_and_territory);
    free_not_null(li->codeset);

    free(li);
}

/**
 * Command-line options destroy
 */
void
cmdline_options_destroy(void)
{
    free_and_null(&g_cmdline_opts->server);
    free_and_null(&g_cmdline_opts->port);
    free_and_null(&g_cmdline_opts->nickname);
    free_and_null(&g_cmdline_opts->username);
    free_and_null(&g_cmdline_opts->rl_name);
    free_and_null(&g_cmdline_opts->hostname);
    free_and_null(&g_cmdline_opts->config_file);

    delete g_cmdline_opts;
}
