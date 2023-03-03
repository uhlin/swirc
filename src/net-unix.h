#ifndef NETWORK_UNIX_H
#define NETWORK_UNIX_H

#include <sys/time.h>
#include <sys/types.h>

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

struct network_recv_context {
	int		sock;
	int		flags;
	time_t		sec;
	suseconds_t	microsec;

#ifdef __cplusplus
	network_recv_context() : sock(INVALID_SOCKET)
	    , flags(0)
	    , sec(0)
	    , microsec(0)
	{}

	network_recv_context(int p_sock, int p_flags, time_t p_sec,
	    suseconds_t p_microsec) : sock(p_sock)
	    , flags(p_flags)
	    , sec(p_sec)
	    , microsec(p_microsec)
	{}
#endif
};

__SWIRC_BEGIN_DECLS
extern int g_socket;

int	net_recv_plain(struct network_recv_context *, char *recvbuf,
	    int recvbuf_size);
int	net_send_plain(const char *, ...);
void	net_do_connect_detached(const char *host, const char *port,
	    const char *pass);
void	net_listen_thread_join(void);
void	net_spawn_listen_thread(void);

void net_set_recv_timeout(const time_t seconds);
void net_set_send_timeout(const time_t seconds);
__SWIRC_END_DECLS

#endif
