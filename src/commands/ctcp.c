#include "common.h"

#include "../dataClassify.h"
#include "../irc.h"
#include "../main.h"
#include "../printtext.h"
#include "../strHand.h"

#include "ctcp.h"
#include "i18n.h"

static void
query_time(CSTRING p_target)
{
	UNUSED_PARAM(p_target);
}

static void
query_userinfo(CSTRING p_target)
{
	UNUSED_PARAM(p_target);
}

static void
query_version(CSTRING p_target)
{
	UNUSED_PARAM(p_target);
}

/*
 * usage: /ctcp <query> <target>
 */
void
cmd_ctcp(CSTRING data)
{
	CSTRING			query, target;
	STRING			dcopy;
	STRING			last = "";
	static chararray_t	cmd = "/ctcp";
	static chararray_t	sep = "\n";

	if (strings_match(data, "")) {
		printtext_print("err", "%s", _("Insufficient arguments"));
		return;
	}

	dcopy = sw_strdup(data);
	(void) strFeed(dcopy, 1);

	if ((query = strtok_r(dcopy, sep, &last)) == NULL ||
	    (target = strtok_r(NULL, sep, &last)) == NULL) {
		printf_and_free(dcopy, _("%s: insufficient arguments"), cmd);
		return;
	} else if (!is_valid_nickname(target) && !is_irc_channel(target)) {
		printf_and_free(dcopy, _("%s: invalid target"), cmd);
		return;
	} else if (is_irc_channel(target) &&
		   strpbrk(target, g_forbidden_chan_name_chars) != NULL) {
		printf_and_free(dcopy,
		    _("%s: forbidden channel name characters"), cmd);
		return;
	}

	if (strings_match(query, "time"))
		query_time(target);
	else if (strings_match(query, "userinfo"))
		query_userinfo(target);
	else if (strings_match(query, "version"))
		query_version(target);
	else
		printtext_print("err", "%s", _("Invalid CTCP query"));
	free(dcopy);
}
