/* Command op and deop
   Copyright (C) 2019-2023 Markus Uhlin. All rights reserved.

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

#include "../dataClassify.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "op.h"

static const char	err1[] = "missing arguments";
static const char	err2[] = "invalid nickname";
static const char	err3[] = "active window isn't an irc channel";

/*
 * usage: /op <nick>
 */
void
cmd_op(const char *data)
{
	static const char cmd[] = "/op";

	if (strings_match(data, "")) {
		printtext_print("err", "%s: %s", cmd, err1);
		return;
	} else if (!is_valid_nickname(data)) {
		printtext_print("err", "%s: %s", cmd, err2);
		return;
	} else if (!is_irc_channel(ACTWINLABEL)) {
		printtext_print("err", "%s: %s", cmd, err3);
		return;
	}

	(void) net_send("MODE %s +o %s", ACTWINLABEL, data);
}

/*
 * usage: /deop <nick>
 */
void
cmd_deop(const char *data)
{
	static const char cmd[] = "/deop";

	if (strings_match(data, "")) {
		printtext_print("err", "%s: %s", cmd, err1);
		return;
	} else if (!is_valid_nickname(data)) {
		printtext_print("err", "%s: %s", cmd, err2);
		return;
	} else if (!is_irc_channel(ACTWINLABEL)) {
		printtext_print("err", "%s: %s", cmd, err3);
		return;
	}

	(void) net_send("MODE %s -o %s", ACTWINLABEL, data);
}
