#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "printtext.h"

static char buf[300] = "";

static void
canSqueezeTextDecoration_test1(void **state)
{
    snprintf(buf, sizeof buf, "foo%c%c%cbar%c%c%cbaz",
	     BLINK, BOLD, COLOR, NORMAL, REVERSE, UNDERLINE);
    squeeze_text_deco(buf);
    assert_string_equal(buf, "foobarbaz");
}

static void
canSqueezeTextDecoration_test2(void **state)
{
    snprintf(buf, sizeof buf, "%c%c%c%c%c%c",
	     BLINK, BOLD, COLOR, NORMAL, REVERSE, UNDERLINE);
    squeeze_text_deco(buf);
    assert_string_equal(buf, "");
}

static void
canSqueezeTextDecoration_test3(void **state)
{
    snprintf(buf, sizeof buf, "%c123", COLOR);
    squeeze_text_deco(buf);
    assert_string_equal(buf, "3");
}

static void
canSqueezeTextDecoration_test4(void **state)
{
    snprintf(buf, sizeof buf, "%c1,test", COLOR);
    squeeze_text_deco(buf);
    assert_string_equal(buf, ",test");
}

static void
canSqueezeTextDecoration_test5(void **state)
{
    snprintf(buf, sizeof buf, "%c12,test", COLOR);
    squeeze_text_deco(buf);
    assert_string_equal(buf, ",test");
}

static void
canSqueezeTextDecoration_test6(void **state)
{
    snprintf(buf, sizeof buf, "%c1,2,3,test", COLOR);
    squeeze_text_deco(buf);
    assert_string_equal(buf, ",3,test");
}

static void
canSqueezeTextDecoration_test7(void **state)
{
    snprintf(buf, sizeof buf, "%c1,234", COLOR);
    squeeze_text_deco(buf);
    assert_string_equal(buf, "4");
}

static void
canSqueezeTextDecoration_test8(void **state)
{
    snprintf(buf, sizeof buf, "%c1,1%c2,22foo%c33,3bar", COLOR, COLOR, COLOR);
    squeeze_text_deco(buf);
    assert_string_equal(buf, "foobar");
}

int
main(void)
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(canSqueezeTextDecoration_test1),
	cmocka_unit_test(canSqueezeTextDecoration_test2),
	cmocka_unit_test(canSqueezeTextDecoration_test3),
	cmocka_unit_test(canSqueezeTextDecoration_test4),
	cmocka_unit_test(canSqueezeTextDecoration_test5),
	cmocka_unit_test(canSqueezeTextDecoration_test6),
	cmocka_unit_test(canSqueezeTextDecoration_test7),
	cmocka_unit_test(canSqueezeTextDecoration_test8),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
