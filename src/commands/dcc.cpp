#include "common.h"

#include "../config.h"
#include "../tls-server.h"

#include "dcc.h"

void
cmd_dcc(const char *data)
{
	UNUSED_PARAM(data);
}

void
dcc_init(void)
{
	if (config_bool("dcc", true)) {
		struct integer_context intctx("dcc_port", 1024, 65535, 8080);

		tls_server::begin(config_integer(&intctx));
	}
}

void
dcc_deinit(void)
{
	tls_server::end();
}
