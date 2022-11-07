/* squeeze_text_deco.cpp
   Copyright (C) 2022 Markus Uhlin. All rights reserved.

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
#include "printtext.h"
#include "strHand.h"

static void
handle_foo_situation(char *buffer, long int &i, long int &j, const char *reject)
{
	if (!buffer[i])
		return;
	else if (buffer[i] == COLOR)
		i--;
	else if (strchr(reject, buffer[i]) == NULL)
		buffer[j++] = buffer[i];
}

char *
squeeze_text_deco(char *buffer)
{
	static const char reject[] =
	    TXT_BLINK
	    TXT_BOLD
	    TXT_NORMAL
	    TXT_REVERSE
	    TXT_UNDERLINE;
	long int i, j;
#if defined(__cplusplus) && __cplusplus >= 201703L
	[[maybe_unused]] bool has_comma;
#else
	bool has_comma;
#endif

	if (buffer == NULL)
		err_exit(EINVAL, "%s", __func__);
	else if (strings_match(buffer, ""))
		return buffer;

	i = j = 0;

	while (buffer[i] != '\0') {
		switch (buffer[i]) {
		case COLOR:
		{
			/*
			 * check for ^CN
			 */
			if (!sw_isdigit(buffer[++i])) {
				handle_foo_situation(buffer, i, j, reject);
				break;
			}

			/*
			 * check for ^CNN or ^CN,
			 */
			if (!sw_isdigit(buffer[++i]) && buffer[i] != ',') {
				handle_foo_situation(buffer, i, j, reject);
				break;
			}

			has_comma = buffer[i++] == ',';

			/*
			 * check for ^CNN, or ^CN,N
			 */
			if (!has_comma && buffer[i] == ',') {
				has_comma = true;
			} else if (has_comma && sw_isdigit(buffer[i])) {
				/* ^CN,N */;
			} else if (has_comma && !sw_isdigit(buffer[i])) {
				i--;
				handle_foo_situation(buffer, i, j, reject);
				break;
			} else {
				handle_foo_situation(buffer, i, j, reject);
				break;
			}

			sw_assert(has_comma);
			UNUSED_VAR(has_comma);

			/*
			 * check for ^CNN,N or ^CN,NN
			 */
			if (buffer[i] == ',') { /* ^CNN, */
				if (!sw_isdigit(buffer[++i])) {
					i--;
					handle_foo_situation(buffer, i, j,
					    reject);
					break;
				}
			} else { /* ^CN,N */
				sw_assert(sw_isdigit(buffer[i]));
				if (sw_isdigit(buffer[++i])) /* we have ^CN,NN? */
					break;
				handle_foo_situation(buffer, i, j, reject);
				break;
			}

			/*
			 * check for ^CNN,NN
			 */
			if (!sw_isdigit(buffer[++i])) {
				handle_foo_situation(buffer, i, j, reject);
				break;
			}

			break;
		} /* case COLOR */
		default:
			if (strchr(reject, buffer[i]) == NULL)
				buffer[j++] = buffer[i];
			break;
		} /* switch block */

		i++;
	}

	buffer[j] = '\0';
	return buffer;
}
