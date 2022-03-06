#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "libUtils.h"

static void
returnsExpected_test1(void **state)
{
	if (setenv("USER", "maxxe", 1) != 0)
		fail();
	assert_string_equal(getuser(), "maxxe");
	UNUSED_PARAM(state);
}

static void
returnsExpected_test2(void **state)
{
	if (setenv("USER", "Markus Uhlin", 1) != 0)
		fail();
	assert_string_equal(getuser(), "Markus");
	UNUSED_PARAM(state);
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(returnsExpected_test1),
		cmocka_unit_test(returnsExpected_test2),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
