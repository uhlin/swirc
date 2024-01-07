#include "common.h"

#include <stdexcept>
#include <string>
#include <vector>

#include "../errHand.h"
#include "../irc.h"
#include "../printtext.h"
#include "../strHand.h"

#include "batch.h"

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

	batch_db.push_back(batch_obj);
}

static void
chathistory(batch &obj)
{
	for (std::string &str : obj.irc_msgs)
		irc_process_proto_msg(str.c_str());
}

static void
netjoin(batch &obj)
{
	for (std::string &str : obj.irc_msgs)
		irc_process_proto_msg(str.c_str());
}

static void
netsplit(batch &obj)
{
	for (std::string &str : obj.irc_msgs)
		irc_process_proto_msg(str.c_str());
}

static void
znc_in_playback(batch &obj)
{
	for (std::string &str : obj.irc_msgs)
		irc_process_proto_msg(str.c_str());
	printtext_print("success", "%s: processed batch ref: %s", __func__,
	    obj.get_ref());
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
