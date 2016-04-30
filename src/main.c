/* main.c  --  starts execution
   Copyright (C) 2012-2016 Markus Uhlin. All rights reserved.

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

#include <locale.h>

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

#define DUP_OPTION_ERR(opt)	err_quit("A duplicate of option -%c found", opt)
#define PUTCHAR(c)		((void) fputc(c, stdout))
#define PUTS(string)		((void) fputs(string, stdout))

/* Things with external linkage
   ============================ */

const char g_swircVersion[] = "v1.0a";
const char g_swircYear[]    = "2012-2016";
const char g_swircAuthor[]  = "Markus Uhlin";

bool g_auto_connect         = false;
bool g_connection_password  = false;
bool g_bind_hostname        = false;
bool g_explicit_config_file = false;

/* Things with internal linkage
   ============================ */

static const char *SoftwareDisclaimer[] = {
    "THIS SOFTWARE IS PROVIDED THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n",
    "AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n",
    "IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n",
    "ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n",
    "BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR\n",
    "CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF\n",
    "SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\n",
    "INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN\n",
    "CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)\n",
    "ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE\n",
    "POSSIBILITY OF SUCH DAMAGE.\n",
};

static const char *OptionDesc[] = {
    "\nCommon\n",
    "    -c <server[:port]>    Auto-connect to given server\n",
    "    -n <nickname>         Online nickname\n",
    "    -u <username>         User identity\n",
    "    -r <rl name>          Specifies the real name\n",
    "    -p <password>         Password (for private networks)\n",
    "    -h <hostname>         Use this hostname on connect\n",
    "    -x <config>           Explicit config file\n",
    "\nExtras\n",
    "    -v, -version          View the current client ver\n",
    "    -?, -help             Print the usage\n",
};

static struct cmdline_opt_values opt_values_data = {
    .server	 = NULL,
    .port	 = NULL,
    .nickname	 = NULL,
    .username	 = NULL,
    .rl_name	 = NULL,
    .password	 = NULL,
    .hostname	 = NULL,
    .config_file = NULL,
};

struct cmdline_opt_values *g_cmdline_opts = &opt_values_data; /* External */

/* Function declarations
   ===================== */

static void view_version  (void);
static void print_help    (const char *exe);
static void case_connect  (void);
static void case_nickname (void);
static void case_username (void);
static void case_rl_name  (void);
static void case_password (void);
static void case_hostname (void);
static void case_config   (void);

int
main(int argc, char *argv[])
{
#if __OpenBSD__
    extern char *malloc_options;
#endif
    int opt;
    const char optstring[] = "c:n:u:r:p:h:x:";

#if __OpenBSD__
    malloc_options = "S";
#endif
    (void) setlocale(LC_ALL, "");
    if (!sigHand_init()) {
	err_msg("FATAL: Failed to initialize signal handling");
	return EXIT_FAILURE;
    }

    if (argc == 2) {
	if (strncmp(argv[1], "-v", 3) == 0 || strncmp(argv[1], "-version", 9) == 0) {
	    view_version();
	    return EXIT_SUCCESS;
	} else if (strncmp(argv[1], "-?", 3) == 0 || strncmp(argv[1], "-help", 6) == 0) {
	    print_help(argv[0]);
	    return EXIT_SUCCESS;
	} else {
	    /* empty */;
	}
    }

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
	case 'p':
	    case_password();
	    break;
	case 'h':
	    case_hostname();
	    break;
	case 'x':
	    case_config();
	    break;
	case UNRECOGNIZED_OPTION:
	    err_msg("%s: -%c: unrecognized option", argv[0], g_option_save);
	    print_help(argv[0]);
	    return EXIT_FAILURE;
	case OPTION_ARG_MISSING:
	    err_msg("%s: -%c: option argument missing", argv[0], g_option_save);
            /*FALLTHROUGH*/
	default:
	    print_help(argv[0]);
	    return EXIT_FAILURE;
	}
    }

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

static void
view_version(void)
{
    char *MessageVersion = Strdup_printf("Swirc %s\nCopyright (C) %s %s\n",
					 g_swircVersion, g_swircYear, g_swircAuthor);

    PUTS(MessageVersion);
    free(MessageVersion);

    while (true) {
	char answer[100];

	PUTS("Do you also want to read the disclaimer? (yes or no): ");
	(void) fflush(stdout);

	BZERO(answer, sizeof answer);
	if (fgets(answer, sizeof answer - 1, stdin) == NULL) {
	    break;
	}

	if (Strings_match(trim(str_tolower(answer)), "yes")) {
	    const char		**ppcc;
	    const size_t	  ar_sz = ARRAY_SIZE(SoftwareDisclaimer);

	    PUTCHAR('\n');
	    for (ppcc = &SoftwareDisclaimer[0]; ppcc < &SoftwareDisclaimer[ar_sz]; ppcc++) {
		PUTS(*ppcc);
	    }

	    break;
	} else if (Strings_match(answer, "no")) {
	    break;
	} else if (answer[90]) {
	    PUTS("*** Aborting.\n");
	    break;
	} else {
	    ;
	}
    }
}

static void
print_help(const char *exe)
{
    char		 *MessageUsage = Strdup_printf("Usage: %s [OPTION] ...\n", exe);
    const char		**ppcc;
    const size_t	  ar_sz	       = ARRAY_SIZE(OptionDesc);

    PUTS(MessageUsage);
    free(MessageUsage);

    for (ppcc = &OptionDesc[0]; ppcc < &OptionDesc[ar_sz]; ppcc++) {
	PUTS(*ppcc);
    }
}

void
cmdline_options_destroy(void)
{
    free_and_null(&g_cmdline_opts->server);
    free_and_null(&g_cmdline_opts->port);
    free_and_null(&g_cmdline_opts->nickname);
    free_and_null(&g_cmdline_opts->username);
    free_and_null(&g_cmdline_opts->rl_name);
    free_and_null(&g_cmdline_opts->password);
    free_and_null(&g_cmdline_opts->hostname);
    free_and_null(&g_cmdline_opts->config_file);
}

/* -c <server[:port]> */
static void
case_connect(void)
{
    static bool	 been_case = false;
    char	*token1, *token2;
    char	*savp	   = "";

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

/* -n <nickname> */
static void
case_nickname(void)
{
    static bool been_case = false;

    if (been_case) {
	DUP_OPTION_ERR('n');
    }

    g_cmdline_opts->nickname = sw_strdup(g_option_arg);
    been_case = true;
}

/* -u <username> */
static void
case_username(void)
{
    static bool been_case = false;

    if (been_case) {
	DUP_OPTION_ERR('u');
    }

    g_cmdline_opts->username = sw_strdup(g_option_arg);
    been_case = true;
}

/* -r <rl name> */
static void
case_rl_name(void)
{
    static bool been_case = false;

    if (been_case) {
	DUP_OPTION_ERR('r');
    }

    g_cmdline_opts->rl_name = sw_strdup(g_option_arg);
    been_case = true;
}

/* -p <password> */
static void
case_password(void)
{
    static bool been_case = false;

    if (been_case) {
	DUP_OPTION_ERR('p');
    }

    g_cmdline_opts->password = sw_strdup(g_option_arg);
    g_connection_password = been_case = true;
}

/* -h <hostname> */
static void
case_hostname(void)
{
    static bool been_case = false;

    if (been_case) {
	DUP_OPTION_ERR('h');
    }

    g_cmdline_opts->hostname = sw_strdup(g_option_arg);
    g_bind_hostname = been_case = true;
}

/* -x <config> */
static void
case_config(void)
{
    static bool been_case = false;

    if (been_case) {
	DUP_OPTION_ERR('x');
    }

    g_cmdline_opts->config_file = sw_strdup(g_option_arg);
    g_explicit_config_file = been_case = true;
}
