#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "libUtils.h"

static void
canDoDifference_test1(void **state)
{
    const int result = int_diff(100, 50);

    assert_int_equal(result, 50);
}

static void
canDoDifference_test2(void **state)
{
    const int result = int_diff(13, 33);

    assert_int_equal(result, -20);
}

static void
canDoDifference_test3(void **state)
{
    const int result = int_diff(1000, 999);

    assert_int_equal(result, 1);
}

int
main()
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(canDoDifference_test1),
	cmocka_unit_test(canDoDifference_test2),
	cmocka_unit_test(canDoDifference_test3),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
