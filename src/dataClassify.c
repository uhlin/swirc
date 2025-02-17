/* Data classification utilities
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

#include <string.h>

#include "dataClassify.h"
#include "errHand.h"
#include "strHand.h"

static const size_t	filename_len_max = 80;
static const size_t	hostname_len_max = 255;
static const size_t	nickname_len_max = 45;
static const size_t	real_name_len_max = 60;
static const size_t	username_len_max = 100;

bool
is_alphabetic(const char *string)
{
	if (string == NULL || *string == '\0')
		return false;

	for (const char *cp = &string[0]; *cp != '\0'; cp++) {
		if (!sw_isalpha(*cp))
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
		{ 0x30A0,  0x30FF,  "Katakana" },
		{ 0x3100,  0x312F,  "Bopomofo" },
		{ 0x31A0,  0x31BF,  "Bopomofo Extended" },
		{ 0x31C0,  0x31EF,  "CJK Strokes" },
		{ 0x31F0,  0x31FF,  "Katakana Phonetic Extensions" },
		{ 0x3200,  0x32FF,  "Enclosed CJK Letters and Months" },
		{ 0x3300,  0x33FF,  "CJK Compatibility" },
		{ 0x3400,  0x4DBF,  "CJK Unified Ideographs Extension A" },
		{ 0x4E00,  0x9FFF,  "CJK Unified Ideographs" },
		{ 0xA000,  0xA48F,  "Yi Syllables" },
		{ 0xA490,  0xA4CF,  "Yi Radicals" },
		{ 0xF900,  0xFAFF,  "CJK Compatibility Ideographs" },
		{ 0xFE30,  0xFE4F,  "CJK Compatibility Forms" },

		{ 0x20000, 0x2A6DF, "CJK Unified Ideographs Extension B" },
		{ 0x2A700, 0x2B739, "CJK Unified Ideographs Extension C" },
		{ 0x2B740, 0x2B81D, "CJK Unified Ideographs Extension D" },
		{ 0x2B820, 0x2CEA1, "CJK Unified Ideographs Extension E" },
		{ 0x2F800, 0x2FA1F, "CJK Compatibility Ideographs Supplement" },
	};
	static const size_t mid = ARRAY_SIZE(array) / 2;

	if (wc < array[0].start || wc > array[ARRAY_SIZE(array) - 1].stop)
		return false;

	for (const RANGE *rp = &array[wc < array[mid].start ? 0 : mid];
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

	for (const char *cp = &string[0]; *cp != '\0'; cp++) {
		if (!sw_isdigit(*cp))
			return false;
	}

	return true;
}

bool
is_printable(const char *str)
{
	if (str == NULL || *str == '\0')
		return false;

	for (const char *cp = str; *cp != '\0'; cp++) {
		if (!sw_isprint(*cp))
			return false;
	}

	return true;
}

bool
is_valid_filename(const char *filename)
{
	static const char legal_index[] =
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    "abcdefghijklmnopqrstuvwxyz"
	    "0123456789 ()+-._[]";

	if (filename == NULL || *filename == '\0' ||
	    xstrnlen(filename, filename_len_max + 1) > filename_len_max ||
	    strstr(filename, "..") != NULL)
		return false;

	for (const char *cp = &filename[0]; *cp != '\0'; cp++) {
		if (strchr(legal_index, *cp) == NULL)
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

	for (const char *cp = &hostname[0]; *cp != '\0'; cp++) {
		if (strchr(host_chars, *cp) == NULL)
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

	for (const char *cp = &nickname[0]; *cp != '\0'; cp++) {
		if (strchr(legal_index, *cp) == NULL)
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

	for (const char *cp = &real_name[0]; *cp != '\0'; cp++) {
		if (!sw_isprint(*cp))
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

	for (const char *cp = &username[0]; *cp != '\0'; cp++) {
		if (strchr(legal_index, *cp) == NULL)
			return false;
	}

	return true;
}

bool
is_whitespace(const char *string)
{
	if (string == NULL || *string == '\0')
		return false;

	for (const char *cp = &string[0]; *cp != '\0'; cp++) {
		if (!sw_isspace(*cp))
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
	/*
	 * XXX: Please keep the 'fullwidth' array sorted.
	 */
	static const RANGE fullwidth[] = {
		{0x2614, 0x2614,   "Umbrella with rain drops"},
		{0x2615, 0x2615,   "Hot beverage"},
		{0x2648, 0x2653,   "Zodiacal symbols"},
		{0x267F, 0x267F,   "Wheelchair symbol"},
		{0x2693, 0x2693,   "Anchor"},
		{0x26A1, 0x26A1,   "High voltage sign"},
		{0x26AA, 0x26AB,   "Medium white/black circle"},
		{0x26BD, 0x26BE,   "Sport symbols"},
		{0x26C4, 0x26C5,   "Weather symbols"},
		{0x26CE, 0x26CE,   "Ophiuchus (Zodiacal symbol)"},
		{0x26D4, 0x26D4,   "No entry traffic sign"},
		{0x26EA, 0x26EA,   "Church (Map symbol)"},
		{0x26F2, 0x26F2,   "Fountain"},
		{0x26F3, 0x26F3,   "Flag in hole"},
		{0x26F5, 0x26F5,   "Sailboat"},
		{0x26FA, 0x26FA,   "Tent"},
		{0x26FD, 0x26FD,   "Fuel pump"},

		{0x2705, 0x2705,   "White heavy check mark"},
		{0x274C, 0x274C,   "Cross mark"},
		{0x274E, 0x274E,   "Negative squared cross mark"},
		{0x2795, 0x2797,   "Heavy variants of arithmetic symbols"},

		{0xFF01, 0xFF5E,   "Fullwidth ASCII variants"},
		{0xFF5F, 0xFF60,   "Fullwidth brackets"},
		{0xFFE0, 0xFFE6,   "Fullwidth symbol variants"},

		{0x1F300, 0x1F30C, "Weather, landscape, and sky symbols"},
		{0x1F30D, 0x1F310, "Globe symbols"},
		{0x1F311, 0x1F320, "Moon, sun, and star symbols"},
		{0x1F32D, 0x1F32F, "Food symbols"},
		{0x1F330, 0x1F335, "Plant symbols 1"},
		{0x1F337, 0x1F344, "Plant symbols 2"},
		{0x1F345, 0x1F353, "Fruit and vegetable symbols"},
		{0x1F354, 0x1F374, "Food symbols"},
		{0x1F375, 0x1F37C, "Beverage symbols"},
		{0x1F37E, 0x1F37F, "Beverage and food symbols"},
		{0x1F380, 0x1F393, "Celebration symbols"},
		{0x1F3A0, 0x1F3AD, "Entertainment symbols"},
		{0x1F3AE, 0x1F3B0, "Game symbols 1"},
		{0x1F3B2, 0x1F3B4, "Game symbols 2"},
		{0x1F3B5, 0x1F3BC, "Musical symbols"},
		{0x1F3BD, 0x1F3CA, "Sport symbols 1"},
		{0x1F3CF, 0x1F3D3, "Sport symbols 2"},
		{0x1F3E0, 0x1F3F0, "Building and map symbols"},
		{0x1F3F4, 0x1F3F4, "Waving black flag"},
		{0x1F3F8, 0x1F3F9, "Sport symbols"},
		{0x1F3FA, 0x1F3FA, "Amphora"},
//		{0x1F3FB, 0x1F3FF, "Emoji modifiers"},

		{0x1F400, 0x1F42C, "Animal symbols"},
		{0x1F42D, 0x1F43D, "Animal faces"},
		{0x1F43E, 0x1F43E, "Paw prints"},
		{0x1F440, 0x1F440, "Eyes"},
		{0x1F442, 0x1F445, "Facial parts symbols"},
		{0x1F446, 0x1F450, "Hand symbols"},
		{0x1F451, 0x1F463, "Clothing and accessories"},
		{0x1F464, 0x1F477, "Portrait and role symbols"},
		{0x1F478, 0x1F480, "Fairy tale symbols"},
		{0x1F481, 0x1F483, "Role symbols"},
		{0x1F484, 0x1F488, "Personal care symbols"},
		{0x1F489, 0x1F48A, "Medical symbols"},
		{0x1F48B, 0x1F492, "Romance symbols"},
		{0x1F493, 0x1F49F, "Heart symbols"},
		{0x1F4A0, 0x1F4AD, "Comic style symbols"},
		{0x1F4AE, 0x1F4AF, "Japanese school grade symbols"},
		{0x1F4B0, 0x1F4B9, "Money symbols"},
		{0x1F4BA, 0x1F4DC, "Office symbols"},
		{0x1F4DD, 0x1F4F6, "Communication symbols"},
		{0x1F4F7, 0x1F4FC, "Audio and video symbols"},
		{0x1F4FF, 0x1F4FF, "Prayer beads"},

		{0x1F500, 0x1F518, "User interface symbols"},
		{0x1F519, 0x1F51D, "Words with arrows"},
		{0x1F51E, 0x1F51F, "Enclosed alphanumeric symbols"},
		{0x1F520, 0x1F524, "User interface input status symbols"},
		{0x1F525, 0x1F52E, "Tool symbols"},
		{0x1F52F, 0x1F531, "Miscellaneous symbols"},
		{0x1F532, 0x1F539, "Geometric shapes"},
		{0x1F53A, 0x1F53D, "User interface symbols"},
		{0x1F54B, 0x1F54E, "Religious symbols"},
		{0x1F550, 0x1F567, "Clock face symbols"},
		{0x1F57A, 0x1F57A, "Man dancing"},
		{0x1F595, 0x1F595, "Reversed hand with middle finger extended"},
		{0x1F596, 0x1F596, "Raised hand with part between middle and ring fingers"},
		{0x1F5A4, 0x1F5A4, "Black heart"},
		{0x1F5FB, 0x1F5FF, "Cultural symbols"},

		{0x1F600, 0x1F64F, "Emoticons"},
		{0x1F680, 0x1F6A4, "Vehicles"},
		{0x1F6A5, 0x1F6A8, "Traffic signs"},
		{0x1F6A9, 0x1F6C5, "Signage and other symbols"},
		{0x1F6CC, 0x1F6CC, "Sleeping accommodation"},
		{0x1F6D0, 0x1F6D2, "Signage and other symbols"},
		{0x1F6D5, 0x1F6D7, "Map symbols"},
		{0x1F6DD, 0x1F6DF, "Miscellaneous symbols"},
		{0x1F6EB, 0x1F6EB, "Airplane departure"},
		{0x1F6EC, 0x1F6EC, "Airplane arriving"},
		{0x1F6F4, 0x1F6FC, "Vehicles"},

		{0x1F90C, 0x1F90C, "Pinched fingers"},
		{0x1F90D, 0x1F90E, "Colored heart symbols"},
		{0x1F90F, 0x1F90F, "Pinching hand"},
		{0x1F910, 0x1F917, "Emoticon faces"},
		{0x1F918, 0x1F91F, "Hand symbols"},
		{0x1F920, 0x1F92F, "Emoticon faces"},
		{0x1F930, 0x1F937, "Portrait and role symbols"},
		{0x1F938, 0x1F93A, "Sport symbols 1"},
		{0x1F93C, 0x1F93F, "Sport symbols 2"},
		{0x1F940, 0x1F945, "Miscellaneous symbols 1"},
		{0x1F947, 0x1F94F, "Miscellaneous symbols 2"},
		{0x1F950, 0x1F96F, "Food symbols"},
		{0x1F970, 0x1F97A, "Faces"},
		{0x1F97B, 0x1F97F, "Clothing"},
		{0x1F980, 0x1F9AD, "Animal symbols"},
		{0x1F9AE, 0x1F9AF, "Accessibility symbols"},
//		{0x1F9B0, 0x1F9B3, "Emoji components"},
		{0x1F9B4, 0x1F9B7, "Body parts"},
		{0x1F9B8, 0x1F9B9, "Role symbols"},
		{0x1F9BA, 0x1F9BF, "Accessibility symbols"},
		{0x1F9C0, 0x1F9CB, "Food symbols"},
		{0x1F9CC, 0x1F9CC, "Troll"},
		{0x1F9CD, 0x1F9CF, "Portrait and accessibility symbols"},
		{0x1F9D0, 0x1F9D8, "Portrait and role symbols"},
		{0x1F9D9, 0x1F9DF, "Fantasy beings"},
		{0x1F9E0, 0x1F9E6, "Miscellaneous symbols"},
		{0x1F9E7, 0x1F9E9, "Activities"},
		{0x1F9EA, 0x1F9FF, "Objects"},
	};
	static const size_t mid = ARRAY_SIZE(fullwidth) / 2;

#if TEST_XWCWIDTH
#pragma message("warning: test included (not to be used in production)")
	if (wc == L'\0' && fwlen == 7357) {
		for (const RANGE *rp = &fullwidth[0];
		    rp < &fullwidth[ARRAY_SIZE(fullwidth)];
		    rp++) {
			if (rp->start > rp->stop)
				err_quit("%s: test failed", __func__);
		}
	}
#endif

	if (wc >= 0x20 && wc <= 0xFF)
		return 1;
	else if (wc < 0x20 || is_combined(wc))
		return 0;
	else if (wc < fullwidth[0].start ||
	    wc > fullwidth[ARRAY_SIZE(fullwidth) - 1].stop)
		return (is_cjk(wc) ? fwlen : 1);

	const size_t begin = (wc < fullwidth[mid].start ? 0 : mid);
	const size_t end = (wc < fullwidth[mid].start ? mid :
	    ARRAY_SIZE(fullwidth));

	for (const RANGE *rp = &fullwidth[begin]; rp < &fullwidth[end]; rp++) {
		if (wc >= rp->start && wc <= rp->stop)
			return fwlen;
	}
	return (is_cjk(wc) ? fwlen : 1);
}

int
xwcswidth(const wchar_t *str, const int fwlen)
{
	const wchar_t	*ptr = str;
	unsigned int	 width = 0;

	while (*ptr) {
		width += xwcwidth(*ptr, fwlen);
		ptr++;
	}
	return width;
}
