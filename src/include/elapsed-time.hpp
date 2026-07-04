#ifndef SRC_INCLUDE_ELAPSED_TIME_HPP_
#define SRC_INCLUDE_ELAPSED_TIME_HPP_
/* Elapsed Time
   Copyright (C) 2026 Markus Uhlin. All rights reserved.

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

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <stdexcept>

#include "colors.h"

class elapsed_time {
public:
	elapsed_time();
	elapsed_time(int64_t, int64_t);

	uint32_t
	get_days(void) const
	{
		return this->days;
	}

	uint32_t
	get_hours(void) const
	{
		return this->hours;
	}

	uint32_t
	get_mins(void) const
	{
		return this->mins;
	}

	uint32_t
	get_secs(void) const
	{
		return this->secs;
	}

	const char *
	get_start_time(void) const
	{
		return (&this->buf[0][0]);
	}

	const char *
	get_stop_time(void) const
	{
		return (&this->buf[1][0]);
	}

	const char	*get_uptime(void);
	const char	*get_uptime_decorated(void);

private:
	int64_t		diff;

	uint32_t	days;
	uint32_t	hours;
	uint32_t	mins;
	uint32_t	secs;

	char		buf[2][200];
	char		upmsg[800];
};

elapsed_time::elapsed_time()
    : diff(0)
    , days(0)
    , hours(0)
    , mins(0)
    , secs(0)
{
	(void) memset(&this->buf[0][0], 0, sizeof(this->buf[0]));
	(void) memset(&this->buf[1][0], 0, sizeof(this->buf[1]));

	(void) memset(this->upmsg, 0, sizeof(this->upmsg));
}

elapsed_time::elapsed_time(int64_t p_start, int64_t p_stop)
    : diff(0)
    , days(0)
    , hours(0)
    , mins(0)
    , secs(0)
{
	char		*cp[2];
	const char	*conv_fail_msg = "time conversion failed";
	struct tm	 res[2];
	time_t		 tval[2] = {
		static_cast<time_t>(p_start),
		static_cast<time_t>(p_stop)
	};

	if (p_start > p_stop) {
		char reason[600] = { '\0' };

		(void) snprintf(reason, sizeof reason, "%s: "
		    "start greater than stop "
		    "(" "%" PRId64 " > " "%" PRId64 ")", __func__,
		    p_start,
		    p_stop);
		throw std::runtime_error(reason);
	} else if ((this->diff = (p_stop - p_start)) < 0)
		this->diff = 0; // XXX

	this->days	= this->diff / (60 * 60 * 24);
	this->hours	= (this->diff % (60 * 60 * 24)) / (60 * 60);
	this->mins	= ((this->diff % (60 * 60 * 24)) % (60 * 60)) / 60;
	this->secs	= ((this->diff % (60 * 60 * 24)) % (60 * 60)) % 60;

#if defined(UNIX)
	if (localtime_r(&tval[0], &res[0]) == nullptr ||
	    localtime_r(&tval[1], &res[1]) == nullptr)
		throw std::runtime_error(conv_fail_msg);
#elif defined(WIN32)
	if ((errno = localtime_s(&res[0], &tval[0])) != 0 ||
	    (errno = localtime_s(&res[1], &tval[1])) != 0)
		throw std::runtime_error(conv_fail_msg);
#endif

	cp[0] = &this->buf[0][0];
	cp[1] = &this->buf[1][0];

	(void) memset(cp[0], 0, sizeof(this->buf[0]));
	(void) memset(cp[1], 0, sizeof(this->buf[1]));

	(void) memset(this->upmsg, 0, sizeof(this->upmsg));

	if (strftime(cp[0], sizeof(this->buf[0]), "%c", &res[0]) == 0 ||
	    strftime(cp[1], sizeof(this->buf[1]), "%c", &res[1]) == 0)
		throw std::runtime_error("cannot format date and time");
}

const char *
elapsed_time::get_uptime(void)
{
	int ret;

	ret = snprintf(this->upmsg, sizeof(this->upmsg), ("Up for "
	    "%" PRIu32 " day%s, "
	    "%" PRIu32 " hour%s, "
	    "%" PRIu32 " minute%s and "
	    "%" PRIu32 " second%s"),
	    this->days, (this->days == 1 ? "" : "s"),
	    this->hours, (this->hours == 1 ? "" : "s"),
	    this->mins, (this->mins == 1 ? "" : "s"),
	    this->secs, (this->secs == 1 ? "" : "s"));

	if (ret < 0 || static_cast<size_t>(ret) >= sizeof(this->upmsg))
		return "";
	return (&this->upmsg[0]);
}

const char *
elapsed_time::get_uptime_decorated(void)
{
	int ret;

	ret = snprintf(this->upmsg, sizeof(this->upmsg), ("Up for "
	    "%s" "%" PRIu32 "%s %sday%s%s, "
	    "%s" "%" PRIu32 "%s %shour%s%s, "
	    "%s" "%" PRIu32 "%s %sminute%s%s and "
	    "%s" "%" PRIu32 "%s %ssecond%s%s"),
	    BOLDGREEN, this->days, NORMAL,
	    BOLDBLACK, (this->days == 1 ? "" : "s"), NORMAL,
	    BOLDGREEN, this->hours, NORMAL,
	    BOLDBLACK, (this->hours == 1 ? "" : "s"), NORMAL,
	    BOLDGREEN, this->mins, NORMAL,
	    BOLDBLACK, (this->mins == 1 ? "" : "s"), NORMAL,
	    BOLDGREEN, this->secs, NORMAL,
	    BOLDBLACK, (this->secs == 1 ? "" : "s"), NORMAL);

	if (ret < 0 || static_cast<size_t>(ret) >= sizeof(this->upmsg))
		return "";
	return (&this->upmsg[0]);
}
#endif	// SRC_INCLUDE_ELAPSED_TIME_HPP_
