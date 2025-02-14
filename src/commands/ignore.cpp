/* Ignore commands
   Copyright (C) 2021-2025 Markus Uhlin. All rights reserved.

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

#include <climits>
#include <regex>
#include <stdexcept>
#include <string>

#include "../errHand.h"
#include "../libUtils.h"
#include "../printtext.h"
#include "../strHand.h"

#include "ignore.h"

class ignore {
	std::string	str;
	std::regex	regex;

public:
	explicit ignore(const char *);

	const std::string &
	get_str(void) const
	{
		return this->str;
	}

	const std::regex &
	get_regex(void) const
	{
		return this->regex;
	}
};

ignore::ignore(const char *_str)
{
	this->str.assign(_str);
	this->regex.assign(_str, std::regex::basic);
}

static const size_t		regex_maxlen = 60;
static std::vector<ignore>	ignore_list;

static void
print_ignore_list()
{
	PRINTTEXT_CONTEXT ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1, true);
	printtext(&ctx, "List of ignores:");

	int	no = 0;
	auto	it = ignore_list.begin();

	while (it != ignore_list.end()) {
		printtext(&ctx, "%d: %s", no, it->get_str().c_str());
		++no;
		++it;
	}
}

/*
 * usage: /ignore [regex]
 */
void
cmd_ignore(const char *data)
{
	PRINTTEXT_CONTEXT	 ctx;
	char			*err_reason = nullptr;

	if (strings_match(data, "")) {
		print_ignore_list();
		return;
	} else if (!is_valid_regex(data, &err_reason)) {
		print_and_free(err_reason, err_reason);
		return;
	} else if (ignore_list.size() >= MAXIGNORES) {
		printtext_print("err", "too many ignores!");
		return;
	}

	ignore object(data);
	ignore_list.push_back(object);

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_SUCCESS, true);
	printtext(&ctx, "Added \"%s\" to ignore list.",
	    object.get_str().c_str());
}

/*
 * usage: /unignore [#]
 */
void
cmd_unignore(const char *data)
{
	PRINTTEXT_CONTEXT	 ctx;
	char			*regex = nullptr;

	if (strings_match(data, "")) {
		print_ignore_list();
		return;
	}

	try {
		char		*ep = const_cast<char *>("");
		long int	 no;

		if (ignore_list.empty())
			throw std::runtime_error("ignore list empty");

		errno = 0;
		no = strtol(data, &ep, 10);

		if (data[0] == '\0' || *ep != '\0')
			throw std::runtime_error("not a number");
		else if (errno == ERANGE && (no == LONG_MAX || no == LONG_MIN))
			throw std::runtime_error("out of range");
		else if (no < 0 ||
		    static_cast<size_t>(no) >= ignore_list.size() ||
		    no > MAXIGNORES)
			throw std::runtime_error("out of range");

		regex = sw_strdup(ignore_list.at(no).get_str().c_str());
		ignore_list.erase(ignore_list.begin() + no);
		printtext_context_init(&ctx, g_active_window,
		    TYPE_SPEC1_SUCCESS, true);
		printtext(&ctx, "Deleted \"%s\" from ignore list.", regex);
		free(regex);
		regex = nullptr;
		print_ignore_list();
		return;
	} catch (const std::out_of_range &e) {
		printtext_context_init(&ctx, g_active_window,
		    TYPE_SPEC1_FAILURE, true);
		printtext(&ctx, "/unignore: %s", e.what());
	} catch (const std::runtime_error &e) {
		printtext_context_init(&ctx, g_active_window,
		    TYPE_SPEC1_FAILURE, true);
		printtext(&ctx, "/unignore: %s", e.what());
	} catch (...) {
		err_log(0, "/unignore: %s", "unknown exception!");
	}

	free(regex);
}

bool
is_in_ignore_list(const char *nick, const char *user, const char *host)
{
	if (nick == nullptr || user == nullptr || host == nullptr ||
	    ignore_list.empty())
		return false;

	std::string nuh(nick);
	nuh.append("!");
	nuh.append(user).append("@").append(host);

	for (auto it = ignore_list.begin(); it != ignore_list.end(); ++it) {
		if (std::regex_match(nuh, it->get_regex())) {
			debug("is_in_ignore_list: \"%s\" matches \"%s\": "
			    "returning true...", it->get_str().c_str(),
			    nuh.c_str());
			return true;
		}
	}

	return false;
}

bool
is_valid_regex(const char *str, char **err_reason)
{
	if (strings_match(str, "")) {
		*err_reason = sw_strdup("no regex");
		return false;
	} else if (xstrnlen(str, (regex_maxlen + 1)) > regex_maxlen) {
		*err_reason = sw_strdup("regex too long");
		return false;
	}

	try {
		std::regex	regex(str, std::regex::basic);
	} catch (const std::regex_error &e) {
		*err_reason = sw_strdup(e.what());
		return false;
	}

	*err_reason = nullptr;
	return true;
}
