#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "strHand.h"

static void
nullString_test(void **state)
{
	size_t ret;

	ret = xstrnlen(NULL, SIZE_MAX);
	assert_true(ret == 0);
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(nullString_test),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
