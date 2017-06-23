#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "strdup_printf.h"

static void
test_strdup_printf(void **state)
{
    char *cp = NULL;
    const int num1 = 101;
    const int num2 = 102;
    const int num3 = 103;

    (void) state; /* unused */

    cp = Strdup_printf("%d %d %d", num1, num2, num3);
    assert_non_null(cp);
    assert_string_equal(cp, "101 102 103");
    free(cp);

    cp = Strdup_printf("%d%d%d", num1, num2, num3);
    assert_non_null(cp);
    assert_string_equal(cp, "101102103");
    free(cp);

    cp = Strdup_printf("%%%d%%%d%%%d", num1, num2, num3);
    assert_non_null(cp);
    assert_string_equal(cp, "%101%102%103");
    free(cp);

    cp = Strdup_printf("%s%d", "num1 = ", num1);
    assert_non_null(cp);
    assert_string_equal(cp, "num1 = 101");
    free(cp);
}

int
main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_strdup_printf),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
