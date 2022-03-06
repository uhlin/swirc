#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "network.h"
#include "commands/op.h"

static int
setup(void **state)
{
	net_send = net_send_fake;
	UNUSED_PARAM(state);
	return 0;
}

static void
canGiveOp_test1(void **state)
{
	cmd_op("companion");
	assert_string_equal(g_sent, "MODE #chatzone +o companion");
	UNUSED_PARAM(state);
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(canGiveOp_test1),
	};

	return cmocka_run_group_tests(tests, setup, NULL);
}
