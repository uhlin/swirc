#include "common.h"

#include <setjmp.h>
#ifdef UNIT_TESTING
#undef UNIT_TESTING
#endif
#include <cmocka.h>

#include "crypt.h"

static void
canEncrypt_test1(void **state)
{
	const cryptstr_t str = "Text to encrypt";
	const cryptstr_t password = "Insecure123";
	char *ret;

	if ((ret = crypt_encrypt_str(str, password, false)) == NULL)
		fail();
	print_message("ret: %s", ret);
	free(ret);
	UNUSED_PARAM(state);
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(canEncrypt_test1),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
