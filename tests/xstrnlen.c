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

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(nullString_test),
		cmocka_unit_test(noNullChar_test),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
