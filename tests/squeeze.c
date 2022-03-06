#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "strHand.h"

static void
canSqueezeCharacters_test1(void **state)
{
	char		 buf[] = "ABCABCABCABCABCABCABCABCABCABC";
	const char	*expected = "ABABABABABABABABABAB";

	assert_string_not_equal(buf, expected);
	squeeze(buf, "C");
	assert_string_equal(buf, expected);
	UNUSED_PARAM(state);
}

static void
canSqueezeCharacters_test2(void **state)
{
	char		 buf[] = "ABCABCABCABCABCABCABCABCABCABC";
	const char	*expected = "AAAAAAAAAA";

	assert_string_not_equal(buf, expected);
	squeeze(buf, "BC");
	assert_string_equal(buf, expected);
	UNUSED_PARAM(state);
}

static void
canSqueezeCharacters_test3(void **state)
{
	char		 buf[] = "ABCABCABCABCABCABCABCABCABCABC";
	const char	*expected = "";

	assert_string_not_equal(buf, expected);
	squeeze(buf, "ABC");
	assert_string_equal(buf, expected);
	UNUSED_PARAM(state);
}

static void
canSqueezeCharacters_test4(void **state)
{
	char		 buf[] = "ZzZzZzZzZzZzZzZzZzZz";
	const char	*expected = "zzzzzzzzzz";

	assert_string_not_equal(buf, expected);
	squeeze(buf, "Z");
	assert_string_equal(buf, expected);
	UNUSED_PARAM(state);
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(canSqueezeCharacters_test1),
		cmocka_unit_test(canSqueezeCharacters_test2),
		cmocka_unit_test(canSqueezeCharacters_test3),
		cmocka_unit_test(canSqueezeCharacters_test4),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
