#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "strHand.h"

static size_t ret;

static void
nullString_test(void **state)
{
	ret = xstrnlen(NULL, SIZE_MAX);
	assert_true(ret == 0);
}

static void
noNullChar_test(void **state)
{
	char buf[10];

	memset(buf, 'Z', sizeof buf);
	ret = xstrnlen(buf, sizeof buf);
	assert_true(ret == sizeof buf);
}

static void
returnsCorrectLength_test1(void **state)
{
	const char str[] = "swirc";

	ret = xstrnlen(str, sizeof str);
	assert_true(ret == 5);
	assert_true(ret == strlen(str));
}

static void
returnsCorrectLength_test2(void **state)
{
	const char str[] = "swirc is nifty";

	ret = xstrnlen(str, sizeof str);
	assert_true(ret == 14);
	assert_true(ret == strlen(str));
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(nullString_test),
		cmocka_unit_test(noNullChar_test),
		cmocka_unit_test(returnsCorrectLength_test1),
		cmocka_unit_test(returnsCorrectLength_test2),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
