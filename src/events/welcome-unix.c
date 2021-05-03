/* Copyright (C) 2014, 2016 Markus Uhlin. All rights reserved. */

#include "common.h"

#include <sys/time.h>

#include "../config.h"
#include "../errHand.h"
#include "../pthrMutex.h"

#include "welcome-unix.h"

static pthread_mutex_t foo_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t welcome_cond;

bool
event_welcome_is_signaled(void)
{
    bool is_signaled = false; /* initial state */
    struct integer_unparse_context unparse_ctx;
    struct timespec ts;
    struct timeval tv;

    unparse_ctx.setting_name     = "connection_timeout";
    unparse_ctx.lo_limit         = 0;
    unparse_ctx.hi_limit         = 300; /* 5 min */
    unparse_ctx.fallback_default = 45;

    if (gettimeofday(&tv, NULL) != 0) {
	err_sys("gettimeofday error");
    }

    ts.tv_sec  = tv.tv_sec + config_integer(&unparse_ctx);
    ts.tv_nsec = tv.tv_usec;

    mutex_lock(&foo_mutex);
    if (pthread_cond_timedwait(&welcome_cond, &foo_mutex, &ts) == 0)
	is_signaled = true;
    mutex_unlock(&foo_mutex);

    return (is_signaled);
}

void
event_welcome_cond_init(void)
{
    if ((errno = pthread_cond_init(&welcome_cond, NULL)) != 0)
	err_sys("pthread_cond_init error");
}

void
event_welcome_cond_destroy(void)
{
    if ((errno = pthread_cond_destroy(&welcome_cond)) != 0)
	err_sys("pthread_cond_destroy error");
}

/* Wake up ANY thread that's currently blocked on the condition variable */
void
event_welcome_signalit(void)
{
    if ((errno = pthread_cond_broadcast(&welcome_cond)) != 0)
	err_sys("pthread_cond_broadcast error");
}
