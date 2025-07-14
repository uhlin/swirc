/* Coordinated universal time
   Copyright (C) 2025 Markus Uhlin. All rights reserved.

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

#include <ctime>

#include "../printtext.h"
#include "../strHand.h"

#include "utctime.h"

/*
 * usage: /utctime
 */
void
cmd_utctime(CSTRING data)
{
	CSTRING		str[2];
	char		buf[2][100];
	struct tm	tm[2];

	if (!strings_match(data, "")) {
		printtext_print("err", "implicit data");
		return;
	}

	const time_t now = time(nullptr);

#if defined(UNIX)
	if (gmtime_r(&now, &tm[0]) == nullptr ||
	    localtime_r(&now, &tm[1]) == nullptr) {
		printtext_print("err", "time conversion failed");
		return;
	}
#elif defined(WIN32)
	if ((errno = gmtime_s(&tm[0], &now)) != 0 ||
	    (errno = localtime_s(&tm[1], &now)) != 0) {
		printtext_print("err", "time conversion failed");
		return;
	}
#endif

	if (strftime(buf[0], ARRAY_SIZE(buf[0]), "%c", &tm[0]) == 0 ||
	    strftime(buf[1], ARRAY_SIZE(buf[1]), "%c", &tm[1]) == 0) {
		printtext_print("err", "failed to format date and time");
		return;
	}

	str[0] = &buf[0][0];
	str[1] = &buf[1][0];

	printtext_print("success", "UTC time:   %s", str[0]);
	printtext_print("success", "Local time: %s", str[1]);
}
