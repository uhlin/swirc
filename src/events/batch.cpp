/* batch.cpp
   Copyright (C) 2024, 2025 Markus Uhlin. All rights reserved.

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
#include <stdint.h>
#include <string>
#include <vector>

#include "../config.h"
#include "../dataClassify.h"
#include "../errHand.h"
#include "../irc.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../theme.h"

#include "batch.h"

#define PRINT_NETJOIN_MSGS 0
#define PRINT_NETSPLIT_MSGS 0

class batch {
public:
	std::vector<std::string>	params;
	std::vector<std::string>	irc_msgs;

	batch()
	{
		this->ref.assign("");
		this->type = BATCH_UNKNOWN;
	}

	batch(CSTRING p_ref, const batch_t p_type)
	{
		this->ref.assign(p_ref);
		this->type = p_type;
	}

	CSTRING
	get_ref(void) const
	{
		return this->ref.c_str();
	}

	batch_t
	get_type(void) const
	{
		return this->type;
	}

private:
	std::string	ref;
	batch_t		type;
};

static std::vector<batch> batch_db;

static bool
save_backlogs_to_disk_yesno(CSTRING label)
{
	CSTRING val = Config("save_backlogs_to_disk");

	if (strings_match_ignore_case(val, "no")) {
		return false;
	} else if (strings_match_ignore_case(val, "both")) {
		return true;
	} else if (strings_match_ignore_case(val, "channels")) {
		return (is_irc_channel(label) ? true : false);
	} else if (strings_match_ignore_case(val, "private")) {
		return (is_irc_channel(label) ? false : true);
	} else {
		err_log(EINVAL, "%s: warning: unrecognized setting value: "
		    "must be 'no', 'both', 'channels' or 'private'", __func__);
	}

	return false;
}

static bool
get_obj_by_ref(CSTRING ref, batch &obj, std::vector<batch>::size_type &pos)
{
	pos = 0;

	for (const batch &x : batch_db) {
		if (strings_match(x.get_ref(), ref)) {
			obj = x;
			return true;
		}

		pos++;
	}

	return false;
}

static void
create_batch(STRING params)
{
	CSTRING     ref, type_str;
	CSTRING     token;
	STRING      last = const_cast<STRING>("");
	batch_t     type;

	if ((ref = strtok_r(params, " ", &last)) == nullptr)
		throw std::runtime_error("null ref");
	else if ((type_str = strtok_r(nullptr, " ", &last)) == nullptr)
		throw std::runtime_error("null type");
	else if (strings_match(type_str, "chathistory"))
		type = BATCH_CHATHISTORY;
	else if (strings_match(type_str, "netjoin"))
		type = BATCH_NETJOIN;
	else if (strings_match(type_str, "netsplit"))
		type = BATCH_NETSPLIT;
	else if (strings_match(type_str, "znc.in/playback"))
		type = BATCH_ZNC_IN_PLAYBACK;
	else
		throw std::runtime_error("unknown batch type");

	batch batch_obj(ref, type);

	while ((token = strtok_r(nullptr, " ", &last)) != nullptr)
		batch_obj.params.push_back(token);

#if defined(__cplusplus) && __cplusplus >= 201103L
	batch_db.emplace_back(batch_obj);
#else
	batch_db.push_back(batch_obj);
#endif
}

static void
chathistory(batch &obj)
{
	PIRC_WINDOW		win;
	PRINTTEXT_CONTEXT	ctx;
	std::string		label("");

	printtext_context_init(&ctx, g_status_window, TYPE_SPEC_NONE, true);

	if (!obj.params.empty())
		label.assign(obj.params.at(0));
	if ((win = window_by_label(label.c_str())) != nullptr)
		ctx.window = win;
	else if (spawn_chat_window(label.c_str(), "") == 0 &&
	    (win = window_by_label(label.c_str())) != nullptr)
		ctx.window = win;

	printtext(&ctx, "--- BEGIN chathistory (%s) ---", label.c_str());

	for (const std::string &str : obj.irc_msgs)
		irc_process_proto_msg(str.c_str());

	printtext(&ctx, "--- END chathistory (%s, %ju msgs) ---", label.c_str(),
	    static_cast<uintmax_t>(obj.irc_msgs.size()));
}

static void
netjoin(batch &obj)
{
#if PRINT_NETJOIN_MSGS
	CSTRING host1, host2;

	host1 = host2 = nullptr;

	if (obj.params.size() >= 2) {
		host1 = obj.params.at(0).c_str();
		host2 = obj.params.at(1).c_str();
	}

	if (host1 != nullptr && host2 != nullptr) {
		printtext_print("warn", "%sNetJoin%s (%ju nicks) %s %s %s",
		    COLOR3, TXT_NORMAL,
		    static_cast<uintmax_t>(obj.irc_msgs.size()),
		    host1, THE_SPEC2, host2);
	}
#endif // PRINT_NETJOIN_MSGS

	for (const std::string &str : obj.irc_msgs)
		irc_process_proto_msg(str.c_str());
}

static void
netsplit(batch &obj)
{
#if PRINT_NETSPLIT_MSGS
	CSTRING host1, host2;

	host1 = host2 = nullptr;

	if (obj.params.size() >= 2) {
		host1 = obj.params.at(0).c_str();
		host2 = obj.params.at(1).c_str();
	}

	if (host1 != nullptr && host2 != nullptr) {
		printtext_print("warn", "%sNetSplit%s (%ju nicks) %s %s %s",
		    COLOR3, TXT_NORMAL,
		    static_cast<uintmax_t>(obj.irc_msgs.size()),
		    host1, THE_SPEC2, host2);
	}
#endif // PRINT_NETSPLIT_MSGS

	for (const std::string &str : obj.irc_msgs)
		irc_process_proto_msg(str.c_str());
}

static void
znc_in_playback(batch &obj)
{
	PIRC_WINDOW		win;
	PRINTTEXT_CONTEXT	ctx;
	bool			logging_save = false;
	std::string		label("");

	printtext_context_init(&ctx, g_status_window, TYPE_SPEC_NONE, true);

	if (!obj.params.empty())
		label.assign(obj.params.at(0));
	if ((win = window_by_label(label.c_str())) != nullptr)
		ctx.window = win;
	else if (spawn_chat_window(label.c_str(), "") == 0 &&
	    (win = window_by_label(label.c_str())) != nullptr)
		ctx.window = win;

	if (win) {
		logging_save = win->logging;
		win->logging = save_backlogs_to_disk_yesno(win->label);
	}
	printtext(&ctx, "--- BEGIN playback (%s) ---", label.c_str());

	for (const std::string &str : obj.irc_msgs)
		irc_process_proto_msg(str.c_str());

	printtext(&ctx, "--- END playback (%s, %ju msgs) ---", label.c_str(),
	    static_cast<uintmax_t>(obj.irc_msgs.size()));
	if (win)
		win->logging = logging_save;
}

static void
process_batch(CSTRING params)
{
	CSTRING ref;
	batch obj("", BATCH_UNKNOWN);
	std::vector<batch>::size_type pos = 0;

	if (strchr(params, ' ') != nullptr)
		throw std::runtime_error("too many parameters");

	ref = params;

	if (!get_obj_by_ref(ref, obj, pos))
		throw std::runtime_error("cannot find a such batch");
	switch (obj.get_type()) {
	case BATCH_CHATHISTORY:
		chathistory(obj);
		break;
	case BATCH_NETJOIN:
		netjoin(obj);
		break;
	case BATCH_NETSPLIT:
		netsplit(obj);
		break;
	case BATCH_ZNC_IN_PLAYBACK:
		znc_in_playback(obj);
		break;
	case BATCH_UNKNOWN:
	default:
		/* unknown batch type */
		throw std::runtime_error("process_batch: unknown batch type");
	}

	batch_db.erase(batch_db.begin() + pos);
}

void
event_batch(struct irc_message_compo *compo)
{
	PRINTTEXT_CONTEXT ctx;

	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);

	try {
		if (compo->params[0] == '+') {
			create_batch(&compo->params[1]);
		} else if (compo->params[0] == '-') {
			process_batch(&compo->params[1]);
		} else
			throw std::runtime_error("batch neither +/-");
	} catch (const std::bad_alloc &e) {
		err_exit(ENOMEM, "%s: error: %s", __func__, e.what());
	} catch (const std::out_of_range &e) {
		printtext(&ctx, "%s: error: %s", __func__, e.what());
	} catch (const std::runtime_error &e) {
		printtext(&ctx, "%s: error: %s", __func__, e.what());
	} catch (...) {
		err_log(0, "%s: error: unknown exception", __func__);
	}
}

void
event_batch_add_irc_msgs(CSTRING ref, CSTRING msg)
{
	PRINTTEXT_CONTEXT ctx;

	printtext_context_init(&ctx, g_status_window, TYPE_SPEC1_WARN, true);

	try {
		batch dummy;
		std::vector<batch>::size_type pos = 0;

		if (!get_obj_by_ref(ref, dummy, pos))
			throw std::runtime_error("cannot find batch");

		batch_db.at(pos).irc_msgs.push_back(msg);
	} catch (const std::bad_alloc &e) {
		err_exit(ENOMEM, "%s: error: %s", __func__, e.what());
	} catch (const std::out_of_range &e) {
		printtext(&ctx, "%s: error: %s", __func__, e.what());
	} catch (const std::runtime_error &e) {
		printtext(&ctx, "%s: error: %s", __func__, e.what());
	} catch (...) {
		err_log(0, "%s: error: unknown exception", __func__);
	}
}

void
event_batch_init(void)
{
	if (!batch_db.empty())
		batch_db.clear();
}

void
event_batch_deinit(void)
{
	if (!batch_db.empty()) {
		printtext_print("warn", "%ju unprocessed batches!",
		    static_cast<uintmax_t>(batch_db.size()));
	}
}
