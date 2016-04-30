#ifndef NETWORK_WIN32_H
#define NETWORK_WIN32_H

#include <ws2tcpip.h>

struct network_recv_context {
    SOCKET   sock;
    int      flags;
    long int sec;
    long int microsec;
};

extern SOCKET g_socket;

bool winsock_init           (void);
bool winsock_deinit         (void);
int  net_send_plain         (const char *fmt, ...);
int  net_recv_plain         (struct network_recv_context *, char *recvbuf, int recvbuf_size);
void net_spawn_listenThread (void);
void net_listenThread_join  (void);

#endif
