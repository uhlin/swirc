/* The cap command
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

#include "../errHand.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "cap.h"

/*
 * usage: /cap [ls | list]
 */
void
cmd_cap(const char *data)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);

	if (strings_match(data, "")) {
		if (net_send("CAP LS") < 0 || net_send("CAP LIST") < 0)
			err_log(ENOTCONN, "/cap");
	} else if (strings_match(data, "ls") || strings_match(data, "LS")) {
		printtext(&ctx, "Capabilities supported by the server:");
		(void) net_send("CAP LS 302");
	} else if (strings_match(data, "list") || strings_match(data, "LIST")) {
		printtext(&ctx, "Capabilities associated with "
		    "the active connection:");
		(void) net_send("CAP LIST");
	} else {
		ctx.spec_type = TYPE_SPEC1_FAILURE;
		printtext(&ctx, "what? ls or list?");
	}
}
