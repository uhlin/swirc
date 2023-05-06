#include "common.h"

#include <setjmp.h>
#ifdef UNIT_TESTING
#undef UNIT_TESTING
#endif
#include <cmocka.h>

#include "dataClassify.h"

#ifndef HAVE_ETEXT_SEGMENT
#pragma message("Etext segment not present")
#endif

#define yesno(_b) ((_b) ? "yes" : "no")

int main(void);

int global;

static bool bval;

static void
isValid_ptrToLocal(void **state)
{
	int local;

	print_message("pointer to local var valid? %s\n",
	    yesno(isValid(&local)));
	UNUSED_PARAM(state);
}

static void
isValid_ptrToGlobal(void **state)
{
	print_message("pointer to global var valid? %s\n",
	    yesno(isValid(&global)));
	UNUSED_PARAM(state);
}

static void
isValid_ptrToFn(void **state)
{
	print_message("pointer to function valid? %s\n",
	    yesno(isValid((void *)main)));
	UNUSED_PARAM(state);
}

static void
isValid_ptrToHeap(void **state)
{
	int *ip = malloc(sizeof *ip);

	print_message("pointer to heap valid? %s\n", yesno(isValid(ip)));
	print_message("pointer to end of allocated heap valid? %s\n",
	    yesno(isValid(++ip)));
	free(--ip);
	print_message("pointer to freed heap valid? %s\n", yesno(isValid(ip)));
	UNUSED_PARAM(state);
}

static void
isValid_nullPtr(void **state)
{
	bval = isValid(NULL);
	print_message("null pointer valid? %s\n", yesno(bval));
	assert_false(bval);
	UNUSED_PARAM(state);
}

static void
isValid_unusualLoc(void **state)
{
	bval = isValid((void *)6);
	print_message("unusual location valid? %s\n", yesno(bval));
	UNUSED_PARAM(state);
}

static void
isValid_emptyStr(void **state)
{
	bval = isValid("");
	print_message("empty string valid? %s\n", yesno(bval));
	UNUSED_PARAM(state);
}

static void
isValid_fixedStr(void **state)
{
	bval = isValid("foo");
	print_message("fixed string valid? %s\n", yesno(bval));
	UNUSED_PARAM(state);
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(isValid_ptrToLocal),
		cmocka_unit_test(isValid_ptrToGlobal),
		cmocka_unit_test(isValid_ptrToFn),
		cmocka_unit_test(isValid_ptrToHeap),
		cmocka_unit_test(isValid_nullPtr),
		cmocka_unit_test(isValid_unusualLoc),
		cmocka_unit_test(isValid_emptyStr),
		cmocka_unit_test(isValid_fixedStr),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
