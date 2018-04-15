/* events/auth.c
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

#include "base64.h"
#include "common.h"

#include "../config.h"
#include "../errHand.h"
#include "../irc.h"
#include "../libUtils.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../strdup_printf.h"

/* solve_ecdsa_nist256p_challenge() */
#include "../commands/sasl.h"

#include "auth.h"
#include "cap.h" /* get_sasl_mechanism() */

static void
abort_authentication()
{
    (void) net_send("AUTHENTICATE *");
    (void) net_send("CAP END");
}

/*lint -sem(get_b64_encoded_username, r_null) */
static char *
get_b64_encoded_username()
{
    const char *username = Config("sasl_username");

    if (strings_match(username, ""))
	return NULL;

    char *encoded_username = xmalloc(500);

    if (b64_encode((uint8_t *) username, strlen(username),
		   encoded_username, 500) == -1) {
	free(encoded_username);
	return NULL;
    }

    return encoded_username;
}

static bool
build_auth_message(char **msg)
{
    const char *username = Config("sasl_username");
    const char *password = Config("sasl_password");

    if (strings_match(username, "") || strings_match(password, "")) {
	*msg = NULL;
	return false;
    }
    char *msg_unencoded = Strdup_printf("%s%c%s%c%s",
	username, '\0', username, '\0', password);
    size_t len = strlen(username) * 2;
    len += 2;
    len += strlen(password);
    *msg = xmalloc(1000);
    if (b64_encode((uint8_t *) msg_unencoded, len, *msg, 1000) == -1) {
	free(msg_unencoded);
	free(*msg);
	*msg = NULL;
	return false;
    }
    free(msg_unencoded);
    return true;
}

static void
handle_ecdsa_nist256p_challenge(const char *challenge)
{
    char *err_reason = "";
    char *solution = solve_ecdsa_nist256p_challenge(challenge, &err_reason);

    if (solution == NULL) {
	err_log(0, "solve_ecdsa_nist256p_challenge failed: %s", err_reason);
	abort_authentication();
	return;
    }

    (void) net_send("AUTHENTICATE %s", solution);
    free(solution);
}

void
event_authenticate(struct irc_message_compo *compo)
{
    const char *mechanism = get_sasl_mechanism();

    if (strings_match(compo->params, "+")) {
	if (strings_match(mechanism, "ECDSA-NIST256P-CHALLENGE")) {
	    char *encoded_username = get_b64_encoded_username();

	    if (!encoded_username) {
		abort_authentication();
		return;
	    }

	    net_send("AUTHENTICATE %s", encoded_username);
	    free(encoded_username);
	} else if (strings_match(mechanism, "PLAIN")) {
	    char *msg = NULL;

	    if (!build_auth_message(&msg)) {
		abort_authentication();
		return;
	    }

	    net_send("AUTHENTICATE %s", msg);
	    free(msg);
	} else {
	    err_log(0, "SASL mechanism unknown  --  aborting authentication!");
	    abort_authentication();
	    return;
	}
    } else { /*=== not 'AUTHENTICATE +' ===*/
	if (strings_match(mechanism, "ECDSA-NIST256P-CHALLENGE"))
	    handle_ecdsa_nist256p_challenge(compo->params);
    }
}

/* Examples:
     :server 902 <nick> :You must use a nick assigned to you
     :server 904 <nick> :SASL authentication failed
     :server 905 <nick> :SASL message too long
     :server 907 <nick> :You have already authenticated using SASL */
void
handle_sasl_auth_fail(struct irc_message_compo *compo)
{
    irc_extract_msg(compo, g_status_window, 1, true);
    abort_authentication();
}

/* sasl_auth_success: 903 (RPL_SASLSUCCESS)

   Example:
     :server 903 <nick> :SASL authentication successful */
void
sasl_auth_success(struct irc_message_compo *compo)
{
    struct printtext_context ctx = {
	.window	    = g_status_window,
	.spec_type  = TYPE_SPEC1_SUCCESS,
	.include_ts = true,
    };

    (void) compo;

    printtext(&ctx, "SASL authentication successful");
    net_send("CAP END");
}
