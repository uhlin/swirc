/* Informational commands
   Copyright (C) 2023 Markus Uhlin. All rights reserved.

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

#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "info.h"

/*
 * usage: /admin [target]
 */
void
cmd_admin(CSTRING data)
{
	if (strings_match(data, "")) {
		if (net_send("ADMIN") < 0)
			printtext_print("err", "cannot send");
	} else {
		if (net_send("ADMIN %s", data) < 0)
			printtext_print("err", "cannot send");
	}
}

/*
 * usage: /info [target]
 */
void
cmd_info(CSTRING data)
{
	if (strings_match(data, "")) {
		if (net_send("INFO") < 0)
			printtext_print("err", "cannot send");
	} else {
		if (net_send("INFO %s", data) < 0)
			printtext_print("err", "cannot send");
	}
}

/*
 * usage: /ison <nick1> [nick2] [nick3] [...]
 */
void
cmd_ison(CSTRING data)
{
	if (strings_match(data, ""))
		printtext_print("err", "missing args");
	else if (net_send("ISON %s", data) < 0)
		printtext_print("err", "cannot send");
	else
		return;
}

/*
 * usage: /servstats [<query> [<target>]]
 */
void
cmd_servstats(CSTRING data)
{
	if (strings_match(data, "")) {
		if (net_send("STATS") < 0)
			printtext_print("err", "cannot send");
	} else {
		if (net_send("STATS %s", data) < 0)
			printtext_print("err", "cannot send");
	}
}
