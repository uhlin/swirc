#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include <locale.h>
#include "printtext.h"

static int
setup(void **state)
{
	setlocale(LC_ALL, "");
	UNUSED_PARAM(state);
	return 0;
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(printtext_convert_wc_test1),
		cmocka_unit_test(printtext_convert_wc_test2),
	};

	return cmocka_run_group_tests(tests, setup, NULL);
}
