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
	UNUSED_PARAM(word);
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
