#include "common.h"

#if defined(CURSES_HDR)
#include CURSES_HDR
#elif UNIX
#include <curses.h>
#elif WIN32
#include "pdcurses/curses.h"
#else
#error Cannot determine curses header file!
#endif

#include <setjmp.h>
#include <cmocka.h>

#include "strHand.h"

static void
returnsBlack(void **state)
{
    assert_string_equal(strColor(COLOR_BLACK), "Black");
}

static void
returnsRed(void **state)
{
    assert_string_equal(strColor(COLOR_RED), "Red");
}

static void
returnsGreen(void **state)
{
    assert_string_equal(strColor(COLOR_GREEN), "Green");
}

static void
returnsYellow(void **state)
{
    assert_string_equal(strColor(COLOR_YELLOW), "Yellow");
}

static void
returnsBlue(void **state)
{
    assert_string_equal(strColor(COLOR_BLUE), "Blue");
}

static void
returnsMagenta(void **state)
{
    assert_string_equal(strColor(COLOR_MAGENTA), "Magenta");
}

static void
returnsCyan(void **state)
{
    assert_string_equal(strColor(COLOR_CYAN), "Cyan");
}

static void
returnsWhite(void **state)
{
    assert_string_equal(strColor(COLOR_WHITE), "White");
}

int
main(void)
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(returnsBlack),
	cmocka_unit_test(returnsRed),
	cmocka_unit_test(returnsGreen),
	cmocka_unit_test(returnsYellow),
	cmocka_unit_test(returnsBlue),
	cmocka_unit_test(returnsMagenta),
	cmocka_unit_test(returnsCyan),
	cmocka_unit_test(returnsWhite),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
