#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "strHand.h"

static char	buf[7] = "foo";
static int	ret = 0;

static void
handlesInvalidArguments_test1(void **state)
{
	ret = sw_strcat(NULL, "bar", sizeof buf);
	assert_int_equal(ret, EINVAL);
	assert_string_equal(buf, "foo");
}

static void
handlesInvalidArguments_test2(void **state)
{
	ret = sw_strcat(buf, NULL, sizeof buf);
	assert_int_equal(ret, EINVAL);
	assert_string_equal(buf, "foo");
}

static void
handlesInvalidArguments_test3(void **state)
{
	ret = sw_strcat(buf, "bar", 0);
	assert_int_equal(ret, EINVAL);
	assert_string_equal(buf, "foo");
}

static void
doesBoundsChecking_test1(void **state)
{
	ret = sw_strcat(buf, "bar!", sizeof buf);
	assert_int_equal(ret, ERANGE);
	assert_string_equal(buf, "foo");
}

static void
doesBoundsChecking_test2(void **state)
{
	ret = sw_strcat(buf, "barbaz", sizeof buf);
	assert_int_equal(ret, ERANGE);
	assert_string_equal(buf, "foo");
}

static void
canConcatenate_test1(void **state)
{
	ret = sw_strcat(buf, "...", sizeof buf);
	assert_int_equal(ret, 0);
	assert_string_equal(buf, "foo...");
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(handlesInvalidArguments_test1),
		cmocka_unit_test(handlesInvalidArguments_test2),
		cmocka_unit_test(handlesInvalidArguments_test3),
		cmocka_unit_test(doesBoundsChecking_test1),
		cmocka_unit_test(doesBoundsChecking_test2),
		cmocka_unit_test(canConcatenate_test1),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
