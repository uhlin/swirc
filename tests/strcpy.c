#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "strHand.h"

static char buf[12] = "";
static int ret = 0;

static void
handles_invalid_arguments(void **state)
{
    ret = sw_strcpy(NULL, "some string", sizeof buf);
    assert_int_equal(ret, EINVAL);
    assert_string_equal(buf, "");

    ret = sw_strcpy(buf, NULL, sizeof buf);
    assert_int_equal(ret, EINVAL);
    assert_string_equal(buf, "");

    ret = sw_strcpy(buf, "some string", 0);
    assert_int_equal(ret, EINVAL);
    assert_string_equal(buf, "");
}

static void
does_bounds_checking(void **state)
{
    ret = sw_strcpy(buf, "some string!", sizeof buf);
    assert_int_equal(ret, ERANGE);
    assert_string_equal(buf, "");

    ret = sw_strcpy(buf, "some string...", sizeof buf);
    assert_int_equal(ret, ERANGE);
    assert_string_equal(buf, "");
}

static void
can_copy(void **state)
{
    ret = sw_strcpy(buf, "some string", sizeof buf);
    assert_int_equal(ret, 0);
    assert_string_equal(buf, "some string");
}

int
main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(handles_invalid_arguments),
	cmocka_unit_test(does_bounds_checking),
	cmocka_unit_test(can_copy),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
