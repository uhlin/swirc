#include "common.h"

#include "../tls-server.h"

#include "atomicops.h"
#include "rgui.h"

void
cmd_rgui(const char *data)
{
	if (atomic_load_bool(&g_accepting_new_connections))
		tls_server_end();
	else
		tls_server_begin(atoi(data));
}
