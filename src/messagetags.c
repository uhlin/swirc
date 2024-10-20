/* messagetags.c
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

#include "irc.h"
#include "events/batch.h"

#include "assertAPI.h"
#include "i18n.h"
#include "libUtils.h"
#include "messagetags.h"
#include "printtext.h"
#include "strHand.h"
#include "strdup_printf.h"

static struct tag_list_ {
	CSTRING		name;
	const size_t	len;
} tag_list[] = {
	{ "account=", 8 },
	{ "batch=", 6 },
	{ "time=", 5 },
};

static void
set_var(CSTRING name, CSTRING value, struct messagetags *tags)
{
	if (strings_match(name, "account="))
		tags->account = sw_strdup(value);
	else if (strings_match(name, "batch="))
		tags->batch = sw_strdup(value);
	else if (strings_match(name, "time="))
		tags->srv_time = sw_strdup(value);
	else
		sw_assert_not_reached();
}

struct messagetags *
msgtags_get(CSTRING str)
{
	struct messagetags *tags = xcalloc(sizeof *tags, 1);

	tags->account =
	tags->batch =
	tags->srv_time = NULL;

	for (struct tag_list_ *ptr = addrof(tag_list[0]);
	    ptr < &tag_list[ARRAY_SIZE(tag_list)];
	    ptr++) {
		const char *cp;

		if ((cp = strstr(str, ptr->name)) != NULL) {
			char tmp[250] = { '\0' };

			cp += ptr->len;
			const size_t num = strcspn(cp, ";");
			if (num == 0 || num >= ARRAY_SIZE(tmp))
				continue;
			memcpy(tmp, cp, num);
			set_var(ptr->name, addrof(tmp[0]), tags);
		}
	}

	return tags;
}

void
msgtags_free(struct messagetags *tags)
{
	if (tags) {
		free(tags->account);
		free(tags->batch);
		free(tags->srv_time);
		free(tags);
	}
}

void
msgtags_handle_batch(CSTRING pmsg, const struct messagetags *tags)
{
	STRING	str;

	if (tags->batch == NULL)
		return;
	if (tags->account != NULL && tags->srv_time != NULL)
		str = strdup_printf("@account=%s;time=%s %s",
		    tags->account, tags->srv_time, pmsg);
	else if (tags->account != NULL)
		str = strdup_printf("@account=%s %s", tags->account, pmsg);
	else if (tags->srv_time != NULL)
		str = strdup_printf("@time=%s %s", tags->srv_time, pmsg);
	else
		str = sw_strdup(pmsg);
	event_batch_add_irc_msgs(tags->batch, str);
	free(str);
}

static bool
no_change(CSTRING str1, CSTRING str2)
{
	if (str1 == NULL || str2 == NULL)
		return false;
	return (strcmp(str1, str2) == 0);
}

void
msgtags_process(struct irc_message_compo *compo, struct messagetags *tags)
{
	if (tags->account && !no_change(compo->account, tags->account)) {
		free(compo->account);
		compo->account = sw_strdup(tags->account);
	}
	if (tags->srv_time) {
		if (sscanf(tags->srv_time, "%d-%d-%dT%d:%d:%d.%dZ",
		    & (compo->year),
		    & (compo->month),
		    & (compo->day),
		    & (compo->hour),
		    & (compo->minute),
		    & (compo->second),
		    & (compo->precision)) != 7) {
			printtext_print("err", _("%s: server time error"),
			    __func__);
		}
	}
}
