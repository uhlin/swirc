#include "common.h"

#include "../irc.h"

#include "account.h"

/* event_account

   Examples:
     :nick!user@host ACCOUNT <accountname> (logged into a new account)
     :nick!user@host ACCOUNT *             (logged out of their account) */
void
event_account(struct irc_message_compo *compo)
{
    (void) compo;
}
