#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "dataClassify.h"

static void
returnsFalseOnNullInput(void **state)
{
    assert_false(is_alphabetic(NULL));
}

static void
returnsFalseOnEmptyInput(void **state)
{
    assert_false(is_alphabetic(""));
}

static void
returnsTrue_test1(void **state)
{
    const char s[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz";

    assert_true(is_alphabetic(s));
}

static void
returnsFalse_test1(void **state)
{
    assert_false(is_alphabetic("ABC DEF"));
}

static void
returnsFalse_test2(void **state)
{
    assert_false(is_alphabetic("ABC123"));
}

static void
returnsFalse_test3(void **state)
{
    assert_false(is_alphabetic("xyz "));
}

static void
returnsFalse_test4(void **state)
{
    assert_false(is_alphabetic(" xyz"));
}

int
main()
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(returnsFalseOnNullInput),
	cmocka_unit_test(returnsFalseOnEmptyInput),
	cmocka_unit_test(returnsTrue_test1),
	cmocka_unit_test(returnsFalse_test1),
	cmocka_unit_test(returnsFalse_test2),
	cmocka_unit_test(returnsFalse_test3),
	cmocka_unit_test(returnsFalse_test4),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
