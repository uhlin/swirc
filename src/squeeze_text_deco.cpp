/* squeeze_text_deco.cpp
   Copyright (C) 2022-2025 Markus Uhlin. All rights reserved.

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

#include "assertAPI.h"
#include "dataClassify.h"
#include "errHand.h"
#include "libUtils.h"
#include "printtext.h"
#include "strHand.h"

#if defined(UNIX)
#define PRINT_SIZE	"%zu"
#elif defined(WIN32)
#define PRINT_SIZE	"%Iu"
#endif

#define MBS_SIZE 6144
#define TMP_SIZE 4096

static const char reject[] =
    TXT_BLINK
    TXT_BOLD
    TXT_NORMAL
    TXT_REVERSE
    TXT_UNDERLINE;

static void
handle_foo_situation(char *buffer, long int &i, long int &j)
{
	if (!buffer[i])
		return;
	else if (buffer[i] == COLOR)
		i--;
	else if (strchr(reject, buffer[i]) == nullptr)
		buffer[j++] = buffer[i];
}

/*
 * check for ^CN
 */
static cc_check_t
check1(char *buffer, long int &i, long int &j)
{
	if (!sw_isdigit(buffer[++i])) {
		handle_foo_situation(buffer, i, j);
		return STOP_INTERPRETING;
	}
	return GO_ON;
}

/*
 * check for ^CNN or ^CN,
 */
static cc_check_t
check2(char *buffer, long int &i, long int &j)
{
	if (!sw_isdigit(buffer[++i]) && buffer[i] != ',') {
		handle_foo_situation(buffer, i, j);
		return STOP_INTERPRETING;
	}
	return GO_ON;
}

/*
 * check for ^CNN, or ^CN,N
 */
static cc_check_t
check3(char *buffer, long int &i, long int &j, bool &has_comma)
{
	if (!has_comma && buffer[i] == ',') {
		has_comma = true;
	} else if (has_comma && sw_isdigit(buffer[i])) {
		/* ^CN,N */;
	} else if (has_comma && !sw_isdigit(buffer[i])) {
		i--;
		handle_foo_situation(buffer, i, j);
		return STOP_INTERPRETING;
	} else {
		handle_foo_situation(buffer, i, j);
		return STOP_INTERPRETING;
	}
	return GO_ON;
}

/*
 * check for ^CNN,N or ^CN,NN
 */
static cc_check_t
check4(char *buffer, long int &i, long int &j)
{
	if (buffer[i] == ',') { /* ^CNN, */
		if (!sw_isdigit(buffer[++i])) {
			i--;
			handle_foo_situation(buffer, i, j);
			return STOP_INTERPRETING;
		}
	} else { /* ^CN,N */
		sw_assert(sw_isdigit(buffer[i]));
		if (sw_isdigit(buffer[++i])) /* we have ^CN,NN? */
			return STOP_INTERPRETING;
		handle_foo_situation(buffer, i, j);
		return STOP_INTERPRETING;
	}
	return GO_ON;
}

/*
 * check for ^CNN,NN
 */
static cc_check_t
check5(char *buffer, long int &i, long int &j)
{
	if (!sw_isdigit(buffer[++i])) {
		handle_foo_situation(buffer, i, j);
		return STOP_INTERPRETING;
	}
	return GO_ON;
}

static void
color(char *buffer, long int &i, long int &j)
{
	bool has_comma;

	if (check1(buffer, i, j) != GO_ON ||
	    check2(buffer, i, j) != GO_ON)
		return;

	has_comma = buffer[i++] == ',';

	if (check3(buffer, i, j, has_comma) != GO_ON)
		return;
	sw_assert(has_comma);
	if (check4(buffer, i, j) != GO_ON ||
	    check5(buffer, i, j) != GO_ON)
		return;
}

char *
squeeze_text_deco(char *buffer)
{
	long int i, j;

	if (buffer == nullptr)
		err_exit(EINVAL, "%s", __func__);
	else if (strings_match(buffer, ""))
		return buffer;

	i = j = 0;

	while (buffer[i] != '\0') {
		if (buffer[i] != COLOR) {
			if (strchr(reject, buffer[i]) == nullptr)
				buffer[j++] = buffer[i];
		} else
			color(buffer, i, j);
		i++;
	}

	buffer[j] = '\0';
	return buffer;
}

static bool
store_mbs(char (&mbs)[MBS_SIZE], const wchar_t *wcs)
{
	const size_t	MAXBYTES = ARRAY_SIZE(mbs) - 1;
	size_t		bytes_stored = 0;

#if defined(UNIX)
	if ((bytes_stored = wcstombs(mbs, wcs, MAXBYTES)) ==
	    g_conversion_failed)
		return false;
#elif defined(WIN32)
	if ((errno = wcstombs_s(&bytes_stored, mbs, ARRAY_SIZE(mbs), wcs,
	    MAXBYTES)) != 0)
		return false;
#endif
	mbs[MAXBYTES] = '\0';

	if (bytes_stored >= MAXBYTES)
		err_log(0, "%s: maximum number of bytes stored", __func__);
	return true;
}

static bool
store_tmp(wchar_t (&tmp)[TMP_SIZE], const char *mbs)
{
	const size_t	MAXELEMENTS = ARRAY_SIZE(tmp) - 1;
	size_t		elements_stored = 0;

#if defined(UNIX)
	if ((elements_stored = mbstowcs(tmp, mbs, MAXELEMENTS)) ==
	    g_conversion_failed)
		return false;
#elif defined(WIN32)
	if ((errno = mbstowcs_s(&elements_stored, tmp, ARRAY_SIZE(tmp), mbs,
	    MAXELEMENTS)) != 0)
		return false;
#endif
	tmp[MAXELEMENTS] = L'\0';

	if (elements_stored >= MAXELEMENTS)
		err_log(0, "%s: maximum number of elements stored", __func__);
	return true;
}

/*
 * This function is to be used in a special context in the printtext
 * module. It may not meet the requirements elsewhere.
 */
wchar_t *
squeeze_text_deco_wide(wchar_t *buffer)
{
	size_t      buflen, newlen, num;
	char        mbs[MBS_SIZE] = {'\0'};
	wchar_t     tmp[TMP_SIZE] = {L'\0'};

	if (buffer == nullptr)
		err_exit(EINVAL, "%s", __func__);
	else if (wcscmp(buffer, L"") == STRINGS_MATCH)
		return buffer;

	buflen = wcslen(buffer);

	if (!store_mbs(mbs, buffer))
		return buffer;

	squeeze_text_deco(mbs);

	if (!store_tmp(tmp, mbs))
		return buffer;
	if ((newlen = wcslen(tmp)) > buflen) {
		err_log(0, "%s: " PRINT_SIZE " greater than " PRINT_SIZE,
		    __func__, newlen, buflen);
	}

	num = (newlen < buflen ? newlen : buflen);
	wmemcpy(buffer, tmp, num);
	buffer[num] = L'\0';

	return buffer;
}
