#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "libUtils.h"

static void
canDoSum_test1(void **state)
{
    const int result = int_sum(100, 50);

    assert_int_equal(result, 150);
}

static void
canDoSum_test2(void **state)
{
    const int result = int_sum(13, 33);

    assert_int_equal(result, 46);
}

static void
canDoSum_test3(void **state)
{
    const int result = int_sum(1000, 999);

    assert_int_equal(result, 1999);
}

int
main(void)
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(canDoSum_test1),
	cmocka_unit_test(canDoSum_test2),
	cmocka_unit_test(canDoSum_test3),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
