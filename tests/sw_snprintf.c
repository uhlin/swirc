#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "strHand.h"

static void
canWriteFormatted_test1(void **state)
{
    char        dest[64]  = "";
    const char *month     = "July";
    const char *name      = "Swirc";
    const int   day       = 30;
    const int   majorvers = 1;
    const int   minorvers = 0;
    const int   year      = 2016;

    sw_snprintf(dest, ARRAY_SIZE(dest),
	"Initial version of %s (v%d.%db) was released %s %d %d",
	name, majorvers, minorvers, month, day, year);
    assert_string_equal(dest,
	"Initial version of Swirc (v1.0b) was released July 30 2016");
}

static void
canWriteFormatted_test2(void **state)
{
    char dest[32] = "";
    const size_t size1 = 1001;
    const size_t size2 = 1002;
    const size_t size3 = 1003;

    sw_snprintf(dest, ARRAY_SIZE(dest), "%zu->%zu->%zu", size1, size2, size3);
    assert_string_equal(dest, "1001->1002->1003");
}

int
main(void)
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(canWriteFormatted_test1),
	cmocka_unit_test(canWriteFormatted_test2),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
