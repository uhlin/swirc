/* IRCv3 Client Capability Negotiation
   Copyright (C) 2017 Markus Uhlin. All rights reserved.

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

   THIS SOFTWARE IS PROVIDED THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

#include "common.h"

#include "../assertAPI.h"
#include "../config.h"
#include "../irc.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "cap.h"

bool
is_sasl_mechanism_supported(const char *mechanism)
{
    if (!mechanism)
	return false;
    else if (Strings_match(mechanism, "ECDSA-NIST256P-CHALLENGE"))
	return true;
    else if (Strings_match(mechanism, "PLAIN"))
	return true;
    else
	return false;

    /*NOTREACHED*/ sw_assert_not_reached();
    /*NOTREACHED*/ return false;
}

const char *
get_sasl_mechanism(void)
{
    const char *mechanism = Config("sasl_mechanism");

    return (Strings_match(mechanism, "") ? "PLAIN" : mechanism);
}

void
event_cap(struct irc_message_compo *compo)
{
    struct printtext_context ctx = {
	.window	    = g_status_window,
	.spec_type  = TYPE_SPEC1_FAILURE,
	.include_ts = true,
    };

    squeeze(compo->params, ":");

    if (strstr(compo->params, "ACK sasl")) {
	const char *mechanism = get_sasl_mechanism();

	if (is_sasl_mechanism_supported(mechanism)) {
	    net_send("AUTHENTICATE %s", mechanism);
	    return;
	}
    }

    net_send("CAP END");
    printtext(&ctx, "SASL exchange aborted!");
}
