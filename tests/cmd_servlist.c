#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "network.h"
#include "commands/servlist.h"

/*
 * usage: /servlist [<mask> [<type>]]
 */

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
    cmd_servlist("");
    assert_string_equal(g_sent, "SERVLIST");
}

static void
sendsExpected_test2(void **state)
{
    cmd_servlist("*");
    assert_string_equal(g_sent, "SERVLIST *");
}

static void
sendsExpected_test3(void **state)
{
    cmd_servlist("* 0xD000");
    assert_string_equal(g_sent, "SERVLIST * 0xD000");
}

int
main(void)
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(sendsExpected_test1),
	cmocka_unit_test(sendsExpected_test2),
	cmocka_unit_test(sendsExpected_test3),
    };

    return cmocka_run_group_tests(tests, setup, NULL);
}
