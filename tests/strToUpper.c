#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "strHand.h"

static void
canUpperString_test1(void **state)
{
	char		str[] = "aBcDeFgHiJkLmNoPqRsTuVwXyZ";
	const char	expected[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	assert_string_not_equal(str, expected);
	assert_string_equal(strToUpper(str), expected);
	UNUSED_PARAM(state);
}

static void
leavesStringUnchanged_test1(void **state)
{
	char		str[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	const char	expected[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	assert_string_equal(strToUpper(str), expected);
	UNUSED_PARAM(state);
}

static void
leavesStringUnchanged_test2(void **state)
{
	char		str[] = "0123456789...";
	const char	expected[] = "0123456789...";

	assert_string_equal(strToUpper(str), expected);
	UNUSED_PARAM(state);
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(canUpperString_test1),
		cmocka_unit_test(leavesStringUnchanged_test1),
		cmocka_unit_test(leavesStringUnchanged_test2),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
