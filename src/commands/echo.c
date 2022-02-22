#include "common.h"

#include "../printtext.h"
#include "../strHand.h"

#include "echo.h"

/*
 * usage: /echo <text>
 */
void
cmd_echo(const char *data)
{
	PRINTTEXT_CONTEXT	ctx;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC1_FAILURE, true);

	if (strings_match(data, "")) {
		printtext(&ctx, "/echo: missing arguments");
		return;
	}

	ctx.spec_type = TYPE_SPEC1;
	printtext(&ctx, "%s", data);
}
