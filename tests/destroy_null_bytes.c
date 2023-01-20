#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "network.h"

static void
canDestroyNullBytes_test1(void **state)
{
	char buf[] = { 'a',0,0,0,'b',0,0,0,'c',0,0,0 };
	const char expected[] = {
		'a','X','X','X',
		'b','X','X','X',
		'c',0,0,0
	};

	assert_true(sizeof buf == sizeof expected);
	destroy_null_bytes_exported(buf, sizeof buf);
	assert_memory_equal(buf, expected, sizeof buf);
	UNUSED_PARAM(state);
}

static void
canDestroyNullBytes_test2(void **state)
{
	char buf[] = { 0,'A',0,'B',0,'C',0 };
	const char expected[] = { 'X','A','X','B','X','C',0 };

	assert_true(sizeof buf == sizeof expected);
	destroy_null_bytes_exported(buf, sizeof buf);
	assert_memory_equal(buf, expected, sizeof buf);
	UNUSED_PARAM(state);
}

static void
canDestroyNullBytes_test3(void **state)
{
	char buf[] = {
		0,0,'a','a',
		0,0,'b','b',
		0,0,'c','c',
		0,0
	};
	const char expected[] = {
		'X','X','a','a',
		'X','X','b','b',
		'X','X','c','c',
		0,0
	};

	assert_true(sizeof buf == sizeof expected);
	destroy_null_bytes_exported(buf, sizeof buf);
	assert_memory_equal(buf, expected, sizeof buf);
	UNUSED_PARAM(state);
}

static void
leavesBufUnchanged_test1(void **state)
{
	char buf[] = { 0,0 };
	const char expected[] = { 0,0 };

	assert_true(sizeof buf == 2);
	assert_true(sizeof buf == sizeof expected);
	destroy_null_bytes_exported(buf, sizeof buf);
	assert_memory_equal(buf, expected, sizeof buf);
	UNUSED_PARAM(state);
}

static void
leavesBufUnchanged_test2(void **state)
{
	char buf[] = { 0 };
	const char expected[] = { 0 };

	assert_true(sizeof buf == 1);
	assert_true(sizeof buf == sizeof expected);
	destroy_null_bytes_exported(buf, sizeof buf);
	assert_memory_equal(buf, expected, sizeof buf);
	UNUSED_PARAM(state);
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(canDestroyNullBytes_test1),
		cmocka_unit_test(canDestroyNullBytes_test2),
		cmocka_unit_test(canDestroyNullBytes_test3),
		cmocka_unit_test(leavesBufUnchanged_test1),
		cmocka_unit_test(leavesBufUnchanged_test2),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
