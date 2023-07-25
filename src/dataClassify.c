/* Data classification utilities
   Copyright (C) 2012-2023 Markus Uhlin. All rights reserved.

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
#include "strHand.h"

static const size_t	hostname_len_max = 255;
static const size_t	nickname_len_max = 45;
static const size_t	real_name_len_max = 60;
static const size_t	username_len_max = 100;

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

/*
 * Chinese, Japanese and Korean
 */
bool
is_cjk(const wchar_t wc)
{
	static const RANGE array[] = {
		{ 0x2E80,  0x2EFF,  "CJK Radicals Supplement" },
		{ 0x2F00,  0x2FDF,  "Kangxi Radicals" },
		{ 0x3000,  0x303F,  "CJK Symbols and Punctuation" },
		{ 0x31C0,  0x31EF,  "CJK Strokes" },
		{ 0x3200,  0x32FF,  "Enclosed CJK Letters and Months" },
		{ 0x3300,  0x33FF,  "CJK Compatibility" },
		{ 0x3400,  0x4DBF,  "CJK Unified Ideographs Extension A" },
		{ 0x4E00,  0x9FFF,  "CJK Unified Ideographs" },
		{ 0xF900,  0xFAFF,  "CJK Compatibility Ideographs" },
		{ 0xFE30,  0xFE4F,  "CJK Compatibility Forms" },
		{ 0x20000, 0x2A6DF, "CJK Unified Ideographs Extension B" },
		{ 0x2F800, 0x2FA1F, "CJK Compatibility Ideographs Supplement" },

		{ 0x3100,  0x312F,  "Bopomofo" },
		{ 0x31A0,  0x31BF,  "Bopomofo Extended" },
	};

	for (const RANGE *rp = &array[0];
	    rp < &array[ARRAY_SIZE(array)];
	    rp++) {
		if (wc >= rp->start && wc <= rp->stop)
			return true;
	}

	return false;
}

bool
is_combined(const wchar_t wc)
{
	static const RANGE array[] = {
		{ 0x0300, 0x036F, "Combining Diacritical Marks" },
		{ 0x1AB0, 0x1AFF, "Combining Diacritical Marks Extended" }, // stop 0x1ACE?
		{ 0x1DC0, 0x1DFF, "Combining Diacritical Marks Supplement" },
		{ 0x20D0, 0x20FF, "Combining Diacritical Marks for Symbols" }, // stop 0x20F0?
		{ 0xFE20, 0xFE2F, "Combining Half Marks" },
	};

	for (const RANGE *rp = &array[0];
	    rp < &array[ARRAY_SIZE(array)];
	    rp++) {
		if (wc >= rp->start && wc <= rp->stop)
			return true;
	}

	return false;
}

bool
is_irc_channel(const char *name)
{
	if (name == NULL || *name == '\0')
		return false;

	return (*name == '&' || *name == '#' || *name == '+' || *name == '!');
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
is_valid_hostname(const char *hostname)
{
	static const char host_chars[] =
	    "abcdefghijklmnopqrstuvwxyz.0123456789-ABCDEFGHIJKLMNOPQRSTUVWXYZ:";

	if (hostname == NULL || *hostname == '\0' ||
	    xstrnlen(hostname, hostname_len_max + 1) > hostname_len_max)
		return false;

	for (const char *ccp = &hostname[0]; *ccp != '\0'; ccp++) {
		if (strchr(host_chars, *ccp) == NULL)
			return false;
	}

	return true;
}

bool
is_valid_nickname(const char *nickname)
{
	static const char legal_index[] =
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
	    "-[\\]^_`{|}";

	if (nickname == NULL || *nickname == '\0' ||
	    xstrnlen(nickname, nickname_len_max + 1) > nickname_len_max)
		return false;

	for (const char *ccp = &nickname[0]; *ccp != '\0'; ccp++) {
		if (strchr(legal_index, *ccp) == NULL)
			return false;
	}

	return true;
}

bool
is_valid_real_name(const char *real_name)
{
	if (real_name == NULL || *real_name == '\0' ||
	    xstrnlen(real_name, real_name_len_max + 1) > real_name_len_max)
		return false;

	for (const char *ccp = &real_name[0]; *ccp != '\0'; ccp++) {
		if (!sw_isprint(*ccp))
			return false;
	}

	return true;
}

bool
is_valid_username(const char *username)
{
	static const char legal_index[] =
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
	    "$-./[\\]^_`{|}~";

	if (username == NULL || *username == '\0' ||
	    xstrnlen(username, username_len_max + 1) > username_len_max)
		return false;

	for (const char *ccp = &username[0]; *ccp != '\0'; ccp++) {
		if (strchr(legal_index, *ccp) == NULL)
			return false;
	}

	return true;
}

bool
is_whitespace(const char *string)
{
	if (string == NULL || *string == '\0')
		return false;

	for (const char *p = &string[0]; *p != '\0'; p++) {
		if (!sw_isspace(*p))
			return false;
	}

	return true;
}

/*
 * Determines the number of column positions required to display a
 * wide character.
 *
 * Specifically written to be used in the printtext and readline
 * modules.
 *
 * Test the 'wc' using iswprint() initially. (This is done elsewhere
 * so we don't want to do it twice...)
 */
int
xwcwidth(const wchar_t wc, const int fwlen)
{
	static const RANGE fullwidth[] = {
		{ 0xFF01, 0xFF5E, "Fullwidth ASCII variants" },
		{ 0xFF5F, 0xFF60, "Fullwidth brackets" },
		{ 0xFFE0, 0xFFE6, "Fullwidth symbol variants" },
		{ 0x1F600, 0x1F64F, "Emoticons" },
	};

	if (wc >= 0x20 && wc <= 0xFF)
		return 1;
	else if (wc < 0x20 || is_combined(wc))
		return 0;
	for (const RANGE *rp = &fullwidth[0];
	    rp < &fullwidth[ARRAY_SIZE(fullwidth)];
	    rp++) {
		if (wc >= rp->start && wc <= rp->stop)
			return fwlen;
	}
	return (is_cjk(wc) ? fwlen : 1);
}

int
xwcswidth(const wchar_t *str, const int fwlen)
{
	const wchar_t	*ptr = str;
	int		 width = 0;

	while (*ptr) {
		width += xwcwidth(*ptr, fwlen);
		ptr++;
	}
	return width;
}
