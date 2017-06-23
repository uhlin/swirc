#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "printtext.h"

static char buf[300] = "";

static void
test0_squeeze_text_deco(void **state)
{
    snprintf(buf, sizeof buf, "foo%c%c%cbar%c%c%cbaz",
	BLINK, BOLD, COLOR, NORMAL, REVERSE, UNDERLINE);
    squeeze_text_deco(buf);
    assert_string_equal(buf, "foobarbaz");
}

static void
test1_squeeze_text_deco(void **state)
{
    snprintf(buf, sizeof buf, "%c%c%c%c%c%c",
	BLINK, BOLD, COLOR, NORMAL, REVERSE, UNDERLINE);
    squeeze_text_deco(buf);
    assert_string_equal(buf, "");
}

static void
test2_squeeze_text_deco(void **state)
{
    snprintf(buf, sizeof buf, "%c123", COLOR);
    squeeze_text_deco(buf);
    assert_string_equal(buf, "3");
}

static void
test3_squeeze_text_deco(void **state)
{
    snprintf(buf, sizeof buf, "%c1,test", COLOR);
    squeeze_text_deco(buf);
    assert_string_equal(buf, ",test");
}

static void
test4_squeeze_text_deco(void **state)
{
    snprintf(buf, sizeof buf, "%c12,test", COLOR);
    squeeze_text_deco(buf);
    assert_string_equal(buf, ",test");
}

static void
test5_squeeze_text_deco(void **state)
{
    snprintf(buf, sizeof buf, "%c1,2,3,test", COLOR);
    squeeze_text_deco(buf);
    assert_string_equal(buf, ",3,test");
}

static void
test6_squeeze_text_deco(void **state)
{
    snprintf(buf, sizeof buf, "%c1,234", COLOR);
    squeeze_text_deco(buf);
    assert_string_equal(buf, "4");
}

static void
test7_squeeze_text_deco(void **state)
{
    snprintf(buf, sizeof buf, "%c1,1%c2,22foo%c33,3bar", COLOR, COLOR, COLOR);
    squeeze_text_deco(buf);
    assert_string_equal(buf, "foobar");
}

int
main()
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(test0_squeeze_text_deco),
	cmocka_unit_test(test1_squeeze_text_deco),
	cmocka_unit_test(test2_squeeze_text_deco),
	cmocka_unit_test(test3_squeeze_text_deco),
	cmocka_unit_test(test4_squeeze_text_deco),
	cmocka_unit_test(test5_squeeze_text_deco),
	cmocka_unit_test(test6_squeeze_text_deco),
	cmocka_unit_test(test7_squeeze_text_deco),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
