/* commands/squery.cpp
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
#include <stdexcept>
#include <string>
#include <vector>

#include "../libUtils.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"

#include "squery.h"

/*
 * usage: /squery <servicename> <text>
 */
void
cmd_squery(const char *data)
{
    if (strings_match(data, "")) {
	print_and_free("/squery: missing arguments", NULL);
	return;
    }

    char *dcopy = sw_strdup(data);
    (void) strFeed(dcopy, 1);
    std::istringstream input(dcopy);
    free_and_null(&dcopy);
    std::vector<std::string> tokens;
    std::string token;

    while (std::getline(input, token))
	tokens.push_back(token);

    try {
	if (tokens.size() != 2)
	    throw std::runtime_error("missing arguments");

	const char *servicename = tokens.at(0).c_str();
	const char *text = tokens.at(1).c_str();

	if (net_send("SQUERY %s :%s", servicename, text) < 0)
	    throw std::runtime_error("cannot send");
    } catch (std::runtime_error &e) {
	std::string s("/squery: ");
	s.append(e.what());
	print_and_free(s.c_str(), NULL);
    }
}
