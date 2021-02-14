#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "strHand.h"

static void
canFeedString_test1(void **state)
{
    char str[] = "This is the target string";
    const char *expected = "This\nis\nthe\ntarget\nstring";

    if (strFeed(str, 4) != 4)
	fail();
    assert_string_equal(str, expected);
}

static void
canFeedString_test2(void **state)
{
    char str[] = "This is the target string";
    const char *expected = "This\nis\nthe\ntarget string";

    if (strFeed(str, 3) != 3)
	fail();
    assert_string_equal(str, expected);
}

static void
canFeedString_test3(void **state)
{
    char str[] = "This is the target string";
    const char *expected = "This\nis\nthe target string";

    if (strFeed(str, 2) != 2)
	fail();
    assert_string_equal(str, expected);
}

static void
canFeedString_test4(void **state)
{
    char str[] = "This is the target string";
    const char *expected = "This\nis the target string";

    if (strFeed(str, 1) != 1)
	fail();
    assert_string_equal(str, expected);
}

static void
canFeedString_test5(void **state)
{
    char str[] = "          ";
    const char *expected = "\n\n\n\n\n     ";

    if (strFeed(str, 5) != 5)
	fail();
    assert_string_equal(str, expected);
}

static void
canFeedString_test6(void **state)
{
    char str[] = "          ";
    const char *expected = "\n\n\n\n\n\n\n\n\n\n";

    if (strFeed(str, 10) != 10)
	fail();
    assert_string_equal(str, expected);
}

static void
leavesStringUnchanged_test1(void **state)
{
    char str[] = "\t\t\t\t\t";
    const char *expected = "\t\t\t\t\t";

    if (strFeed(str, 5) != 0)
	fail();
    assert_string_equal(str, expected);
}

static void
leavesStringUnchanged_test2(void **state)
{
    char str[] = "0123456789";
    const char *expected = "0123456789";

    if (strFeed(str, 10) != 0)
	fail();
    assert_string_equal(str, expected);
}

static void
leavesStringUnchanged_test3(void **state)
{
    char str[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const char *expected =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    if (strFeed(str, 26) != 0)
	fail();
    assert_string_equal(str, expected);
}

int
main(void)
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(canFeedString_test1),
	cmocka_unit_test(canFeedString_test2),
	cmocka_unit_test(canFeedString_test3),
	cmocka_unit_test(canFeedString_test4),
	cmocka_unit_test(canFeedString_test5),
	cmocka_unit_test(canFeedString_test6),
	cmocka_unit_test(leavesStringUnchanged_test1),
	cmocka_unit_test(leavesStringUnchanged_test2),
	cmocka_unit_test(leavesStringUnchanged_test3),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
