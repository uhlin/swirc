#include "base64.h"
#include "common.h"

#include "../config.h"
#include "../irc.h"
#include "../libUtils.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../strdup_printf.h"

#include "auth.h"

/*lint -sem(get_b64_encoded_username, r_null) */
static char *
get_b64_encoded_username()
{
    const char *username = Config("sasl_username");

    if (Strings_match(username, ""))
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

    if (Strings_match(username, "") || Strings_match(password, "")) {
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
abort_authentication()
{
    net_send("AUTHENTICATE *");
    net_send("CAP END");
}

void
event_authenticate(struct irc_message_compo *compo)
{
    if (Strings_match(compo->params, "+")) {
	char *msg = NULL;

	if (!build_auth_message(&msg)) {
	    abort_authentication();
	    return;
	}

	net_send("AUTHENTICATE %s", msg);
	free(msg);
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
