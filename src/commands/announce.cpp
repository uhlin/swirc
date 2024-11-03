/* Send announcements on IRC
   Copyright (C) 2024 Markus Uhlin. All rights reserved.

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

#include <stdexcept>
#include <string>
#include <vector>

#include "../assertAPI.h"
#include "../dataClassify.h"
#include "../io-loop.h"
#include "../irc.h"
#include "../libUtils.h"
#include "../main.h"
#include "../printtext.h"
#include "../strHand.h"

#include "announce.h"
#include "znc.h"

class announce {
public:
	std::vector<std::string> chans;
	std::string msg;

	announce();
	announce(bool, CSTRING, CSTRING);

	void print(void);
	void send(void);

private:
	bool znc_broadcast;
};

announce::announce()
{
	this->msg.assign("");
	this->znc_broadcast = false;
}

announce::announce(bool p_znc_broadcast, CSTRING p_chans, CSTRING p_msg)
{
	CSTRING			token;
	STRING			str;
	STRING			last = const_cast<STRING>("");
	static chararray_t	sep = ",";

	str = sw_strdup(p_chans);

	if ((token = strtok_r(str, sep, &last)) == nullptr)
		sw_assert_not_reached();
	if (strpbrk(token, g_forbidden_chan_name_chars) == nullptr)
		this->chans.push_back(token);

	while ((token = strtok_r(nullptr, sep, &last)) != nullptr) {
		if (strpbrk(token, g_forbidden_chan_name_chars) == nullptr)
			this->chans.push_back(token);
	}

	free(str);
	this->msg.assign(p_msg);
	this->znc_broadcast = p_znc_broadcast;
}

void
announce::print(void)
{
	std::string chanlist("");

	for (const std::string &str : this->chans) {
		if (!is_irc_channel(str.c_str()))
			chanlist.append("#");
		chanlist.append(str).append(" ");
	}

	printtext_print("sp1", "channels:      %s", chanlist.c_str());
	printtext_print("sp1", "message:       %s", this->msg.c_str());
	printtext_print("sp1", "znc broadcast: %s", (this->znc_broadcast ?
	    "yes" : "no"));
	printtext_print("sp1", " ");
}

void
announce::send(void)
{
	for (const std::string &str : this->chans) {
		std::string winlabel(str);

		if (!is_irc_channel(winlabel.c_str()))
			winlabel.insert(0, "#");
		transmit_user_input(winlabel.c_str(), this->msg.c_str());
	}

	std::string arg("Broadcast");
	arg.append(" ").append(this->msg);

	if (this->znc_broadcast)
		cmd_znc(arg.c_str());
}

static const uint32_t		MAXCHANNELS = 300;
static const uint32_t		MAXMESSAGE = 550;

static const uint32_t		MAXANNOUNCEMENTS = 30;
static std::vector<announce>	announcements;

static bool
subcmd_ok(CSTRING subcmd)
{
	if (strings_match(subcmd, "doit"))
		return true;
	else if (strings_match(subcmd, "list"))
		return true;
	else if (strings_match(subcmd, "new"))
		return true;
	else if (strings_match(subcmd, "rm"))
		return true;
	return false;
}

static void
subcmd_doit(CSTRING arg)
{
	long int no;
	long int val = 0;
	std::vector<announce>::iterator it;

	if (arg == nullptr) {
		printtext_print("err", "%s: insufficient args", __func__);
		return;
	} else if (announcements.empty()) {
		printtext_print("err", "%s: zero announcements", __func__);
		return;
	} else if (!is_numeric(arg)) {
		printtext_print("err", "%s: bad argument", __func__);
		return;
	} else if (!getval_strtol(arg, 0, announcements.size() - 1, &val)) {
		printtext_print("err", "%s: bad number", __func__);
		return;
	}

	no = 0;
	it = announcements.begin();

	while (it != announcements.end()) {
		if (no == val) {
			printtext_print("success", "sending the announcement"
			    "...");
			(*it).send();
			return;
		}

		++no;
		++it;
	}
}

static void
subcmd_list(void)
{
	long int no;
	std::vector<announce>::iterator it;

	if (announcements.empty()) {
		printtext_print("err", "%s: zero announcements", __func__);
		return;
	}

	no = 0;
	it = announcements.begin();

	while (it != announcements.end()) {
		printtext_print("sp1", "----- announcement %ld -----", no);
		(*it).print();
		++no;
		++it;
	}
}

static void
subcmd_new(CSTRING arg1, CSTRING arg2, CSTRING arg3)
{
	bool znc_broadcast;

	if (arg1 == nullptr ||
	    arg2 == nullptr ||
	    arg3 == nullptr) {
		printtext_print("err", "%s: insufficient args", __func__);
		return;
	} else if (!strings_match(arg1, "yes") &&
		   !strings_match(arg1, "no")) {
		printtext_print("err", "%s: what? yes or no?", __func__);
		return;
	} else if (strings_match(arg1, "yes")) {
		znc_broadcast = true;
	} else if (strings_match(arg1, "no")) {
		znc_broadcast = false;
	} else {
		sw_assert_not_reached();
	}

	if (strings_match(arg2, "")) {
		printtext_print("err", "%s: no channels", __func__);
		return;
	} else if (strings_match(arg3, "")) {
		printtext_print("err", "%s: no message", __func__);
		return;
	} else if (!(announcements.size() < MAXANNOUNCEMENTS)) {
		printtext_print("err", "%s: too many announcements", __func__);
		return;
	} else if (strlen(arg2) > MAXCHANNELS) {
		printtext_print("err", "%s: too many channels", __func__);
		return;
	} else if (strlen(arg3) > MAXMESSAGE) {
		printtext_print("err", "%s: too long message", __func__);
		return;
	}

	try {
		announce obj(znc_broadcast, arg2, arg3);

		announcements.push_back(obj);
	} catch (...) {
		printtext_print("err", "%s: error while creating the "
		    "announcement", __func__);
		return;
	}

	printtext_print("success", "ok");
}

static void
subcmd_rm(CSTRING arg)
{
	if (arg == nullptr) {
		printtext_print("err", "%s: insufficient args", __func__);
		return;
	} else if (announcements.empty()) {
		printtext_print("err", "%s: zero announcements", __func__);
		return;
	} else if (is_numeric(arg)) {
		long int num = 0;

		if (!getval_strtol(arg, 0, announcements.size() - 1, &num)) {
			printtext_print("err", "%s: bad number", __func__);
			return;
		}

		try {
			announcements.erase(announcements.begin() + num);
		} catch (...) {
			printtext_print("err", "%s: failed to remove "
			    "element %ld", __func__, num);
			return;
		}

		printtext_print("success", "ok");
	} else if (strings_match(arg, "all")) {
		announcements.clear();
		printtext_print("success", "ok");
	} else {
		printtext_print("err", "%s: bad argument", __func__);
	}
}

/*
 * usage:
 *     /announce [doit|list|new|rm] [args]
 *     /announce doit <#>
 *     /announce list
 *     /announce new <[yes|no]> <chan1[,chan2][...]> <msg>
 *     /announce rm <[#|all]>
 */
void
cmd_announce(CSTRING data)
{
	CSTRING			arg[3];
	CSTRING			subcmd;
	STRING			dcopy;
	STRING			last = const_cast<STRING>("");
	static chararray_t	cmd = "/announce";
	static chararray_t	sep = "\n";

	if (strings_match(data, "")) {
		printtext_print("err", "insufficient args");
		return;
	}

	dcopy = sw_strdup(data);
	(void) strFeed(dcopy, 3);

	if ((subcmd = strtok_r(dcopy, sep, &last)) == nullptr) {
		printf_and_free(dcopy, "%s: insufficient args", cmd);
		return;
	} else if (!subcmd_ok(subcmd)) {
		printf_and_free(dcopy, "%s: invalid subcommand '%s'", cmd,
		    subcmd);
		return;
	}

	arg[0] = strtok_r(nullptr, sep, &last);
	arg[1] = strtok_r(nullptr, sep, &last);
	arg[2] = strtok_r(nullptr, sep, &last);

	if (strings_match(subcmd, "doit"))
		subcmd_doit(arg[0]);
	else if (strings_match(subcmd, "list"))
		subcmd_list();
	else if (strings_match(subcmd, "new"))
		subcmd_new(arg[0], arg[1], arg[2]);
	else if (strings_match(subcmd, "rm"))
		subcmd_rm(arg[0]);
	else
		printtext_print("err", "%s: invalid subcommand", cmd);
	free(dcopy);
}
