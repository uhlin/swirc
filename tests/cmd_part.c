#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include <string.h>

#include "network.h"
#include "commands/jp.h"

static int
setup(void **state)
{
	net_send = net_send_fake;
	UNUSED_PARAM(state);
	return 0;
}

static void
sendsExpectedString_test1(void **state)
{
	cmd_part("#chatzone");
	assert_string_equal(g_sent, "PART #chatzone");
	UNUSED_PARAM(state);
}

static void
sendsExpectedString_test2(void **state)
{
	cmd_part("#chatzone adios!");
	assert_string_equal(g_sent, "PART #chatzone :adios!");
	UNUSED_PARAM(state);
}

static void
sendsExpectedString_test3(void **state)
{
	cmd_part("#chatzone good bye!");
	assert_string_equal(g_sent, "PART #chatzone :good bye!");
	UNUSED_PARAM(state);
}

static void
sendsExpectedString_test4(void **state)
{
	cmd_part("");
	assert_true(strncmp(g_sent, "PART #channel :", 15) == 0);
	UNUSED_PARAM(state);
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(sendsExpectedString_test1),
		cmocka_unit_test(sendsExpectedString_test2),
		cmocka_unit_test(sendsExpectedString_test3),
		cmocka_unit_test(sendsExpectedString_test4),
	};

	return cmocka_run_group_tests(tests, setup, NULL);
}
