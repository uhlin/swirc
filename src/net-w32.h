#ifndef NETWORK_WIN32_H
#define NETWORK_WIN32_H

#include <ws2tcpip.h>

struct network_recv_context {
	SOCKET		sock;
	int		flags;
	long int	sec;
	long int	microsec;

#ifdef __cplusplus
	network_recv_context(SOCKET sock, int flags, long int sec,
	    long int microsec)
	{
		this->sock	= sock;
		this->flags	= flags;
		this->sec	= sec;
		this->microsec	= microsec;
	}
#endif
};

__SWIRC_BEGIN_DECLS
extern SOCKET g_socket;

bool	winsock_deinit(void);
bool	winsock_init(void);
int	net_recv_plain(struct network_recv_context *, char *recvbuf,
	    int recvbuf_size);
int	net_send_plain(const char *, ...);
void	net_do_connect_detached(const char *host, const char *port,
	    const char *pass);
void	net_listen_thread_join(void);
void	net_spawn_listen_thread(void);

void	net_set_recv_timeout(const DWORD seconds);
void	net_set_send_timeout(const DWORD seconds);
__SWIRC_END_DECLS

#endif
