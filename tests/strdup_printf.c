#include "common.h"

#include <setjmp.h>
#ifdef UNIT_TESTING
#undef UNIT_TESTING
#endif
#include <cmocka.h>

#include "strdup_printf.h"

static char *ret = NULL;

static void
canDuplicatePrintfStyleFormatString_test1(void **state)
{
    const char *month     = "July";
    const char *name      = "Swirc";
    const int   day       = 30;
    const int   majorvers = 1;
    const int   minorvers = 0;
    const int   year      = 2016;

    assert_null(ret);
    ret = strdup_printf("Initial version of %s (v%d.%db) was released %s %d %d",
			name, majorvers, minorvers, month, day, year);
    assert_string_equal(ret,
	"Initial version of Swirc (v1.0b) was released July 30 2016");
    free(ret);
    ret = NULL;
}

static void
canDuplicatePrintfStyleFormatString_test2(void **state)
{
    const size_t size1 = 1001;
    const size_t size2 = 1002;
    const size_t size3 = 1003;

    assert_null(ret);
    ret = strdup_printf("%zu->%zu->%zu", size1, size2, size3);
    assert_string_equal(ret, "1001->1002->1003");
    free(ret);
    ret = NULL;
}

int
main()
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(canDuplicatePrintfStyleFormatString_test1),
	cmocka_unit_test(canDuplicatePrintfStyleFormatString_test2),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
