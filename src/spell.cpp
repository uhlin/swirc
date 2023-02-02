/* spell.cpp
   Copyright (C) 2023 Markus Uhlin. All rights reserved.

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

#include <hunspell/hunspell.h>

#include <stdexcept>
#include <string>

#include "config.h"
#include "errHand.h"
#include "libUtils.h"
#include "main.h"
#include "nestHome.h"
#include "printtext.h"
#include "spell.h"
#include "strHand.h"
#ifdef UNIX
#include "swircpaths.h"
#endif

const char g_aff_suffix[] = ".aff";
const char g_dic_suffix[] = ".dic";

static Hunhandle *hh = nullptr;

suggestion::suggestion()
{
	this->word = nullptr;
	this->wide_word = nullptr;
}

suggestion::suggestion(const char *word)
{
	char			*lang, *encoding;
	std::string		 orig_locale("");
	std::string		 tmp_locale("");
	struct locale_info	*li;

	this->word = nullptr;
	this->wide_word = nullptr;

	if (word == nullptr || strings_match(word, ""))
		throw std::runtime_error("no word");

	li = get_locale_info(LC_CTYPE);
	if (li->lang_and_territory == nullptr || li->codeset == nullptr) {
		free_locale_info(li);
		throw std::runtime_error("get locale info error");
	} else if ((encoding = Hunspell_get_dic_encoding(hh)) == nullptr) {
		free_locale_info(li);
		throw std::runtime_error("get dic encoding error");
	}

	lang = li->lang_and_territory;
	(void) orig_locale.assign(lang).append(".").append(li->codeset);
	(void) tmp_locale.assign(lang).append(".").append(encoding);
	free_locale_info(li);

	mutex_lock(&g_puts_mutex);
	if (xsetlocale(LC_CTYPE, tmp_locale.c_str()) == nullptr)
		debug("temporary locale error");
	const size_t size = strlen(word) + 1;
	this->wide_word = static_cast<wchar_t *>(xcalloc(size,
	    sizeof(wchar_t)));
	if (xmbstowcs(this->wide_word, word, size) == g_conversion_failed)
		BZERO(this->wide_word, size);
	if (xsetlocale(LC_CTYPE, orig_locale.c_str()) == nullptr)
		debug("original locale error");
	mutex_unlock(&g_puts_mutex);
	this->word = sw_strdup(word);
}

suggestion::~suggestion()
{
	if (this->word) {
		free(this->word);
		this->word = nullptr;
	}
	if (this->wide_word) {
		free(this->wide_word);
		this->wide_word = nullptr;
	}
}

const char *
suggestion::get_word(void)
{
	return (this->word);
}

const wchar_t *
suggestion::get_wide_word(void)
{
	return (this->wide_word);
}

void
spell_init(void)
{
	std::string aff("");
	std::string dic("");

	if (config_bool("spell_syswide", true)) {
#if defined(UNIX)
		aff.append(HUNSPELL_PATH);
		dic.append(HUNSPELL_PATH);
#elif defined(WIN32)
		aff.append(g_home_dir);
		dic.append(g_home_dir);
#endif
	} else {
		aff.append(g_home_dir);
		dic.append(g_home_dir);
	}

	aff.append(SLASH).append(Config("spell_lang")).append(g_aff_suffix);
	dic.append(SLASH).append(Config("spell_lang")).append(g_dic_suffix);

	if (hh)
		Hunspell_destroy(hh);
	if ((hh = Hunspell_create(aff.c_str(), dic.c_str())) == nullptr)
		printtext_print("err", "%s: error", __func__);
}

void
spell_deinit(void)
{
	if (hh)
		Hunspell_destroy(hh);
	hh = nullptr;
}

bool
spell_word(const char *word)
{
	if (hh == nullptr)
		return false;
	return (Hunspell_spell(hh, word) != 0 ? true : false);
}

bool
spell_wide_word(const wchar_t *word)
{
	UNUSED_PARAM(word);
	return false;
}
