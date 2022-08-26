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
	const cryptarray_t str = "Text to encrypt";
	const cryptarray_t password = "Insecure123";
	char *ret;

	if ((ret = crypt_encrypt_str(addrof(str[0]), addrof(password[0]),
	    false)) == NULL)
		fail();
	print_message("ret: %s\n", ret);
	free(ret);
	UNUSED_PARAM(state);
}

static void
canEncryptAndDecrypt_test1(void **state)
{
#define TEXT "Hello World!"
	const cryptarray_t str = TEXT;
	const cryptarray_t password = "Encryption pass 321";
	char *ret1, *ret2;

	ret1 = ret2 = NULL;

	if ((ret1 = crypt_encrypt_str(addrof(str[0]), addrof(password[0]),
	    false)) == NULL)
		fail();
	else if ((ret2 = crypt_decrypt_str(ret1, addrof(password[0]), false)) ==
	    NULL)
		fail();

	assert_string_equal(ret2, TEXT);

	free(ret1);
	free(ret2);

	UNUSED_PARAM(state);
#undef TEXT
}

static void
canEncryptAndDecrypt_test2(void **state)
{
#define TEXT "Swirc IRC client"
	const cryptarray_t str = TEXT;
	const cryptarray_t password = "><><987 ENC PASS.!.";
	char *ret1, *ret2;

	ret1 = ret2 = NULL;

	if ((ret1 = crypt_encrypt_str(addrof(str[0]), addrof(password[0]),
	    true)) == NULL)
		fail();
	else if ((ret2 = crypt_decrypt_str(ret1, addrof(password[0]), true)) ==
	    NULL)
		fail();

	assert_string_equal(ret2, TEXT);

	free(ret1);
	free(ret2);

	UNUSED_PARAM(state);
#undef TEXT
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(canEncrypt_test1),
		cmocka_unit_test(canEncryptAndDecrypt_test1),
		cmocka_unit_test(canEncryptAndDecrypt_test2),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
