/* IRCv3 Client Capability Negotiation
   Copyright (C) 2017-2022 Markus Uhlin. All rights reserved.

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

#include "../assertAPI.h"
#include "../config.h"
#include "../errHand.h"
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
    else if (strings_match(mechanism, "ECDSA-NIST256P-CHALLENGE"))
	return true;
    else if (strings_match(mechanism, "PLAIN"))
	return true;
    else if (strings_match(mechanism, "SCRAM-SHA-256"))
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

    return (strings_match(mechanism, "") ? "PLAIN" : mechanism);
}

static void
ACK(const char *feature)
{
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_SUCCESS, true);
    printtext(&ctx, "%s accepted", feature);
}

static void
NAK(const char *feature)
{
    PRINTTEXT_CONTEXT ctx;

    printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_FAILURE, true);
    printtext(&ctx, "%s rejected", feature);
}

static bool
shouldContinueCapabilityNegotiation_case1(void)
{
    return (config_bool("away_notify", false) ||
	    config_bool("invite_notify", false) ||
	    config_bool("ircv3_server_time", false) ||
	    sasl_is_enabled());
}

static bool
shouldContinueCapabilityNegotiation_case2(void)
{
    return (config_bool("invite_notify", false) ||
	    config_bool("ircv3_server_time", false) ||
	    sasl_is_enabled());
}

static bool
shouldContinueCapabilityNegotiation_case3(void)
{
    return (config_bool("ircv3_server_time", false) ||
	    sasl_is_enabled());
}

static bool
shouldContinueCapabilityNegotiation_case4(void)
{
    return (sasl_is_enabled());
}

static void
handle_ack_and_nak(PPRINTTEXT_CONTEXT ctx, struct irc_message_compo *compo,
    const char *cmd, const char *caplist, bool *continue_capneg)
{
	if (strings_match(caplist, "account-notify")) {
		if (strings_match(cmd, "ACK"))
			ACK("Account notify");
		else
			NAK("Account notify");
		*continue_capneg = shouldContinueCapabilityNegotiation_case1();
	} else if (strings_match(caplist, "away-notify")) {
		if (strings_match(cmd, "ACK"))
			ACK("Away notify");
		else
			NAK("Away notify");
		*continue_capneg = shouldContinueCapabilityNegotiation_case2();
	} else if (strings_match(caplist, "invite-notify")) {
		if (strings_match(cmd, "ACK"))
			ACK("Invite notify");
		else
			NAK("Invite notify");
		*continue_capneg = shouldContinueCapabilityNegotiation_case3();
	} else if (strings_match(caplist, "server-time")) {
		if (strings_match(cmd, "ACK"))
			ACK("Server time");
		else
			NAK("Server time");
		*continue_capneg = shouldContinueCapabilityNegotiation_case4();
	} else if (strings_match(caplist, "sasl")) {
		if (strings_match(cmd, "ACK")) {
			const char *mechanism;

			ACK("SASL authentication");

			mechanism = get_sasl_mechanism();

			if (!is_sasl_mechanism_supported(mechanism)) {
				err_log(ENOSYS, "Unsupported SASL mechanism"
				    ": '%s'", mechanism);
				return;
			}

			(void) net_send("AUTHENTICATE %s", mechanism);
		} else {
			NAK("SASL authentication");
		}
	} else {
		printtext(ctx, "Unknown acknowledgement during capability "
		    "negotiation...");
		printtext(ctx, "params = %s", compo->params);
		printtext(ctx, "prefix = %s", (compo->prefix ? compo->prefix :
		    "none"));
	}
}

/**
 * event_cap()
 *
 * Examples:
 *     :server.com CAP * LS :multi-prefix sasl
 *     :server.com CAP * LIST :multi-prefix
 *     :server.com CAP * ACK :multi-prefix sasl
 *     :server.com CAP * NAK :multi-prefix sasl
 */
void
event_cap(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT ctx;
	bool continue_capneg = false;
	char *cmd, *caplist;
	char *last = "";

	if (strFeed(compo->params, 2) != 2) {
		err_log(0, "event_cap: strFeed() != 2");
		return;
	}

	/* client identifier */
	(void) strtok_r(compo->params, "\n", &last);

	if ((cmd = strtok_r(NULL, "\n", &last)) == NULL ||
	    (caplist = strtok_r(NULL, "\n", &last)) == NULL)
		return;

	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);

	if (*caplist == ':')
		caplist++;
	(void) trim(caplist);

	if (strings_match(cmd, "LS")) {
		/* list the capabilities supported by the server */;
	} else if (strings_match(cmd, "LIST")) {
		/* list the capabilities associated with the active connection */;
	} else if (strings_match(cmd, "ACK") || strings_match(cmd, "NAK")) {
		handle_ack_and_nak(&ctx, compo, cmd, caplist, &continue_capneg);
		if (continue_capneg)
			return;
		(void) net_send("CAP END");
		printtext(&ctx, "Ended IRCv3 Client Capability Negotiation");
	} else {
		printtext(&ctx, "Unknown command: %s "
		    "(during capability negotiation)", cmd);
	}
}
