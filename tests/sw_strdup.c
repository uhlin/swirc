#include "common.h"

#include <setjmp.h>
#ifdef UNIT_TESTING
#undef UNIT_TESTING
#endif
#include <cmocka.h>

#include "strHand.h"

static void
canDuplicateString_test1(void **state)
{
    const char *string = "Swirc IRC client";
    char *string_copy = sw_strdup(string);

    assert_string_equal(string_copy, string);
    free(string_copy);
}

static void
canDuplicateString_test2(void **state)
{
    const char *string = "\f\n\r\t\v0123456789";
    char *string_copy = sw_strdup(string);

    assert_string_equal(string_copy, string);
    free(string_copy);
}

int
main()
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(canDuplicateString_test1),
	cmocka_unit_test(canDuplicateString_test2),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
