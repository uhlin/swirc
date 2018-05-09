/* Copyright (C) 2012-2016 Markus Uhlin. All rights reserved. */

#include "common.h"
#include "errHand.h"
#include "sig.h"

#include <signal.h>

static void
clean_up()
{
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
    struct sigmsg_tag	*ssp;
    const size_t	 ar_sz = ARRAY_SIZE(sigmsg);

    clean_up();

    for (ssp = &sigmsg[0]; ssp < &sigmsg[ar_sz]; ssp++) {
	if (ssp->num == signum) {
	    err_msg("[-] FATAL: Received signal %d (%s)\n    %s",
		    ssp->num, ssp->num_str, ssp->msg);
	    break;
	}
    }

    _exit(EXIT_FAILURE);
}

bool
sigHand_init(void)
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
