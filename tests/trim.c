#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "strHand.h"

static void
canTrim_test1(void **state)
{
	char str[] = "          ";

	assert_string_equal(trim(str), "");
}

static void
canTrim_test2(void **state)
{
	char str[] = " \f\n\r\t\v";

	assert_string_equal(trim(str), "");
}

static void
canTrim_test3(void **state)
{
	char		 str[] = "Swirc IRC client          ";
	const char	*expected = "Swirc IRC client";

	assert_string_equal(trim(str), expected);
}

static void
canTrim_test4(void **state)
{
	char		 str[] = "          Hello World!          ";
	const char	*expected = "          Hello World!";

	assert_string_equal(trim(str), expected);
}

static void
leavesStringUnchanged_test1(void **state)
{
	char		 str[] = "          Diablo";
	const char	*expected = "          Diablo";

	assert_string_equal(trim(str), expected);
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(canTrim_test1),
		cmocka_unit_test(canTrim_test2),
		cmocka_unit_test(canTrim_test3),
		cmocka_unit_test(canTrim_test4),
		cmocka_unit_test(leavesStringUnchanged_test1),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
