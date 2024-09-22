/* netsplit.cpp
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

#include "errHand.h"
#include "libUtils.h"
#include "netsplit.h"
#include "printtext.h"
#include "strHand.h"
#include "strdup_printf.h"
#include "theme.h"

netsplit::netsplit()
{
	this->channel.assign("");
	this->server[0].assign("*.net");
	this->server[1].assign("*.split");
	this->announced = false;
	this->secs[0] = g_time_error;
	this->secs[1] = g_time_error;
}

netsplit::netsplit(const struct netsplit_context *ctx, CSTRING nick)
{
	this->channel.assign(ctx->chan);
	this->server[0].assign(ctx->serv1);
	this->server[1].assign(ctx->serv2);
	this->nicks.push_back(nick);
	this->announced = false;
	this->secs[0] = time(nullptr);
	this->secs[1] = g_time_error;
}

void
netsplit::announce_netjoin(void) const
{
	CSTRING			host[2];
	PRINTTEXT_CONTEXT	ctx;
	immutable_cp_t		chan = this->channel.c_str();
	uintmax_t		array[3];

	host[0] = this->server[0].c_str();
	host[1] = this->server[1].c_str();

	printtext_context_init(&ctx, nullptr, TYPE_SPEC1_SUCCESS, true);

	array[0] = static_cast<uintmax_t>(this->nicks.size());
	array[1] = static_cast<uintmax_t>(this->rem_nicks.size());
	array[2] = array[0] + array[1];

	if ((ctx.window = window_by_label(chan)) == nullptr)
		ctx.window = g_status_window;
	printtext(&ctx, "%sNetJoin%s %s%s%s %s %s %s (%ju users returned "
	    "out of %ju)",
	    COLOR3, TXT_NORMAL,
	    LEFT_BRKT, chan, RIGHT_BRKT,
	    host[0], THE_SPEC2, host[1],
	    array[1], array[2]);
}

void
netsplit::announce_split(void)
{
	CSTRING			host[2];
	PRINTTEXT_CONTEXT	ctx;
	immutable_cp_t		chan = this->channel.c_str();
	uintmax_t		no_nicks;

	host[0] = this->server[0].c_str();
	host[1] = this->server[1].c_str();

	printtext_context_init(&ctx, nullptr, TYPE_SPEC1_WARN, true);

	no_nicks = static_cast<uintmax_t>(this->nicks.size());

	if ((ctx.window = window_by_label(chan)) == nullptr)
		ctx.window = g_status_window;
	printtext(&ctx, "%sNetSplit%s %s%s%s %s %s %s (%ju nicks)",
	    COLOR3, TXT_NORMAL,
	    LEFT_BRKT, chan, RIGHT_BRKT,
	    host[0], THE_SPEC2, host[1],
	    no_nicks);
	this->announced = true;
}

bool
netsplit::find_nick(CSTRING p_nick)
{
	std::vector<std::string>::iterator it;

	for (it = this->nicks.begin(); it != this->nicks.end(); ++it) {
		if (strings_match_ignore_case(it->c_str(), p_nick))
			return true;
	}

	return false;
}

bool
netsplit::remove_nick(CSTRING p_nick)
{
	std::vector<std::string>::iterator	it = this->nicks.begin();
	std::vector<std::string>::size_type	count = 0;

	while (it != this->nicks.end()) {
		if (strings_match_ignore_case(it->c_str(), p_nick)) {
			// Saved the removed nick
			const std::string str(it->c_str());
#if defined(__cplusplus) && __cplusplus >= 201103L
			this->rem_nicks.emplace_back(str);
#else
			this->rem_nicks.push_back(str);
#endif

			it = this->nicks.erase(it);
			count++;
		} else
			++it;
	}

	if (count > 1) {
		err_log(0, "%s: count=%ju (%s)", __func__,
		    static_cast<uintmax_t>(count), p_nick);
	}
	return (count > 0 ? true : false);
}

/*
 * The Netsplit database
 */
static std::vector<netsplit *> netsplit_db;

/*
 * Creates a new Netsplit
 */
int
netsplit_create(const struct netsplit_context *ctx, CSTRING nick)
{
	try {
		netsplit_db.push_back(new netsplit(ctx, nick));
	} catch (const std::bad_alloc &e) {
		err_exit(ENOMEM, "%s: fatal: %s", __func__, e.what());
	} catch (...) {
		return ERR;
	}
	return OK;
}

bool
netsplit_db_empty(void)
{
	return (netsplit_db.empty());
}

/*
 * Destroys all Netsplits stored in the database
 */
void
netsplit_destroy_all(void)
{
	std::vector<netsplit *>::iterator it;

	if (netsplit_db.empty())
		return;
	for (it = netsplit_db.begin(); it != netsplit_db.end(); ++it)
		delete *it;
	netsplit_db.clear();
}

netsplit *
netsplit_find(CSTRING nick, CSTRING channel)
{
	std::vector<netsplit *>::iterator it;

	for (it = netsplit_db.begin(); it != netsplit_db.end(); ++it) {
		immutable_cp_t db_chan = (*it)->channel.c_str();

		if (strings_match_ignore_case(db_chan, channel) &&
		    (*it)->find_nick(nick))
			return (*it);
	}

	return nullptr;
}

const std::vector<netsplit *> &
netsplit_get_db(void)
{
	return (netsplit_db);
}

netsplit *
netsplit_get_split(const struct netsplit_context *ctx)
{
	std::vector<netsplit *>::iterator it;

	for (it = netsplit_db.begin(); it != netsplit_db.end(); ++it) {
		CSTRING		db_chan = (*it)->channel.c_str();
		CSTRING		serv1, serv2;

		serv1 = (*it)->server[0].c_str();
		serv2 = (*it)->server[1].c_str();

		if (strings_match_ignore_case(db_chan, ctx->chan) &&
		    strings_match_ignore_case(serv1, ctx->serv1) &&
		    strings_match_ignore_case(serv2, ctx->serv2))
			return (*it);
	}

	return nullptr;
}

static STRING
get_removal_msg(const struct netsplit_context *ctx, const intmax_t (&array)[2])
{
	STRING	msg;

	msg = strdup_printf("%s%s%s: %s %s %s - "
	    "removing old netsplit from memory, after %jd seconds "
	    "(elapsed %jd) ...",
	    LEFT_BRKT, ctx->chan, RIGHT_BRKT,
	    ctx->serv1, THE_SPEC2, ctx->serv2,
	    array[0], array[1]);
	return (msg);
}

void
netsplit_run_bkgd_task(void)
{
	PRINTTEXT_CONTEXT			ptext_ctx;
	static const time_t			keep_split_secs = 900;
	static const time_t			secs_split_stop = 5;
	std::vector<netsplit *>::iterator	it;

	if (netsplit_db.empty())
		return;

	printtext_context_init(&ptext_ctx, g_status_window, TYPE_SPEC1_FAILURE,
	    true);
	const time_t now = time(nullptr);
	it = netsplit_db.begin();

	while (it != netsplit_db.end()) {
		immutable_cp_t	chan = (*it)->channel.c_str();
		time_t		diff;

		if (!(*it)->join_begun()) {
			if ((*it)->get_split_time() > now) {
				printtext(&ptext_ctx, "%s: %s: unexpected "
				    "split time (too large)", __func__, chan);
				delete *it;
				it = netsplit_db.erase(it);
				continue;
			}

			diff = now - (*it)->get_split_time();

			if (diff > keep_split_secs) {
				intmax_t			array[2];
				STRING				msg;
				struct netsplit_context		ns_ctx(chan,
				    (*it)->server[0].c_str(),
				    (*it)->server[1].c_str());

				array[0] = static_cast<intmax_t>(keep_split_secs);
				array[1] = static_cast<intmax_t>(diff);

				msg = get_removal_msg(&ns_ctx, array);

				ptext_ctx.spec_type = TYPE_SPEC1_WARN;
				printtext(&ptext_ctx, "%s", msg);
				free(msg);

				delete *it;
				it = netsplit_db.erase(it);
				continue;
			} else if (diff >= secs_split_stop &&
				   !(*it)->has_announced_split())
				(*it)->announce_split();
			++it;
		} else {
			/*
			 * join begun...
			 */
			if ((*it)->get_join_time() > now) {
				printtext(&ptext_ctx, "%s: %s: unexpected "
				    "join time (too large)", __func__, chan);
				delete *it;
				it = netsplit_db.erase(it);
				continue;
			}

			diff = now - (*it)->get_join_time();

			if (diff >= secs_split_stop) {
				(*it)->announce_netjoin();
				delete *it;
				it = netsplit_db.erase(it);
				continue;
			} else
				++it;
		}
	}
}

/* ----------------------------------------------------------------- */

void
netsplit_init(void)
{
	/* null */;
}

void
netsplit_deinit(void)
{
	netsplit_destroy_all();
}
