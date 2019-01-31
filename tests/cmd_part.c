#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "network.h"
#include "commands/jp.h"

static int
setup(void **state)
{
    (void) state;
    net_send = net_send_fake;
    return 0;
}

static void
sendsExpectedString_test1(void **state)
{
    cmd_part("#chatzone");
    assert_string_equal(g_sent, "PART #chatzone");
}

int
main()
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(sendsExpectedString_test1),
    };

    return cmocka_run_group_tests(tests, setup, NULL);
}
