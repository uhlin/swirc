#include "common.h"

#include <setjmp.h>
#ifdef UNIT_TESTING
#undef UNIT_TESTING
#endif
#include <cmocka.h>

#include "dataClassify.h"

int expected = -1;

static void
test1(void **state)
{
	const wchar_t str[] = L"Ӽe̲̅v̲̅o̲̅l̲̅u̲̅t̲̅i̲̅o̲̅ɳ̲̅ᕗ";

	expected = 11;
	assert_true(xwcswidth(str) == expected);
	UNUSED_PARAM(state);
}

static void
test2(void **state)
{
	const wchar_t str[] = L"你a好b世c界";

	expected = 11;
	assert_true(xwcswidth(str) == expected);
	UNUSED_PARAM(state);
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test1),
		cmocka_unit_test(test2),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
