#include "common.h"

#include "../config.h"
#include "../irc.h"
#include "../strHand.h"

#include "cap.h"

bool
is_sasl_mechanism_supported(const char *mechanism)
{
    return (mechanism && Strings_match(mechanism, "PLAIN"));
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
    (void) compo;
}
