#ifndef NETWORK_UNIX_H
#define NETWORK_UNIX_H

#include <sys/time.h>
#include <sys/types.h>

struct network_recv_context {
    int         sock;
    int         flags;
    time_t      sec;
    suseconds_t microsec;

#ifdef __cplusplus
    network_recv_context(int sock, int flags, time_t sec, suseconds_t microsec)
	{
	    this->sock     = sock;
	    this->flags    = flags;
	    this->sec      = sec;
	    this->microsec = microsec;
	}
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

extern int g_socket;

int	net_recv_plain(struct network_recv_context *, char *recvbuf,
	    int recvbuf_size);
int	net_send_plain(const char *, ...);
void	net_do_connect_detached(const char *host, const char *port,
	    const char *pass);
void	net_listenThread_join(void);
void	net_spawn_listenThread(void);

void net_set_recv_timeout(const time_t seconds);
void net_set_send_timeout(const time_t seconds);

#ifdef __cplusplus
}
#endif

#endif
