/* Copyright (C) 2012-2021 Markus Uhlin. All rights reserved. */

#include "common.h"

#include <signal.h>

#include "errHand.h"
#include "sig.h"
#include "terminal.h"

static void
clean_up(void)
{
    term_restore_title();
}

static void
signal_handler(int signum)
{
    struct sigmsg_tag {
	int	 num;
	char	*num_str;
	char	*msg;
    } sigmsg[] = {
	{ SIGABRT, "SIGABRT", "Abnormal termination"   },
	{ SIGFPE,  "SIGFPE",  "Floating-point error"   },
	{ SIGILL,  "SIGILL",  "Illegal instruction"    },
	{ SIGSEGV, "SIGSEGV", "Illegal storage access" },
	{ SIGTERM, "SIGTERM", "Termination request"    },
    };

    clean_up();

    for (struct sigmsg_tag *ssp = &sigmsg[0]; ssp < &sigmsg[ARRAY_SIZE(sigmsg)];
	 ssp++) {
	if (ssp->num == signum) {
	    err_msg("[-] FATAL: Received signal %d (%s)\n    %s",
		    ssp->num, ssp->num_str, ssp->msg);
	    break;
	}
    }

    _exit(EXIT_FAILURE);
}

void
block_signals(void)
{
    debug("block_signals() called");
}

bool
sighand_init(void)
{
    if (signal(SIGABRT, signal_handler) == SIG_ERR) {
	err_ret("SIGABRT error");
	return (false);
    }
    if (signal(SIGFPE, signal_handler) == SIG_ERR) {
	err_ret("SIGFPE error");
	return (false);
    }
    if (signal(SIGILL, signal_handler) == SIG_ERR) {
	err_ret("SIGILL error");
	return (false);
    }
    if (signal(SIGINT, SIG_IGN) == SIG_ERR) { /* CTRL+C signal */
	err_ret("SIGINT error");
	return (false);
    }
    if (signal(SIGSEGV, signal_handler) == SIG_ERR) {
	err_ret("SIGSEGV error");
	return (false);
    }
    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
	err_ret("SIGTERM error");
	return (false);
    }

    return (true);
}
