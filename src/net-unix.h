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

/*lint -printf(3, net_send) */

ssize_t net_send               (int sock, int flags, const char *fmt, ...);
ssize_t net_recv               (struct network_recv_context *, char *recvbuf, size_t recvbuf_size);
void    net_spawn_listenThread (void);
void    net_listenThread_join  (void);

#endif
