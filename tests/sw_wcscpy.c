#include "common.h"

#include <setjmp.h>
#include <wchar.h> /* wcscmp() */
#include <cmocka.h>

#include "strHand.h"

static void
handlesBufferOverflow_test1(void **state)
{
    wchar_t dest[4] = L"";

    if (sw_wcscpy(dest, L"ABCD", ARRAY_SIZE(dest)) != ERANGE)
	fail();
    else if (wcscmp(dest, L"") != 0)
	fail();
}

static void
canCopy_test1(void **state)
{
    wchar_t dest[5] = L"";

    if (sw_wcscpy(dest, L"ABCD", ARRAY_SIZE(dest)) != 0)
	fail();
    else if (wcscmp(dest, L"ABCD") != 0)
	fail();
}

static void
canCopy_test2(void **state)
{
    wchar_t dest[7] = L"";

    if (sw_wcscpy(dest, L"FOOBAR", ARRAY_SIZE(dest)) != 0)
	fail();
    else if (wcscmp(dest, L"FOOBAR") != 0)
	fail();
}

int
main(void)
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(handlesBufferOverflow_test1),
	cmocka_unit_test(canCopy_test1),
	cmocka_unit_test(canCopy_test2),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
