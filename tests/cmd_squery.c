#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "network.h"
#include "commands/squery.h"

static int
setup(void **state)
{
    (void) state;
    net_send = net_send_fake;
    return 0;
}

static void
sendsExpected_test1(void **state)
{
    cmd_squery("Alis HELP");
    assert_string_equal(g_sent, "SQUERY Alis :HELP");
}

static void
sendsExpected_test2(void **state)
{
    cmd_squery("alis help list");
    assert_string_equal(g_sent, "SQUERY alis :help list");
}

int
main(void)
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(sendsExpected_test1),
	cmocka_unit_test(sendsExpected_test2),
    };

    return cmocka_run_group_tests(tests, setup, NULL);
}
