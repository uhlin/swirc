#include "common.h"

#include <setjmp.h>
#include <cmocka.h>

#include "network.h"
#include "commands/op.h"

static int setup(void **state)
{
    (void) state;
    net_send = net_send_fake;
    return 0;
}

static void canTakeOp_test1(void **state)
{
    cmd_deop("victim");
    assert_string_equal(g_sent, "MODE #chatzone -o victim");
}

int main(void)
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(canTakeOp_test1),
    };

    return cmocka_run_group_tests(tests, setup, NULL);
}
