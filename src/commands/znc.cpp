#include "common.h"

#include <sstream>
#include <string>
#include <vector>

#include "../libUtils.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "znc.h"

/*
 * usage: /znc [*module] <command>
 */
void
cmd_znc(const char *data)
{
    bool written_linefeed = false;

    if (strings_match(data, "")) {
	print_and_free("/znc: missing arguments", NULL);
	return;
    }

    char *dcopy = sw_strdup(data);
    if (*dcopy == '*')
	written_linefeed = strFeed(dcopy, 1) == 1;
    std::istringstream input(dcopy);
    free_and_null(&dcopy);
    std::vector<std::string> tokens;
    std::string token;

    while (std::getline(input, token))
	tokens.push_back(token);

    if (! (written_linefeed)) {
	if (tokens.size() != 1) {
	    print_and_free("/znc: bogus number of tokens (expected one)", NULL);
	    return;
	}

	(void) net_send("PRIVMSG *status :%s", tokens.at(0).c_str());
	return;
    }

    if (tokens.size() != 2) {
	print_and_free("/znc: bogus number of tokens (expected two)", NULL);
	return;
    } else if (tokens.at(0).at(0) != '*') {
	print_and_free("/znc: bogus module name", NULL);
	return;
    }

    char *module = sw_strdup(tokens.at(0).c_str());
    (void) net_send("PRIVMSG %s :%s", strToLower(module), tokens.at(1).c_str());
    free_and_null(&module);
}
