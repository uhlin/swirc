#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "dataClassify.h"

static void
returnsFalseOnNullInput(void **state)
{
	assert_false(is_numeric(NULL));
	UNUSED_PARAM(state);
}

static void
returnsFalseOnEmptyInput(void **state)
{
	assert_false(is_numeric(""));
	UNUSED_PARAM(state);
}

static void
returnsTrue_test1(void **state)
{
	assert_true(is_numeric("0123456789"));
	UNUSED_PARAM(state);
}

static void
returnsFalse_test1(void **state)
{
	assert_false(is_numeric("123 456"));
	UNUSED_PARAM(state);
}

static void
returnsFalse_test2(void **state)
{
	assert_false(is_numeric("123abc"));
	UNUSED_PARAM(state);
}

static void
returnsFalse_test3(void **state)
{
	assert_false(is_numeric("789 "));
	UNUSED_PARAM(state);
}

static void
returnsFalse_test4(void **state)
{
	assert_false(is_numeric(" 789"));
	UNUSED_PARAM(state);
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(returnsFalseOnNullInput),
		cmocka_unit_test(returnsFalseOnEmptyInput),
		cmocka_unit_test(returnsTrue_test1),
		cmocka_unit_test(returnsFalse_test1),
		cmocka_unit_test(returnsFalse_test2),
		cmocka_unit_test(returnsFalse_test3),
		cmocka_unit_test(returnsFalse_test4),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
