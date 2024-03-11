/* spell.cpp
   Copyright (C) 2023-2024 Markus Uhlin. All rights reserved.

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

#ifdef HAVE_HUNSPELL
#include <hunspell/hunspell.h>

#include <climits>
#include <clocale>
#include <cwchar>
#include <cwctype>
#include <stdexcept>
#include <string>

#include "commands/fetchdic.h"

#include "assertAPI.h"
#include "config.h"
#include "errHand.h"
#include "filePred.h"
#include "libUtils.h"
#include "main.h"
#include "nestHome.h"
#include "printtext.h"
#include "spell.h"
#include "strHand.h"
#ifdef UNIX
#include "swircpaths.h"
#endif
#endif // HAVE_HUNSPELL

#define MAXWORDLEN 50

bool g_suggs_mode = false;

#ifdef HAVE_HUNSPELL
static Hunhandle			*pHunspell = nullptr;
static std::vector<sugg_ptr>		*rl_suggs = nullptr;
static std::vector<sugg_ptr>::iterator	 suggs_it;
static std::wstring			 rl_word;

suggestion::suggestion()
{
	this->word = nullptr;
	this->wide_word = nullptr;
}

suggestion::suggestion(CSTRING p_word)
{
	const size_t size = strlen(p_word) + 1;

	this->word = new char[size];
	memcpy(this->word, p_word, size);

	this->wide_word = new wchar_t[size];
	if (xmbstowcs(this->wide_word, p_word, size - 1) == g_conversion_failed)
		this->wide_word[0] = L'\0';
	this->wide_word[size - 1] = L'\0';
}

suggestion::~suggestion()
{
	if (this->word != nullptr) {
		delete[] this->word;
		this->word = nullptr;
	}
	if (this->wide_word != nullptr) {
		delete[] this->wide_word;
		this->wide_word = nullptr;
	}
}

CSTRING
suggestion::get_word(void) const
{
	return (this->word);
}

const wchar_t *
suggestion::get_wide_word(void) const
{
	return (this->wide_word);
}

void
spell_init(bool report_nonexistent)
{
	std::string	 aff("");
	std::string	 dic("");

	if (config_bool("spell_syswide", true)) {
#if defined(UNIX)
		aff.append(HUNSPELL_PATH);
		dic.append(HUNSPELL_PATH);
#elif defined(WIN32)
		aff.append(g_progpath ? g_progpath : g_home_dir);
		dic.append(g_progpath ? g_progpath : g_home_dir);
#endif
	} else {
		aff.append(g_home_dir);
		dic.append(g_home_dir);
	}

	aff.append(SLASH).append(Config("spell_lang")).append(g_aff_suffix);
	dic.append(SLASH).append(Config("spell_lang")).append(g_dic_suffix);

	if (report_nonexistent) {
		if (!is_regular_file(aff.c_str())) {
			printtext_print("err", "%s: %s not found", __func__,
			    aff.c_str());
		}
		if (!is_regular_file(dic.c_str())) {
			printtext_print("err", "%s: %s not found", __func__,
			    dic.c_str());
		}
	}

	if (pHunspell)
		Hunspell_destroy(pHunspell);

	redir_stderr();
	if ((pHunspell = Hunspell_create(aff.c_str(), dic.c_str())) == nullptr)
		printtext_print("err", "%s: error", __func__);
	restore_stderr();
}

void
spell_deinit(void)
{
	g_suggs_mode = false;

	if (pHunspell)
		Hunspell_destroy(pHunspell);
	pHunspell = nullptr;
}

void
spell_destroy_suggs(std::vector<sugg_ptr> *suggs)
{
	if (suggs) {
		std::vector<sugg_ptr>::iterator it;

		for (it = suggs->begin(); it != suggs->end(); ++it)
			delete *it;

		delete suggs;
	}
}

//lint -sem(get_mbs, r_null)
static STRING
get_mbs(const wchar_t *wcs)
{
	STRING	 out;
	size_t	 bytes_convert, size;

	size		= size_product(wcslen(wcs) + 1, MB_LEN_MAX);
	out		= static_cast<STRING>(xmalloc(size));
	bytes_convert	= wcstombs(out, wcs, size - 1);

	if (bytes_convert == g_conversion_failed) {
		free(out);
		return nullptr;
	}

	out[bytes_convert] = '\0';
	return out;
}

std::vector<sugg_ptr> *
spell_get_suggs(CSTRING mbs, const wchar_t *wcs)
{
	char**			 list;
	int			 nsuggs;
	std::vector<sugg_ptr>	*suggs;
	sugg_ptr		 ptr;

	if (mbs == nullptr && wcs == nullptr)
		return nullptr;
	else if (mbs) {
		if (strcmp(mbs, "") == STRINGS_MATCH ||
		    (nsuggs = Hunspell_suggest(pHunspell, &list, mbs)) < 1)
			return nullptr;
	} else if (wcs) {
		STRING word;

		if (wcscmp(wcs, L"") == STRINGS_MATCH ||
		    wcslen(wcs) > MAXWORDLEN)
			return nullptr;

		if ((word = get_mbs(wcs)) == nullptr ||
		    (nsuggs = Hunspell_suggest(pHunspell, &list, word)) < 1) {
			free(word);
			return nullptr;
		}

		free(word);
	} else
		sw_assert_not_reached();

	suggs = new std::vector<sugg_ptr>();

	for (int i = 0; i < nsuggs; i++) {
		try {
			ptr = new suggestion(list[i]);
			suggs->push_back(ptr);
		} catch (const std::bad_alloc &e) {
			err_exit(ENOMEM, "%s: fatal: %s", __func__, e.what());
		} catch (const std::runtime_error &e) {
			delete ptr;
			debug("%s: %s", __func__, e.what());
		} catch (...) {
			delete ptr;
			debug("%s: unknown error", __func__);
		}
	}

	Hunspell_free_list(pHunspell, &list, nsuggs);
	return suggs;
}

void
spell_test1(CSTRING word)
{
	std::vector<sugg_ptr>			*suggs;
	std::vector<sugg_ptr>::iterator		 it;

	printtext_print(nullptr, " -- %s is %s", word, (spell_word(word) ?
	    "correct" : "incorrect"));

	if ((suggs = spell_get_suggs(word, nullptr)) != nullptr) {
		for (it = suggs->begin(); it != suggs->end(); ++it)
			printtext_print(nullptr, "%s", (*it)->get_word());
		spell_destroy_suggs(suggs);
	}
}

void
spell_test2(const wchar_t *word)
{
	std::vector<sugg_ptr>			*suggs;
	std::vector<sugg_ptr>::iterator		 it;

	printtext_print(nullptr, " -- %ls is %s", word, (spell_wide_word(word) ?
	    "correct" : "incorrect"));

	if ((suggs = spell_get_suggs(nullptr, word)) != nullptr) {
		for (it = suggs->begin(); it != suggs->end(); ++it)
			printtext_print(nullptr, "%ls", (*it)->get_wide_word());
		spell_destroy_suggs(suggs);
	}
}

bool
spell_word(CSTRING word)
{
	if (pHunspell == nullptr || word == nullptr ||
	    strcmp(word, "") == STRINGS_MATCH)
		return false;
	return (Hunspell_spell(pHunspell, word) != 0 ? true : false);
}

static void
erase_word(volatile struct readline_session_context *ctx, const size_t len)
{
	for (size_t i = 0; i < len; i++)
		readline_handle_backspace(ctx);
}

static void
type_word(volatile struct readline_session_context *ctx,
    const std::wstring &word)
{
	for (size_t i = 0; i < word.size(); i++)
		readline_handle_key_exported(ctx, word.at(i));
}

static void
auto_complete_next_sugg(volatile struct readline_session_context *ctx)
{
	std::wstring	 word(L"");
	sugg_ptr	 ptr;

	if (suggs_it == rl_suggs->end()) {
		printtext_print("warn", "no more suggestions");
		g_suggs_mode = false;
		spell_destroy_suggs(rl_suggs);
		rl_suggs = nullptr;
		return;
	}

	if (!rl_word.empty())
		word.assign(rl_word);
	else {
		ptr = *(suggs_it - 1);
		word.assign(ptr->get_wide_word());
	}

	erase_word(ctx, word.size());
	if (!rl_word.empty())
		rl_word.assign(L"");

	ptr = *suggs_it;
	word.assign(ptr->get_wide_word());
	type_word(ctx, word);
	++suggs_it;
}

static void
print_suggestions(std::vector<sugg_ptr> *suggs)
{
	std::vector<sugg_ptr>::iterator it;

	printtext_print(nullptr, "suggestions:");

	for (it = suggs->begin(); it != suggs->end(); ++it)
		printtext_print(nullptr, "  %ls", (*it)->get_wide_word());
}

void
spell_word_readline(volatile struct readline_session_context *ctx)
{
	if (!config_bool("spell", true) ||
	    ctx->numins > int_diff(g_readline_bufsize, MAXWORDLEN * 3))
		return;

	if (!g_suggs_mode) {
		int pos;

		if (ctx->bufpos < 1)
			return;
		pos = ctx->bufpos;

		if ((ctx->buffer[pos] != L'\0' && ctx->buffer[pos] != L' ') ||
		    !iswalpha(ctx->buffer[pos - 1]))
			return;
		while (pos != 0 && iswalpha(ctx->buffer[pos - 1]))
			pos--;
		rl_word.assign(&ctx->buffer[pos], ctx->bufpos - pos);

		if (spell_wide_word(rl_word.c_str())) {
			printtext_print("success", "%ls is correct",
			    rl_word.c_str());
			return;
		}

		printtext_print("err", "%ls is incorrect", rl_word.c_str());

		if (rl_suggs)
			spell_destroy_suggs(rl_suggs);
		if ((rl_suggs = spell_get_suggs(nullptr, rl_word.c_str())) ==
		    nullptr)
			return;
		print_suggestions(rl_suggs);
		g_suggs_mode = true;
		suggs_it = rl_suggs->begin();
		return;
	}

	if (rl_suggs)
		auto_complete_next_sugg(ctx);
	else
		err_log(EINVAL, "%s: readline suggestions null", __func__);
}

bool
spell_wide_word(const wchar_t *word)
{
	STRING	mbs;
	bool	ret;

	if (pHunspell == nullptr || word == nullptr ||
	    wcscmp(word, L"") == STRINGS_MATCH ||
	    wcslen(word) > MAXWORDLEN)
		return false;

	if ((mbs = get_mbs(word)) == nullptr)
		return false;

	ret = (Hunspell_spell(pHunspell, mbs) != 0 ? true : false);
	free(mbs);

	return ret;
}
#else
#pragma message("Consider installing Hunspell")
#endif // HAVE_HUNSPELL
