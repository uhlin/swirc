#include "common.h"

#include <hunspell/hunspell.hxx>

#include "config.h"
#include "nestHome.h"
#include "spell.h"

const char g_aff_suffix[] = ".aff";
const char g_dic_suffix[] = ".dic";

static Hunspell *hs = nullptr;

void
spell_init(void)
{
}

void
spell_deinit(void)
{
	delete hs;
	hs = nullptr;
}
