#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "strHand.h"

static char buf[7] = "foo";
static int ret = 0;

static void
handles_invalid_arguments(void **state)
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
does_bounds_checking(void **state)
{
    ret = sw_strcat(buf, "bar!", sizeof buf);
    assert_int_equal(ret, ERANGE);
    assert_string_equal(buf, "foo");

    ret = sw_strcat(buf, "barbaz", sizeof buf);
    assert_int_equal(ret, ERANGE);
    assert_string_equal(buf, "foo");
}

static void
can_concatenate(void **state)
{
    ret = sw_strcat(buf, "...", sizeof buf);
    assert_int_equal(ret, 0);
    assert_string_equal(buf, "foo...");
}

int
main()
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(handles_invalid_arguments),
	cmocka_unit_test(does_bounds_checking),
	cmocka_unit_test(can_concatenate),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
