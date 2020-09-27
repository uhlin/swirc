/* commands/znc.cpp
   Copyright (C) 2020 Markus Uhlin. All rights reserved.

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
    if (strings_match(data, "")) {
	print_and_free("/znc: missing arguments", NULL);
	return;
    }

    char *dcopy = sw_strdup(data);
    bool written_linefeed = false;
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
    } else if (tokens.size() != 2) {
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
