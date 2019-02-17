#ifndef NETWORK_H
#define NETWORK_H

#if defined(UNIX)
#include "net-unix.h"
#elif defined(WIN32)
#include "net-w32.h"
#endif

#include "x509_check_host.h"

#ifdef __cplusplus
extern "C" {
#endif

struct server {
    char *host;
    char *port;
};

struct network_connect_context {
    char *server;
    char *port;
    char *password;
    char *username;
    char *rl_name;
    char *nickname;
};

typedef PTR_ARGS_NONNULL int (*NET_SEND_FN)(const char *, ...) PRINTFLIKE(1);
typedef PTR_ARGS_NONNULL int (*NET_RECV_FN)(struct network_recv_context *, char *, int);

typedef enum {
    CONNECTION_ESTABLISHED,
    CONNECTION_FAILED,
    SHOULD_RETRY_TO_CONNECT
} conn_res_t;

/*
 * net_send_fake() store the sent data into this buffer
 */
extern char g_sent[512];

extern NET_SEND_FN net_send;
extern NET_RECV_FN net_recv;

extern volatile bool g_connection_in_progress;
extern volatile bool g_on_air;

/*lint -sem(net_addr_resolve, r_null) */

struct server	*server_new(const char *host, const char *port);
void		 server_destroy(struct server *);

bool		 is_sasl_enabled(void);
int		 net_send_fake(const char *, ...);
struct addrinfo *net_addr_resolve(const char *host, const char *port);
conn_res_t	 net_connect(const struct network_connect_context *);
void		 net_irc_listen(void);

void	net_ssl_init(void);
void	net_ssl_deinit(void);
void	net_ssl_close(void);
int	net_ssl_start(void);
int	net_ssl_send(const char *fmt, ...);
int	net_ssl_recv(struct network_recv_context *, char *, int);
int	net_ssl_check_hostname(const char *, unsigned int);

#ifdef __cplusplus
}
#endif

#endif
