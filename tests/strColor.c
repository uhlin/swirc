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
	UNUSED_PARAM(state);
}

static void
returnsRed(void **state)
{
	assert_string_equal(strColor(COLOR_RED), "Red");
	UNUSED_PARAM(state);
}

static void
returnsGreen(void **state)
{
	assert_string_equal(strColor(COLOR_GREEN), "Green");
	UNUSED_PARAM(state);
}

static void
returnsYellow(void **state)
{
	assert_string_equal(strColor(COLOR_YELLOW), "Yellow");
	UNUSED_PARAM(state);
}

static void
returnsBlue(void **state)
{
	assert_string_equal(strColor(COLOR_BLUE), "Blue");
	UNUSED_PARAM(state);
}

static void
returnsMagenta(void **state)
{
	assert_string_equal(strColor(COLOR_MAGENTA), "Magenta");
	UNUSED_PARAM(state);
}

static void
returnsCyan(void **state)
{
	assert_string_equal(strColor(COLOR_CYAN), "Cyan");
	UNUSED_PARAM(state);
}

static void
returnsWhite(void **state)
{
	assert_string_equal(strColor(COLOR_WHITE), "White");
	UNUSED_PARAM(state);
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
