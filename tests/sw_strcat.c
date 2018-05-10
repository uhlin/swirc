#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "strHand.h"

static char buf[7] = "foo";
static int ret = 0;

static void
handlesInvalidArguments(void **state)
{
    ret = sw_strcat(NULL, "bar", sizeof buf);
    assert_int_equal(ret, EINVAL);
    assert_string_equal(buf, "foo");

    ret = sw_strcat(buf, NULL, sizeof buf);
    assert_int_equal(ret, EINVAL);
    assert_string_equal(buf, "foo");

    ret = sw_strcat(buf, "bar", 0);
    assert_int_equal(ret, EINVAL);
    assert_string_equal(buf, "foo");
}

static void
doesBoundsChecking(void **state)
{
    ret = sw_strcat(buf, "bar!", sizeof buf);
    assert_int_equal(ret, ERANGE);
    assert_string_equal(buf, "foo");

    ret = sw_strcat(buf, "barbaz", sizeof buf);
    assert_int_equal(ret, ERANGE);
    assert_string_equal(buf, "foo");
}

static void
canConcatenate(void **state)
{
    ret = sw_strcat(buf, "...", sizeof buf);
    assert_int_equal(ret, 0);
    assert_string_equal(buf, "foo...");
}

int
main()
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(handlesInvalidArguments),
	cmocka_unit_test(doesBoundsChecking),
	cmocka_unit_test(canConcatenate),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
