#ifndef NETWORK_UNIX_H
#define NETWORK_UNIX_H

#include <sys/time.h>
#include <sys/types.h>

struct network_recv_context {
    int         sock;
    int         flags;
    time_t      sec;
    suseconds_t microsec;
};

extern int g_socket;

int	net_send_plain(const char *fmt, ...);
int	net_recv_plain(struct network_recv_context *, char *recvbuf, int recvbuf_size);
void	net_spawn_listenThread(void);
void	net_listenThread_join(void);

#endif
