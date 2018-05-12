#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "libUtils.h"

static void
canDoMultiplication_test1(void **state)
{
    assert_true(size_product(9, 9) == 81);
}

static void
canDoMultiplication_test2(void **state)
{
    assert_true(size_product(12, 13) == 156);
}

static void
canDoMultiplication_test3(void **state)
{
    assert_true(size_product(0, 6667) == 0);
}

static void
canDoMultiplication_test4(void **state)
{
    assert_true(size_product(65535, 0) == 0);
}

int
main()
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(canDoMultiplication_test1),
	cmocka_unit_test(canDoMultiplication_test2),
	cmocka_unit_test(canDoMultiplication_test3),
	cmocka_unit_test(canDoMultiplication_test4),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
