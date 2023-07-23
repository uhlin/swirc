#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "printtext.h"

static wchar_t buf[300] = { L'\0' };

static void
canSqueezeTextDecoration_test1(void **state)
{
	swprintf(buf, ARRAY_SIZE(buf), L"%c1,2,3,test", COLOR);
	squeeze_text_deco_wide(buf);
	assert_true(wcscmp(buf, L",3,test") == 0);
	UNUSED_PARAM(state);
}

static void
canSqueezeTextDecoration_test2(void **state)
{
	swprintf(buf, ARRAY_SIZE(buf), L"%c1,234", COLOR);
	squeeze_text_deco_wide(buf);
	assert_true(wcscmp(buf, L"4") == 0);
	UNUSED_PARAM(state);
}

static void
canSqueezeTextDecoration_test3(void **state)
{
	swprintf(buf, ARRAY_SIZE(buf), L"%c1,1%c2,22foo%c33,3bar",
	    COLOR, COLOR, COLOR);
	squeeze_text_deco_wide(buf);
	assert_true(wcscmp(buf, L"foobar") == 0);
	UNUSED_PARAM(state);
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(canSqueezeTextDecoration_test1),
		cmocka_unit_test(canSqueezeTextDecoration_test2),
		cmocka_unit_test(canSqueezeTextDecoration_test3),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
