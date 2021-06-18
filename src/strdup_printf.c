/* Duplicate a printf style format string
   Copyright (C) 2012-2021 Markus Uhlin. All rights reserved.

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

#include <stdio.h>

#include "errHand.h"
#include "mutex.h"
#include "strdup_printf.h"

#if defined(UNIX)
static pthread_once_t	init_done = PTHREAD_ONCE_INIT;
static pthread_mutex_t	mutex;
#elif defined(WIN32)
static init_once_t	init_done = ONCE_INITIALIZER;
static HANDLE		mutex;
#endif

static void
mutex_init(void)
{
    mutex_new(&mutex);
}

static int
get_size(const char *fmt, va_list ap)
{
	int	size;
	va_list	ap_copy;

	va_copy(ap_copy, ap);
#if defined(UNIX)
	size = vsnprintf(NULL, 0, fmt, ap_copy); /* C99 */
#elif defined(WIN32)
	size = _vscprintf(fmt, ap_copy);
#endif
	va_end(ap_copy);

	return size;
}

char *
strdup_printf(const char *fmt, ...)
{
	char	*buffer;
	va_list	 ap;

	va_start(ap, fmt);
	buffer = strdup_vprintf(fmt, ap);
	va_end(ap);

	return buffer;
}

char *
strdup_vprintf(const char *fmt, va_list ap)
{
	char	*buffer;
	int	 n_print;
	int	 size;

#if defined(UNIX)
	if ((errno = pthread_once(&init_done, mutex_init)) != 0)
		err_sys("strdup_vprintf: pthread_once");
#elif defined(WIN32)
	if ((errno = init_once(&init_done, mutex_init)) != 0)
		err_sys("strdup_vprintf: init_once");
#endif

	mutex_lock(&mutex);

	if ((size = get_size(fmt, ap)) < 0)
		err_exit(ENOSYS, "strdup_vprintf: get_size");
	else
		size += 1;

	if ((buffer = malloc(size)) == NULL) {
		err_exit(ENOMEM, "strdup_vprintf: malloc (allocating %d bytes)",
		    size);
	}

	errno = 0;

#if defined(UNIX)
	if ((n_print = vsnprintf(buffer, size, fmt, ap)) < 0 || n_print >= size)
		err_sys("strdup_vprintf: vsnprintf() returned %d", n_print);
#elif defined(WIN32)
	if ((n_print = vsnprintf_s(buffer, size, size - 1, fmt, ap)) < 0)
		err_sys("strdup_vprintf: vsnprintf_s() returned %d", n_print);
#endif

	mutex_unlock(&mutex);

	return buffer;
}
