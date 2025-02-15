/* Copyright (C) 2014-2025 Markus Uhlin. All rights reserved. */

#include "common.h"

#include "../config.h"
#include "../errHand.h"
#include "../libUtils.h"
#include "../network.h"

#include "atomicops.h"
#include "welcome-w32.h"

static HANDLE welcome_cond = NULL;

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
		err_msg("%s: integer overflow", __func__);
		abort();
	}

	return (elt_count * elt_size);
}

bool
event_welcome_is_signaled(void)
{
	struct integer_context intctx = {
		.setting_name		= "connection_timeout",
		.lo_limit		= 0,
		.hi_limit		= 300, /* 5 min */
		.fallback_default	= 45,
	};

	return (WaitForSingleObject(welcome_cond,
	    dword_product(config_integer(&intctx), 1000)) == WAIT_OBJECT_0);
}

void
event_welcome_cond_init(void)
{
	if ((welcome_cond = CreateEvent(NULL, true, false, NULL)) == NULL) {
		err_quit("%s: CreateEvent: %s", __func__,
		    errdesc_by_last_err());
	}
}

void
event_welcome_cond_destroy(void)
{
	if (!CloseHandle(welcome_cond)) {
		err_quit("%s: CloseHandle: %s", __func__,
		    errdesc_by_last_err());
	} else
		welcome_cond = NULL;
}

void
event_welcome_signalit(void)
{
	if (!atomic_load_bool(&g_connection_in_progress))
		return;
	if (welcome_cond != NULL && !SetEvent(welcome_cond))
		err_quit("%s: SetEvent: %s", __func__, errdesc_by_last_err());
}
