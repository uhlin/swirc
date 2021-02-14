#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "strHand.h"

static char buf[12] = "";
static int ret = 0;

static void
handlesInvalidArguments_test1(void **state)
{
    ret = sw_strcpy(NULL, "some string", sizeof buf);
    assert_int_equal(ret, EINVAL);
    assert_string_equal(buf, "");
}

static void
handlesInvalidArguments_test2(void **state)
{
    ret = sw_strcpy(buf, NULL, sizeof buf);
    assert_int_equal(ret, EINVAL);
    assert_string_equal(buf, "");
}

static void
handlesInvalidArguments_test3(void **state)
{
    ret = sw_strcpy(buf, "some string", 0);
    assert_int_equal(ret, EINVAL);
    assert_string_equal(buf, "");
}

static void
doesBoundsChecking_test1(void **state)
{
    ret = sw_strcpy(buf, "some string!", sizeof buf);
    assert_int_equal(ret, ERANGE);
    assert_string_equal(buf, "");
}

static void
doesBoundsChecking_test2(void **state)
{
    ret = sw_strcpy(buf, "some string...", sizeof buf);
    assert_int_equal(ret, ERANGE);
    assert_string_equal(buf, "");
}

static void
canCopy_test1(void **state)
{
    ret = sw_strcpy(buf, "some string", sizeof buf);
    assert_int_equal(ret, 0);
    assert_string_equal(buf, "some string");
}

int
main(void)
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(handlesInvalidArguments_test1),
	cmocka_unit_test(handlesInvalidArguments_test2),
	cmocka_unit_test(handlesInvalidArguments_test3),
	cmocka_unit_test(doesBoundsChecking_test1),
	cmocka_unit_test(doesBoundsChecking_test2),
	cmocka_unit_test(canCopy_test1),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
