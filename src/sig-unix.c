/* Copyright (C) 2012-2021 Markus Uhlin. All rights reserved. */

#include "common.h"

#include <signal.h>
#include <time.h>

#include "errHand.h"
#include "network.h"		/* g_connection_in_progress */
#include "readline.h"		/* MY_KEY_RESIZE */
#include "sig.h"
#include "terminal.h"

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
clean_up(void)
{
    term_restore_title();
}

static void
signal_handler(int signum)
{
    struct timespec ts = {
	.tv_sec = 0,
	.tv_nsec = 250000000,
    };

    switch (signum) {
    case SIGWINCH:
	if (nanosleep(&ts, NULL) == 0 &&
	    !atomic_load_bool(&g_connection_in_progress))
	    (void) unget_wch(MY_KEY_RESIZE);
	return;
    default:
	clean_up();

	for (struct sig_message_tag *ssp = &sig_message[0];
	     ssp < &sig_message[ARRAY_SIZE(sig_message)]; ssp++) {
	    if (ssp->num == signum) {
#if USE_STRSIGNAL
		err_msg("[-] FATAL: Received signal %d (%s)\n    %s", ssp->num,
		    ssp->num_str,
		    (strsignal(ssp->num) ? strsignal(ssp->num) : ssp->msg));
#else
		err_msg("[-] FATAL: Received signal %d (%s)\n    %s", ssp->num,
		    ssp->num_str, ssp->msg);
#endif
		break;
	    }
	}

	break;
    }

    _Exit(EXIT_FAILURE);
}

void
block_signals(void)
{
    sigset_t set;

    (void) sigemptyset(&set);

    for (struct sig_message_tag *ssp = &sig_message[0];
	 ssp < &sig_message[ARRAY_SIZE(sig_message)]; ssp++) {
	if (ssp->ignore)
	    (void) sigaddset(&set, ssp->num);
    }

    if ((errno = pthread_sigmask(SIG_BLOCK, &set, NULL)) != 0)
	err_sys("block_signals: pthread_sigmask");
}

bool
sighand_init(void)
{
    sigset_t set;
    struct sigaction act = { 0 };

    (void) sigfillset(&set);
    (void) sigfillset(&act.sa_mask);
    act.sa_flags = 0;

    if (pthread_sigmask(SIG_SETMASK, &set, NULL) != 0) {
	err_ret("sighand_init: SIG_SETMASK");
	return false;
    }

    for (struct sig_message_tag *ssp = &sig_message[0];
	 ssp < &sig_message[ARRAY_SIZE(sig_message)]; ssp++) {
	if (ssp->ignore) {
	    act.sa_handler = SIG_IGN;
	} else {
	    act.sa_handler = signal_handler;
	}
	if (sigaction(ssp->num, &act, NULL) != 0) {
	    err_ret("sighand_init: sigaction failed on signal %d (%s)",
		    ssp->num, ssp->num_str);
	    return false;
	}
    }

    (void) sigemptyset(&set);

    if (pthread_sigmask(SIG_SETMASK, &set, NULL) != 0) {
	err_ret("sighand_init: SIG_SETMASK");
	return false;
    }

    return true;
}
