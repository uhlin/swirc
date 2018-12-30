#include "common.h"

#include <setjmp.h>
#ifdef UNIT_TESTING
#undef UNIT_TESTING
#endif
#include <cmocka.h>

#include "libUtils.h"
#include "strHand.h"

static void
canConcatenate_test1(void **state)
{
    char *string = sw_strdup("X");

    realloc_strcat(&string, "Y");
    assert_string_equal(string, "XY");

    realloc_strcat(&string, "Z");
    assert_string_equal(string, "XYZ");

    free(string);
}

static void
canConcatenate_test2(void **state)
{
    char *string = sw_strdup("FOO");

    realloc_strcat(&string, "BAR");
    assert_string_equal(string, "FOOBAR");

    realloc_strcat(&string, "BAZ");
    assert_string_equal(string, "FOOBARBAZ");

    free(string);
}

static void
canConcatenate_test3(void **state)
{
    char *string = sw_strdup("");

    realloc_strcat(&string, "abc");
    assert_string_equal(string, "abc");

    free(string);
}

static void
zeroLengthDestAndSrc_test(void **state)
{
    char *string = sw_strdup("");

    realloc_strcat(&string, "");
    assert_string_equal(string, "");

    free(string);
}

int
main()
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(canConcatenate_test1),
	cmocka_unit_test(canConcatenate_test2),
	cmocka_unit_test(canConcatenate_test3),
	cmocka_unit_test(zeroLengthDestAndSrc_test),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
