#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "network.h"
#include "commands/ban.h"

static int
setup(void **state)
{
	net_send = net_send_fake;
	UNUSED_PARAM(state);
	return 0;
}

static void
canBan_test1(void **state)
{
	cmd_ban("*hitler*!*@*");
	assert_string_equal(g_sent, "MODE #chatzone +b *hitler*!*@*");
	UNUSED_PARAM(state);
}

static void
canBan_test2(void **state)
{
	cmd_ban("*!*@spammers.com");
	assert_string_equal(g_sent, "MODE #chatzone +b *!*@spammers.com");
	UNUSED_PARAM(state);
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(canBan_test1),
		cmocka_unit_test(canBan_test2),
	};

	return cmocka_run_group_tests(tests, setup, NULL);
}
