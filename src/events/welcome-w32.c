/* Copyright (C) 2014-2018 Markus Uhlin. All rights reserved. */

#include "common.h"

#include <windows.h>

#include "../config.h"
#include "../errHand.h"
#include "../libUtils.h"

#include "welcome-w32.h"

static HANDLE welcome_cond;

bool
event_welcome_is_signaled(void)
{
    struct integer_unparse_context unparse_ctx = {
	.setting_name     = "connection_timeout",
	.lo_limit         = 0,
	.hi_limit         = 300, /* 5 min */
	.fallback_default = 45,
    };

    return WaitForSingleObject(welcome_cond,
	size_product(config_integer_unparse(&unparse_ctx), 1000)) ==
	WAIT_OBJECT_0;
}

void
event_welcome_cond_init(void)
{
    if ((welcome_cond = CreateEvent(NULL, true, false, NULL)) == NULL)
	err_quit("CreateEvent error 0x%lx", (unsigned long int) GetLastError());
}

void
event_welcome_cond_destroy(void)
{
    if (!CloseHandle(welcome_cond))
	err_quit("CloseHandle error 0x%lx", (unsigned long int) GetLastError());
}

void
event_welcome_signalit(void)
{
    if (!SetEvent(welcome_cond))
	err_quit("SetEvent error 0x%lx", (unsigned long int) GetLastError());
}
