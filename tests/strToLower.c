#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "strHand.h"

static void
canLowerString_test1(void **state)
{
    char str[] = "aBcDeFgHiJkLmNoPqRsTuVwXyZ";
    const char expected[] = "abcdefghijklmnopqrstuvwxyz";

    assert_string_not_equal(str, expected);
    assert_string_equal(strToLower(str), expected);
}

static void
leavesStringUnchanged_test1(void **state)
{
    char str[] = "abcdefghijklmnopqrstuvwxyz";
    const char expected[] = "abcdefghijklmnopqrstuvwxyz";

    assert_string_equal(strToLower(str), expected);
}

static void
leavesStringUnchanged_test2(void **state)
{
    char str[] = "0123456789...";
    const char expected[] = "0123456789...";

    assert_string_equal(strToLower(str), expected);
}

int
main(void)
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(canLowerString_test1),
	cmocka_unit_test(leavesStringUnchanged_test1),
	cmocka_unit_test(leavesStringUnchanged_test2),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
