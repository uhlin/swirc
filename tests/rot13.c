#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "libUtils.h"

static void	rot13_byte_test1(void **);
static void	rot13_byte_test2(void **);
static void	rot13_byte_test3(void **);

static void	rot13_str_test1(void **);
static void	rot13_str_test2(void **);
static void	rot13_str_test3(void **);

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(rot13_byte_test1),
		cmocka_unit_test(rot13_byte_test2),
		cmocka_unit_test(rot13_byte_test3),
		cmocka_unit_test(rot13_str_test1),
		cmocka_unit_test(rot13_str_test2),
		cmocka_unit_test(rot13_str_test3),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}

static void
rot13_byte_test1(void **state)
{
	assert_true(rot13_byte('C') == 'P');
	UNUSED_PARAM(state);
}

static void
rot13_byte_test2(void **state)
{
	assert_true(rot13_byte('X') == 'K');
	UNUSED_PARAM(state);
}

static void
rot13_byte_test3(void **state)
{
	assert_true(rot13_byte('a') == 'n');
	assert_true(rot13_byte('f') == 's');
	UNUSED_PARAM(state);
}

static void
rot13_str_test1(void **state)
{
	char str[] = "Hello 123 World";

	assert_string_equal(rot13_str(str), "Uryyb 123 Jbeyq");
	assert_string_equal(rot13_str(str), "Hello 123 World");
	UNUSED_PARAM(state);
}

static void
rot13_str_test2(void **state)
{
	char str1[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char str2[] = "abcdefghijklmnopqrstuvwxyz";

	assert_string_equal(rot13_str(str1), "NOPQRSTUVWXYZABCDEFGHIJKLM");
	assert_string_equal(rot13_str(str1), "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

	assert_string_equal(rot13_str(str2), "nopqrstuvwxyzabcdefghijklm");
	assert_string_equal(rot13_str(str2), "abcdefghijklmnopqrstuvwxyz");
	UNUSED_PARAM(state);
}

static void
rot13_str_test3(void **state)
{
	char str[] = "456 XYZ abc (){}";

	assert_string_equal(rot13_str(str), "456 KLM nop (){}");
	assert_string_equal(rot13_str(str), "456 XYZ abc (){}");
	UNUSED_PARAM(state);
}
