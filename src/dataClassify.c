/* Data classification utilities
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

#include <string.h>

#include "dataClassify.h"

static const size_t	nickname_len_max  = 45;
static const size_t	username_len_max  = 100;
static const size_t	real_name_len_max = 60;
static const size_t	hostname_len_max  = 255;

bool
is_alphabetic(const char *string)
{
	if (string == NULL || *string == '\0')
		return false;

	for (const char *p = &string[0]; *p != '\0'; p++) {
		if (!sw_isalpha(*p))
			return false;
	}

	return true;
}

bool
is_numeric(const char *string)
{
	if (string == NULL || *string == '\0')
		return false;

	for (const char *p = &string[0]; *p != '\0'; p++) {
		if (!sw_isdigit(*p))
			return false;
	}

	return true;
}

bool
is_whiteSpace(const char *string)
{
	if (string == NULL || *string == '\0')
		return false;

	for (const char *p = &string[0]; *p != '\0'; p++) {
		if (!sw_isspace(*p))
			return false;
	}

	return true;
}

bool
is_irc_channel(const char *name)
{
	if (name == NULL || *name == '\0')
		return false;

	return (*name == '&' || *name == '#' || *name == '+' || *name == '!');
}

bool
is_valid_uMode(const char *modes)
{
	const char legal_index[] = "aiwroOs";

	if (modes == NULL || *modes == '\0')
		return false;

	for (const char *p = &modes[0]; *p != '\0'; p++) {
		if (strchr(legal_index, *p) == NULL)
			return false;
	}

	return true;
}

bool
is_valid_nickname(const char *nickname)
{
	const char legal_index[] =
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
	    "-[\\]^_`{|}";

	if (nickname == NULL || *nickname == '\0' ||
	    strlen(nickname) > nickname_len_max)
		return false;

	for (const char *ccp = &nickname[0]; *ccp != '\0'; ccp++) {
		if (strchr(legal_index, *ccp) == NULL)
			return false;
	}

	return true;
}

bool
is_valid_username(const char *username)
{
	const char legal_index[] =
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
	    "$-./[\\]^_`{|}~";

	if (username == NULL || *username == '\0' ||
	    strlen(username) > username_len_max)
		return false;

	for (const char *ccp = &username[0]; *ccp != '\0'; ccp++) {
		if (strchr(legal_index, *ccp) == NULL)
			return false;
	}

	return true;
}

bool
is_valid_real_name(const char *real_name)
{
	if (real_name == NULL || *real_name == '\0' ||
	    strlen(real_name) > real_name_len_max)
		return false;

	for (const char *ccp = &real_name[0]; *ccp != '\0'; ccp++) {
		if (!sw_isprint(*ccp))
			return false;
	}

	return true;
}

bool
is_valid_hostname(const char *hostname)
{
	const char host_chars[] =
	    "abcdefghijklmnopqrstuvwxyz.0123456789-ABCDEFGHIJKLMNOPQRSTUVWXYZ:";

	if (hostname == NULL || *hostname == '\0' ||
	    strlen(hostname) > hostname_len_max)
		return false;

	for (const char *ccp = &hostname[0]; *ccp != '\0'; ccp++) {
		if (strchr(host_chars, *ccp) == NULL)
			return false;
	}

	return true;
}
