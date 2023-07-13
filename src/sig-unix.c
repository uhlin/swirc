/* Copyright (C) 2012-2023 Markus Uhlin. All rights reserved. */

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
	const char	*num_str;
	bool		 ignore;
	const char	*msg;
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
print_sig_message(const int signum)
{
	for (struct sig_message_tag *ssp = &sig_message[0];
	    ssp < &sig_message[ARRAY_SIZE(sig_message)];
	    ssp++) {
		if (ssp->num == signum) {
#if USE_STRSIGNAL
			err_msg("[-] FATAL: Received signal %d (%s)\n    %s",
			    ssp->num, ssp->num_str, (strsignal(ssp->num) ?
			    strsignal(ssp->num) : ssp->msg));
#else
			err_msg("[-] FATAL: Received signal %d (%s)\n    %s",
			    ssp->num, ssp->num_str, ssp->msg);
#endif
			return;
		}
	}
}

static void
signal_handler(int signum)
{
	if (signum == SIGWINCH) {
		struct timespec ts;

		ts.tv_sec	= 0;
		ts.tv_nsec	= 250000000;

		if (nanosleep(&ts, NULL) == 0 && !atomic_load_bool
		    (&g_connection_in_progress))
			(void) unget_wch(MY_KEY_RESIZE);
	} else {
		clean_up();
		print_sig_message(signum);
		_Exit(EXIT_FAILURE);
	}
}

void
block_signals(void)
{
	sigset_t set;

	(void) sigemptyset(&set);

	for (struct sig_message_tag *ssp = &sig_message[0];
	    ssp < &sig_message[ARRAY_SIZE(sig_message)];
	    ssp++) {
		if (ssp->ignore)
			(void) sigaddset(&set, ssp->num);
	}
	if ((errno = pthread_sigmask(SIG_BLOCK, &set, NULL)) != 0)
		err_sys("%s: pthread_sigmask", __func__);
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
		err_ret("%s: SIG_SETMASK", __func__);
		return false;
	}

	for (struct sig_message_tag *ssp = &sig_message[0];
	    ssp < &sig_message[ARRAY_SIZE(sig_message)];
	    ssp++) {
		if (ssp->ignore)
			act.sa_handler = SIG_IGN;
		else
			act.sa_handler = signal_handler;
		if (sigaction(ssp->num, &act, NULL) != 0) {
			err_ret("%s: sigaction failed on signal %d (%s)",
			    __func__, ssp->num, ssp->num_str);
			return false;
		}
	}

	(void) sigemptyset(&set);

	if (pthread_sigmask(SIG_SETMASK, &set, NULL) != 0) {
		err_ret("%s: SIG_SETMASK", __func__);
		return false;
	}

	return true;
}
