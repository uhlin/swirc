/* Copyright (C) 2014-2024 Markus Uhlin. All rights reserved. */

#include "common.h"

#include <sys/time.h>

#include "../assertAPI.h"
#include "../config.h"
#include "../errHand.h"
#include "../pthrMutex.h"

#include "welcome-unix.h"

static bool is_signaled = false;

static pthread_mutex_t	foo_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t	welcome_cond;

bool
event_welcome_is_signaled(void)
{
	int			ret;
	struct integer_context	intctx = { 0 };
	struct timespec		ts = { 0 };
	struct timeval		tv = { 0 };

	intctx.setting_name = "connection_timeout";
	intctx.lo_limit = 0;
	intctx.hi_limit = 300; /* 5 min */
	intctx.fallback_default = 45;

	if (gettimeofday(&tv, NULL) != 0)
		err_sys("%s: gettimeofday", __func__);

	ts.tv_sec = tv.tv_sec + config_integer(&intctx);
	ts.tv_nsec = tv.tv_usec;

	mutex_lock(&foo_mutex);
	while (!is_signaled) {
		ret = pthread_cond_timedwait(&welcome_cond, &foo_mutex, &ts);

		if (ret == 0) {
			is_signaled = true;
		} else if (ret == EINVAL) {
			err_exit(EINVAL, "%s: pthread_cond_timedwait",
			    __func__);
		} else if (ret == ETIMEDOUT) {
			is_signaled = false;
			break;
		} else {
			sw_assert_not_reached();
		}
	}
	mutex_unlock(&foo_mutex);

	return is_signaled;
}

void
event_welcome_cond_init(void)
{
	if ((errno = pthread_cond_init(&welcome_cond, NULL)) != 0)
		err_sys("%s: pthread_cond_init", __func__);
	is_signaled = false;
}

void
event_welcome_cond_destroy(void)
{
	if ((errno = pthread_cond_destroy(&welcome_cond)) != 0)
		err_sys("%s: pthread_cond_destroy", __func__);
}

/*
 * Wake up ANY thread that's currently blocked on the condition variable
 */
void
event_welcome_signalit(void)
{
	if ((errno = pthread_cond_broadcast(&welcome_cond)) != 0)
		err_sys("%s: pthread_cond_broadcast", __func__);
}
