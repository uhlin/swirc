/* names-htbl-modify.cpp
   Copyright (C) 2022 Markus Uhlin. All rights reserved.

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

#include "../libUtils.h"
#include "../nicklist.h"
#include "../strHand.h"
#include "../window.h"

#include "names-htbl-modify.h"

#define hash(str) hash_djb_g(str, true, NAMES_HASH_TABLE_SIZE)

int
event_names_htbl_modify_owner(const char *nick, const char *channel,
    bool is_owner)
{
	PIRC_WINDOW	window;
	PNAMES		names;

	if (nick == NULL || strings_match(nick, "") ||
	    (window = window_by_label(channel)) == NULL)
		return ERR;

	for (names = window->names_hash[hash(nick)];
	    names != NULL;
	    names = names->next) {
		if (strings_match_ignore_case(nick, names->nick)) {
			if (names->is_owner && is_owner)
				return OK;
			else
				names->is_owner = is_owner;
			if (!names->is_owner) {
				window->num_owners--;

				if (names->is_superop)
					window->num_superops++;
				else if (names->is_op)
					window->num_ops++;
				else if (names->is_halfop)
					window->num_halfops++;
				else if (names->is_voice)
					window->num_voices++;
				else
					window->num_normal++;
			} else {
				/*
				 * not owner
				 */

				window->num_owners++;

				if (names->is_superop)
					window->num_superops--;
				else if (names->is_op)
					window->num_ops--;
				else if (names->is_halfop)
					window->num_halfops--;
				else if (names->is_voice)
					window->num_voices--;
				else
					window->num_normal--;
			}

			(void) nicklist_draw(window, LINES);
			return OK;
		}
	}

	return ERR;
}

int
event_names_htbl_modify_superop(const char *nick, const char *channel,
    bool is_superop)
{
	PIRC_WINDOW	window;
	PNAMES		names;

	if (nick == NULL || strings_match(nick, "") ||
	    (window = window_by_label(channel)) == NULL)
		return ERR;

	for (names = window->names_hash[hash(nick)];
	    names != NULL;
	    names = names->next) {
		if (strings_match_ignore_case(nick, names->nick)) {
			if (names->is_superop && is_superop)
				return OK;
			else
				names->is_superop = is_superop;

			if (names->is_owner) {
				return OK;
			} else if (!names->is_superop) {
				window->num_superops--;

				if (names->is_op)
					window->num_ops++;
				else if (names->is_halfop)
					window->num_halfops++;
				else if (names->is_voice)
					window->num_voices++;
				else
					window->num_normal++;
			} else {
				/*
				 * not superop
				 */

				window->num_superops++;

				if (names->is_op)
					window->num_ops--;
				else if (names->is_halfop)
					window->num_halfops--;
				else if (names->is_voice)
					window->num_voices--;
				else
					window->num_normal--;
			}

			(void) nicklist_draw(window, LINES);
			return OK;
		}
	}

	return ERR;
}

int
event_names_htbl_modify_op(const char *nick, const char *channel, bool is_op)
{
	PIRC_WINDOW	window;
	PNAMES		names;

	if (nick == NULL || strings_match(nick, "") ||
	    (window = window_by_label(channel)) == NULL)
		return ERR;

	for (names = window->names_hash[hash(nick)];
	    names != NULL;
	    names = names->next) {
		if (strings_match_ignore_case(nick, names->nick)) {
			if (names->is_op && is_op)
				return OK;
			else
				names->is_op = is_op;

			if (names->is_owner || names->is_superop) {
				return OK;
			} else if (!names->is_op) {
				window->num_ops--;

				if (names->is_halfop)
					window->num_halfops++;
				else if (names->is_voice)
					window->num_voices++;
				else
					window->num_normal++;
			} else {
				/*
				 * not op
				 */

				window->num_ops++;

				if (names->is_halfop)
					window->num_halfops--;
				else if (names->is_voice)
					window->num_voices--;
				else
					window->num_normal--;
			}

			(void) nicklist_draw(window, LINES);
			return OK;
		}
	}

	return ERR;
}

int
event_names_htbl_modify_halfop(const char *nick, const char *channel,
    bool is_halfop)
{
	PIRC_WINDOW	window;
	PNAMES		names;

	if (nick == NULL || strings_match(nick, "") ||
	    (window = window_by_label(channel)) == NULL)
		return ERR;

	for (names = window->names_hash[hash(nick)];
	    names != NULL;
	    names = names->next) {
		if (strings_match_ignore_case(nick, names->nick)) {
			if (names->is_halfop && is_halfop)
				return OK;
			else
				names->is_halfop = is_halfop;

			if (names->is_owner || names->is_superop ||
			    names->is_op) {
				return OK;
			} else if (!names->is_halfop) {
				window->num_halfops--;

				if (names->is_voice)
					window->num_voices++;
				else
					window->num_normal++;
			} else {
				/*
				 * not halfop
				 */

				window->num_halfops++;

				if (names->is_voice)
					window->num_voices--;
				else
					window->num_normal--;
			}

			(void) nicklist_draw(window, LINES);
			return OK;
		}
	}

	return ERR;
}

int
event_names_htbl_modify_voice(const char *nick, const char *channel,
    bool is_voice)
{
	PIRC_WINDOW	window;
	PNAMES		names;

	if (nick == NULL || strings_match(nick, "") ||
	    (window = window_by_label(channel)) == NULL)
		return ERR;

	for (names = window->names_hash[hash(nick)];
	    names != NULL;
	    names = names->next) {
		if (strings_match_ignore_case(nick, names->nick)) {
			if (names->is_voice && is_voice)
				return OK;
			else
				names->is_voice = is_voice;

			if (names->is_owner || names->is_superop ||
			    names->is_op || names->is_halfop) {
				return OK;
			} else if (!names->is_voice) {
				window->num_voices--;
				window->num_normal++;
			} else {
				/*
				 * not voice
				 */

				window->num_voices++;
				window->num_normal--;
			}

			(void) nicklist_draw(window, LINES);
			return OK;
		}
	}

	return ERR;
}
