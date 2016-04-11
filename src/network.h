#ifndef NETWORK_H
#define NETWORK_H

#if defined(UNIX)
#include "net-unix.h"
#elif defined(WIN32)
#include "net-w32.h"
#endif

struct network_connect_context {
    char *server;
    char *port;
    char *password;
    char *username;
    char *rl_name;
    char *nickname;
};

extern bool g_connection_in_progress;
extern volatile bool g_on_air;

/*lint -sem(net_addr_resolve, r_null) */

struct addrinfo *net_addr_resolve(const char *host, const char *port);
void		 net_connect(const struct network_connect_context *);

#endif
