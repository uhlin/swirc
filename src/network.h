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

typedef int (*NET_SEND_FN)(const char *, ...);
typedef int (*NET_RECV_FN)(struct network_recv_context *, char *, int);

extern NET_SEND_FN net_send;
extern NET_RECV_FN net_recv;

extern bool g_connection_in_progress;
extern volatile bool g_on_air;

/*lint -sem(net_addr_resolve, r_null) */

struct addrinfo *net_addr_resolve(const char *host, const char *port);
void		 net_connect(const struct network_connect_context *);

void	net_ssl_init();
void	net_ssl_deinit();
void	net_ssl_close();
int	net_ssl_start();
int	net_ssl_send(const char *fmt, ...);
int	net_ssl_recv(struct network_recv_context *ctx, char *recvbuf, int recvbuf_size);

#endif
