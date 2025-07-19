#ifndef NETWORK_WIN32_H
#define NETWORK_WIN32_H

#include <ws2tcpip.h>

struct network_recv_context {
	SOCKET		sock;
	int		flags;
	long int	sec;
	long int	microsec;

#ifdef __cplusplus
	network_recv_context() : sock(INVALID_SOCKET)
	    , flags(0)
	    , sec(0)
	    , microsec(0)
	{}

	network_recv_context(SOCKET p_sock, int p_flags, long int p_sec,
	    long int p_microsec) : sock(p_sock)
	    , flags(p_flags)
	    , sec(p_sec)
	    , microsec(p_microsec)
	{}
#endif
};

__SWIRC_BEGIN_DECLS
extern SOCKET g_socket;

bool	winsock_deinit(void);
bool	winsock_init(void);
int	net_recv_plain(struct network_recv_context *, STRING recvbuf,
	    int recvbuf_size);
int	net_send_plain(CSTRING, ...);
void	net_do_connect_detached(CSTRING host, CSTRING port, CSTRING pass);
void	net_listen_thread_join(void);
void	net_spawn_listen_thread(void);

void	net_set_recv_timeout(const DWORD seconds);
void	net_set_send_timeout(const DWORD seconds);
__SWIRC_END_DECLS

#endif
