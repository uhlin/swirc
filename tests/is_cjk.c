#include "common.h"

#include <setjmp.h>
#ifdef UNIT_TESTING
#undef UNIT_TESTING
#endif
#include <cmocka.h>

#include "dataClassify.h"

static void
returnsTrue_test1(void **state)
{
	wchar_t wc;
	wchar_t *ptr = &wc;

	*ptr = L'一';
	assert_true(*ptr == 0x4e00);
	assert_true(is_cjk(*ptr));
	for (int i = 0; i < 10; i++) {
		(*ptr)++;
		assert_true(is_cjk(*ptr));
	}
	UNUSED_PARAM(state);
}

static void
returnsTrue_test2(void **state)
{
	wchar_t wc;
	wchar_t *ptr = &wc;

	*ptr = L'俿';
	assert_true(*ptr == 0x4fff);
	assert_true(is_cjk(*ptr));
	for (int i = 0; i < 10; i++) {
		(*ptr)++;
		assert_true(is_cjk(*ptr));
	}
	UNUSED_PARAM(state);
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(returnsTrue_test1),
		cmocka_unit_test(returnsTrue_test2),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
