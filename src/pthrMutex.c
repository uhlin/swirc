/* Wrapper functions for POSIX threads dealing with mutexes
   Copyright (C) 2012-2025 Markus Uhlin. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

   - Neither the name of the author nor the names of its contributors may be
     used to endorse or promote products derived from this software without
     specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

#include "common.h"
#include "errHand.h"
#include "pthrMutex.h"

void
mutex_lock(pthread_mutex_t *mutex)
{
	if ((errno = pthread_mutex_lock(mutex)) != 0)
		err_sys("%s: pthread_mutex_lock", __func__);
}

void
mutex_unlock(pthread_mutex_t *mutex)
{
	if ((errno = pthread_mutex_unlock(mutex)) != 0)
		err_sys("%s: pthread_mutex_unlock", __func__);
}

void
mutex_destroy(pthread_mutex_t *mutex)
{
	if ((errno = pthread_mutex_destroy(mutex)) != 0)
		err_sys("%s: pthread_mutex_destroy", __func__);
}

void
mutex_new(pthread_mutex_t *mutex)
{
	pthread_mutexattr_t attr;

	if ((errno = pthread_mutexattr_init(&attr)) != 0)
		err_sys("%s: pthread_mutexattr_init", __func__);
	else if ((errno = pthread_mutexattr_settype(&attr,
		    PTHREAD_MUTEX_RECURSIVE)) != 0)
		err_sys("%s: pthread_mutexattr_settype", __func__);
	else if ((errno = pthread_mutex_init(mutex, &attr)) != 0)
		err_sys("%s: pthread_mutex_init", __func__);
	else if ((errno = pthread_mutexattr_destroy(&attr)) != 0)
		err_sys("%s: pthread_mutexattr_destroy", __func__);
}
