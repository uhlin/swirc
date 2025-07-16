#ifndef NETWORK_H
#define NETWORK_H

#include "atomicops.h"
#include "mutex.h"

#if defined(UNIX)
#include "net-unix.h"
#elif defined(WIN32)
#include "net-w32.h"
#endif

#include "x509_check_host.h"

#if defined(UNIX)
#define CLOSE_GLOBAL_SOCKET()\
	((void) close(g_socket))
#elif defined(WIN32)
#define CLOSE_GLOBAL_SOCKET()\
	((void) closesocket(g_socket))
#endif

#define DEFAULT_RECV_TIMEOUT	30
#define DEFAULT_SEND_TIMEOUT	15

#define TEMP_RECV_TIMEOUT	4
#define TEMP_SEND_TIMEOUT	4

struct server {
	char	*host;
	char	*port;
	char	*pass;
};

struct network_connect_context {
	char	*server;
	char	*port;
	char	*password;
	char	*username;
	char	*rl_name;
	char	*nickname;
};

typedef NONNULL int (*NET_SEND_FN)(const char *, ...) PRINTFLIKE(1);
typedef NONNULL int (*NET_RECV_FN)(struct network_recv_context *, char *, int);

typedef enum {
	CONNECTION_ESTABLISHED,
	CONNECTION_FAILED,
	SHOULD_RETRY_TO_CONNECT
} conn_res_t;

__SWIRC_BEGIN_DECLS

/* From network-openssl.c */
extern char *g_ca_file;

/*
 * net_send_fake() store the sent data into this buffer
 */
extern char g_sent[600];

extern NET_SEND_FN net_send;
extern NET_RECV_FN net_recv;

extern volatile bool g_connection_in_progress;
extern volatile bool g_connection_lost;
extern volatile bool g_irc_listening;
extern volatile bool g_on_air;

extern char g_last_server[1024];
extern char g_last_port[32];
extern char g_last_pass[256];

extern int g_socket_address_family;

/*lint -sem(net_addr_resolve, r_null) */

bool		 sasl_is_enabled(void);
conn_res_t	 net_connect(const struct network_connect_context *,
		     long int *sleep_time_seconds);
int		 net_send_fake(CSTRING, ...);
struct addrinfo *net_addr_resolve(CSTRING host, CSTRING port);
struct server	*server_new(CSTRING host, CSTRING port, CSTRING pass);
void		 destroy_null_bytes_exported(STRING, const int);
void		 net_connect_clean_up(void);
void		 net_irc_listen(bool *connection_lost);
void		 net_kill_connection(void);
void		 net_request_disconnect(void);
void		 server_destroy(struct server *);

void	net_set_sock_addr_family_ipv4(void);
void	net_set_sock_addr_family_ipv6(void);

int	 net_ssl_begin(void);
void	 net_ssl_end(void);
int	 net_ssl_check_hostname(const char *, unsigned int);
SSL	*net_ssl_getobj(void);
int	 net_ssl_send(const char *, ...);
int	 net_ssl_recv(struct network_recv_context *, char *, int);
void	 net_ssl_init(void);
void	 net_ssl_deinit(void);

__SWIRC_END_DECLS

#endif
