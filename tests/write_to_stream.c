#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include <unistd.h>

#include "libUtils.h"
#include "strHand.h"

static char	data[100] = { '\0' };
static char	file[25] = { '\0' };

static void
canWriteToStream_test1(void **state)
{
	FILE	*fp = NULL;
	int	 fd = -1;

	if (sw_strcpy(file, "/tmp/swirc.ut.XXXXXXXXXX", ARRAY_SIZE(file)) !=
	    0) {
		fail();
	} else if ((fd = mkstemp(file)) == -1 || (fp = fdopen(fd, "w")) ==
	    NULL) {
		if (fd != -1) {
			(void) unlink(file);
			(void) close(fd);
		}
		fail();
	}
	write_to_stream(fp, "hello world\n");
	fclose_ensure_success(fp);

	fp = fopen_exit_on_error(file, "r");
	if (fgets(data, size_to_int(ARRAY_SIZE(data)), fp) == NULL)
		fail();
	fclose_ensure_success(fp);
	assert_string_equal(data, "hello world\n");
}

static void
canWriteToStream_test2(void **state)
{
	FILE	*fp;

	fp = fopen_exit_on_error(file, "a");
	write_to_stream(fp, "one two three\n");
	fclose_ensure_success(fp);

	fp = fopen_exit_on_error(file, "r");
	if (fgets(data, size_to_int(ARRAY_SIZE(data)), fp) == NULL)
		fail();
	assert_string_equal(data, "hello world\n");
	if (fgets(data, size_to_int(ARRAY_SIZE(data)), fp) == NULL)
		fail();
	assert_string_equal(data, "one two three\n");
	fclose_ensure_success(fp);
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(canWriteToStream_test1),
		cmocka_unit_test(canWriteToStream_test2),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
