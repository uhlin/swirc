#include "common.h"

#include <hunspell/hunspell.h>

#include <string>

#include "config.h"
#include "nestHome.h"
#include "printtext.h"
#include "spell.h"
#ifdef UNIX
#include "swircpaths.h"
#endif

const char g_aff_suffix[] = ".aff";
const char g_dic_suffix[] = ".dic";

static Hunhandle *hh = nullptr;

void
spell_init(void)
{
}

void
spell_deinit(void)
{
	if (hh)
		Hunspell_destroy(hh);
	hh = nullptr;
}
