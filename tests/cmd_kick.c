#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "network.h"
#include "commands/kick.h"

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
    cmd_kick("victim1,victim2,victim3 bye!");
    assert_string_equal(g_sent, "KICK #channel victim1,victim2,victim3 :bye!");
}

static void
sendsExpectedString_test2(void **state)
{
    cmd_kick("victim1,victim2,victim3");
    assert_string_equal(g_sent, "KICK #channel victim1,victim2,victim3 :");
}

static void
sendsExpectedString_test3(void **state)
{
    cmd_kick("victim1,victim2,victim3 say good bye!");
    assert_string_equal(g_sent,
	"KICK #channel victim1,victim2,victim3 :say good bye!");
}

int
main()
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(sendsExpectedString_test1),
	cmocka_unit_test(sendsExpectedString_test2),
	cmocka_unit_test(sendsExpectedString_test3),
    };

    return cmocka_run_group_tests(tests, setup, NULL);
}
