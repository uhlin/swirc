#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "icb.h"
#include "network.h"

static int
setup(void **state)
{
	(void) state;
	net_send = net_send_fake;
	return 0;
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(icb_send_pm_test1),
		cmocka_unit_test(icb_send_pm_test2),
	};

	return cmocka_run_group_tests(tests, setup, NULL);
}
