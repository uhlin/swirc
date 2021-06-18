/* Copyright (C) 2014-2021 Markus Uhlin. All rights reserved. */

#include "common.h"

#include <windows.h>

#include "../config.h"
#include "../errHand.h"
#include "../libUtils.h"

#include "welcome-w32.h"

static HANDLE welcome_cond;

/* -------------------------------------------------- */

/**
 * DWORD is a 32-bit unsigned integer
 *
 * Return 'elt_count' elements of size 'elt_size' (elt_count * elt_size) -- but
 * check for overflow
 *
 * Leave the function non-static in case we want to use it somewhere else
 */
DWORD
dword_product(const DWORD elt_count, const DWORD elt_size)
{
	if (elt_size && elt_count > MAXDWORD / elt_size) {
		err_msg("Integer overflow");
		abort();
	}

	return (elt_count * elt_size);
}

bool
event_welcome_is_signaled(void)
{
	struct integer_context intctx = {
		.setting_name = "connection_timeout",
		.lo_limit = 0,
		.hi_limit = 300, /* 5 min */
		.fallback_default = 45,
	};

	return (WaitForSingleObject(welcome_cond,
	    dword_product(config_integer(&intctx), 1000)) == WAIT_OBJECT_0);
}

void
event_welcome_cond_init(void)
{
	if ((welcome_cond = CreateEvent(NULL, true, false, NULL)) == NULL) {
		err_quit("event_welcome_cond_init: CreateEvent: %s",
		    errdesc_by_last_err());
	}
}

void
event_welcome_cond_destroy(void)
{
	if (!CloseHandle(welcome_cond)) {
		err_quit("event_welcome_cond_destroy: CloseHandle: %s",
		    errdesc_by_last_err());
	}
}

void
event_welcome_signalit(void)
{
	if (!SetEvent(welcome_cond)) {
		err_quit("event_welcome_signalit: SetEvent: %s",
		    errdesc_by_last_err());
	}
}
