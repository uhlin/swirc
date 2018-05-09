/* Copyright (C) 2012-2016 Markus Uhlin. All rights reserved. */

#include "common.h"

#include <signal.h>
#include <time.h>

#include "errHand.h"
#include "network.h"		/* g_connection_in_progress */
#include "readline.h"		/* MY_KEY_RESIZE */
#include "sig.h"

#define USE_STRSIGNAL 0

static struct sig_message_tag {
    int		 num;
    char	*num_str;
    bool	 ignore;
    char	*msg;
} sig_message[] = {
    { SIGABRT,  "SIGABRT",  false, "Abnormal termination"           },
    { SIGBUS,   "SIGBUS",   false, "Bus error (bad memory access)"  },
    { SIGFPE,   "SIGFPE",   false, "Floating point exception"       },
    { SIGILL,   "SIGILL",   false, "Illegal Instruction"            },
    { SIGSEGV,  "SIGSEGV",  false, "Invalid memory reference"       },
    { SIGSYS,   "SIGSYS",   false, "Bad argument to routine"        },
    { SIGTERM,  "SIGTERM",  false, "Termination signal"             },
    { SIGWINCH, "SIGWINCH", false, "Window resize signal"           },
    { SIGXCPU,  "SIGXCPU",  false, "CPU time limit exceeded"        },
    { SIGXFSZ,  "SIGXFSZ",  false, "File size limit exceeded"       },
    { SIGHUP,   "SIGHUP",   true,  "Terminal line hangup"           },
    { SIGINT,   "SIGINT",   true,  "Interrupt program"              },
    { SIGPIPE,  "SIGPIPE",  true,  "Write on a pipe with no reader" },
    { SIGQUIT,  "SIGQUIT",  true,  "Quit program"                   },
};

static void
clean_up()
{
}

static void
signal_handler(int signum)
{
    struct timespec ts = {
	.tv_sec  = 0,
	.tv_nsec = 250000000,
    };
    struct sig_message_tag	*ssp;
    const size_t		 ar_sz = ARRAY_SIZE(sig_message);

    switch (signum) {
    case SIGWINCH:
	if (nanosleep(&ts, NULL) == 0 && !g_connection_in_progress)
	    (void) unget_wch(MY_KEY_RESIZE);
	return;
    default:
	clean_up();

	for (ssp = &sig_message[0]; ssp < &sig_message[ar_sz]; ssp++) {
	    if (ssp->num == signum) {
#if USE_STRSIGNAL
		err_msg("[-] FATAL: Received signal %d (%s)\n    %s",
		    ssp->num,
		    ssp->num_str,
		    strsignal(ssp->num) ? strsignal(ssp->num) : ssp->msg);
#else
		err_msg("[-] FATAL: Received signal %d (%s)\n    %s",
			ssp->num, ssp->num_str, ssp->msg);
#endif

		break;
	    }
	}

	break;
    }

    _Exit(EXIT_FAILURE);
}

bool
sigHand_init(void)
{
    sigset_t			 set;
    struct sigaction		 act;
    struct sig_message_tag	*ssp;
    const size_t		 ar_sz = ARRAY_SIZE(sig_message);

    (void) sigfillset(&set);
    if (sigprocmask(SIG_SETMASK, &set, NULL) != 0) {
	err_ret("SIG_SETMASK error");
	return (false);
    }

    (void) sigfillset(&act.sa_mask);
    for (act.sa_flags = 0, ssp = &sig_message[0]; ssp < &sig_message[ar_sz];
	 ssp++) {
	if (ssp->ignore) {
	    act.sa_handler = SIG_IGN;
	} else {
	    act.sa_handler = signal_handler;
	}

	if (sigaction(ssp->num, &act, NULL) != 0) {
	    err_ret("sigaction failed on signal %d (%s)",
		    ssp->num, ssp->num_str);
	    return (false);
	}
    }

    (void) sigemptyset(&set);
    if (sigprocmask(SIG_SETMASK, &set, NULL) != 0) {
	err_ret("SIG_SETMASK error");
	return (false);
    }

    return (true); /* All ok! */
}
