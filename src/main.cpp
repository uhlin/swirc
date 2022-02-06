/* main.cpp  --  starts execution
   Copyright (C) 2012-2022 Markus Uhlin. All rights reserved.

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

#if UNIX
#include <unistd.h> /* geteuid() pledge() */
#endif

#if WIN32
#include <process.h> /* _getpid() */
#endif

#include "assertAPI.h"
#include "curses-funcs.h"
#include "cursesInit.h"
#include "errHand.h"
#include "i18n.h"
#include "io-loop.h"
#include "irc.h"
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

const char g_swircVersion[] = "v3.3.3";
const char g_swircYear[]    = "2012-2022";
const char g_swircAuthor[]  = "Markus Uhlin";
const char g_swircWebAddr[] = "https://www.nifty-networks.net/swirc/";

char     *g_progname = const_cast<char *>("");
long int  g_pid = -1;

std::vector<std::string> g_join_list;

bool g_auto_connect         = false;
bool g_bind_hostname        = false;
bool g_change_color_defs    = true;
bool g_connection_password  = false;
bool g_debug_logging        = false;
bool g_explicit_config_file = false;
bool g_icb_mode             = false;
bool g_sasl_authentication  = true;
bool g_ssl_verify_peer      = true;

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
    N_("\nOptions:\n\n"),
    N_("    -4                   Use IPv4 addresses only\n"),
    N_("    -6                   Use IPv6 addresses only\n"),
    N_("    -?, --help           Output help\n"),
    N_("    -C                   Do not change color definitions\n"),
    N_("    -P                   Permanently disable SASL authentication\n"),
    N_("    -R                   Disable TLS/SSL peer verification\n"),
#if OUTPUT_INTERNAL_OPTIONS
    "    -T                   Internal option. Windows: Invoked when\n",
    "                         launched by a toast.\n",
#endif
    N_("    -c <server[:port]>   Connect to IRC server\n"),
    N_("    -d                   Debug logging\n"),
    N_("    -i                   Turn on Internet Citizen's Band mode\n"),
    N_("    -j <join>            A comma-separated list of channels to join\n"),
    N_("    -n <nickname>        Online nickname\n"),
    N_("    -p                   Query for server password (for private servers)\n"),
    N_("    -r <rl name>         Your real name\n"),
    N_("    -u <username>        Your username\n"),
    N_("    -v, --version        Output Swirc version\n"),
    N_("    -x <config>          Config file\n"),
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
	    for (const char **ppcc = &SoftwareDisclaimer[0];
		 ppcc < &SoftwareDisclaimer[ARRAY_SIZE(SoftwareDisclaimer)];
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
    char *MessageUsage = strdup_printf(_("Usage: %s [OPTION] ...\n"), exe);
    const char **ppcc;
    const size_t ar_sz = ARRAY_SIZE(OptionDesc);

    PUTS(MessageUsage);
    free(MessageUsage);

    for (ppcc = &OptionDesc[0]; ppcc < &OptionDesc[ar_sz]; ppcc++) {
	PUTS(_(*ppcc));
    }
}

/**
 * Command line option -T. The option is automatically invoked when
 * launched by a toast.
 */
static void
case_launched_by_toast_hook()
{
    debug("case_launched_by_toast_hook() called");
}

/**
 * -c <server[:port]>
 */
static void
case_connect()
{
    static bool	 been_case = false;
    char	*token1, *token2;
    char	*savp	   = const_cast<char *>("");

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

static void
case_join()
{
    char *cp;
    char *last = const_cast<char *>("");
    static bool been_case = false;

    if (been_case)
	DUP_OPTION_ERR('j');

    for (cp = g_option_arg;; cp = NULL) {
	char *token;

	if ((token = strtok_r(cp, ",", &last)) == NULL)
	    break;
	else if (strpbrk(token, g_forbidden_chan_name_chars) != NULL)
	    err_quit("forbidden chan name chars");
	else
	    g_join_list.push_back(token);
    }

    been_case = true;
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
 * Process options
 */
static void
process_options(int argc, char *argv[], const char *optstring)
{
    int opt = EOF;

    while ((opt = options(argc, argv, optstring)) != EOF) {
	switch (opt) {
	case '4':
	    net_set_sock_addr_family_ipv4();
	    break;
	case '6':
	    net_set_sock_addr_family_ipv6();
	    break;
	case 'C':
	    g_change_color_defs = false;
	    break;
	case 'P':
	    g_sasl_authentication = false;
	    break;
	case 'R':
	    g_ssl_verify_peer = false;
	    break;
	case 'T':
	    case_launched_by_toast_hook();
	    break;
	case 'c':
	    case_connect();
	    break;
	case 'd':
	    g_debug_logging = true;
	    break;
	case 'h':
	    case_hostname();
	    break;
	case 'i':
	    case_icb();
	    break;
	case 'j':
	    case_join();
	    break;
	case 'n':
	    case_nickname();
	    break;
	case 'p':
	    case_password();
	    break;
	case 'r':
	    case_rl_name();
	    break;
	case 'u':
	    case_username();
	    break;
	case 'x':
	    case_config();
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

static unsigned int
get_seed()
{
#if defined(UNIX)
	struct timeval tv = { 0 };

	errno = 0;

	if (gettimeofday(&tv, NULL) != 0)
		err_dump("%s", "get_seed: gettimeofday");

	return (getpid() ^ tv.tv_sec ^ tv.tv_usec);
#elif defined(WIN32)
	SYSTEMTIME st = { 0 };

	GetLocalTime(&st);

	return (_getpid() ^ st.wSecond ^ st.wMilliseconds);
#endif
}

#if defined(WIN32) && defined(TOAST_NOTIFICATIONS)
static void
toast_notifications_init()
{
	const wchar_t aumid[] = L"SwircDevelopmentTeam.Swirc";
	const GUID clsid = __uuidof(NotificationActivator);

	/*
	 * Initializes the COM library for use by the calling thread
	 */
	(void) CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	/*
	 * Register AUMID and COM server (for Desktop Bridge apps, this no-ops)
	 */
	if (DesktopNotificationManagerCompat::RegisterAumidAndComServer(aumid,
	    clsid) != S_OK)
		err_log(0, "Failed to register AUMID and COM server");

	/*
	 * Register COM activator
	 */
	if (DesktopNotificationManagerCompat::RegisterActivator() != S_OK)
		err_log(0, "Failed to register COM activator");

	Toasts::SendTestNotification();
}
#endif

/**
 * Starts execution
 */
int
main(int argc, char *argv[])
{
	char *cp;

#if __OpenBSD__
	extern char *malloc_options;

	malloc_options = const_cast<char *>("S");
#endif

#if defined(UNIX)
#define SLASH_CHAR '/'
#elif defined(WIN32)
#define SLASH_CHAR '\\'
#endif

	if ((cp = strrchr(argv[0], SLASH_CHAR)) == NULL)
		g_progname = argv[0];
	else
		g_progname = cp + 1;

#if defined(UNIX)
	sw_static_assert(sizeof(pid_t) <= sizeof(long int),
	    "pid type unexpectedly large");

	g_pid = getpid();
#elif defined(WIN32)
	g_pid = _getpid();
#endif

	(void) setlocale(LC_ALL, "");

#ifdef HAVE_LIBINTL_H
#if defined(UNIX)
	if (bindtextdomain("swirc", "/usr/local/share/locale") == NULL) {
		err_ret("bindtextdomain");
		return EXIT_FAILURE;
	}
#elif defined(WIN32)
	if (bindtextdomain("swirc", getenv("CD")) == NULL) {
		err_ret("bindtextdomain");
		return EXIT_FAILURE;
	}
#endif
	if (textdomain("swirc") == NULL) {
		err_ret("textdomain");
		return EXIT_FAILURE;
	}
#endif

	if (!sighand_init()) {
		err_msg("fatal: failed to initialize signal handling");
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
		}
	}

#if UNIX
	if (geteuid() == 0) {
		err_msg("fatal: running the program with root privileges is "
		    "forbidden");
		return EXIT_FAILURE;
	}
#endif

	process_options(argc, argv, "46CPRTc:dh:ij:n:pr:u:x:");

	srand(get_seed());

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
	toast_notifications_init();
#endif

#if defined(OpenBSD) && OpenBSD >= 201811
	chararray_t cert_pem_path = "/etc/ssl/cert.pem";

	if (unveil(g_home_dir, "rwc") == -1 ||
	    unveil(cert_pem_path, "r") == -1) {
		err_ret("unveil");
		return EXIT_FAILURE;
	}
#endif

#if defined(OpenBSD) && OpenBSD >= 201605
	if (pledge("cpath rpath wpath dns getpw inet stdio tty", NULL) == -1) {
		err_ret("pledge");
		return EXIT_FAILURE;
	}
#endif

	enter_io_loop();

	/*
	 * Reverse order...
	 */
	net_ssl_deinit();
	readline_deinit();
	windowSystem_deinit();
	statusbar_deinit();
	titlebar_deinit();
	escape_curses();
	nestHome_deinit();
	term_deinit();

	cmdline_options_destroy();
	puts("- Exit Success! -");
	return (EXIT_SUCCESS);
}

/**
 * Get locale info
 */
struct locale_info *
get_locale_info(int category)
{
	char *last = const_cast<char *>("");
	char *tok;
	char buf[200] = { '\0' };
	struct locale_info *li = new locale_info();

	if (sw_strcpy(buf, setlocale(category, NULL), ARRAY_SIZE(buf)) != 0)
		return (li);
	if ((tok = strtok_r(buf, ".", &last)) != NULL)
		li->lang_and_territory = sw_strdup(tok);
	if ((tok = strtok_r(NULL, ".", &last)) != NULL)
		li->codeset = sw_strdup(tok);
	return (li);
}

/**
 * Free locale info
 */
void
free_locale_info(struct locale_info *li)
{
	if (li) {
		free(li->lang_and_territory);
		free(li->codeset);
		delete li;
	}
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
