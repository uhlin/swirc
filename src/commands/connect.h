#ifndef CMD_CONNECT_H
#define CMD_CONNECT_H

#define ICB_PORT	"7326"
#define IRC_PORT	"6667"
#define IRC_SSL_PORT	"6697"

#define IRC_CONNECT(m_server, m_port)\
	net_do_connect_detached((m_server), (m_port),\
	    (g_connection_password ? get_password() : NULL))

typedef struct tagIRC_SERVER {
	const char	*host;
	const char	*port;
	const char	*desc;
} IRC_SERVER, *PIRC_SERVER;

typedef const struct tagIRC_SERVER servarray_const_t[];
typedef const struct tagIRC_SERVER *servarray_const_ptr_t;

__SWIRC_BEGIN_DECLS
extern bool g_disconnect_wanted;

void	do_connect(const char *server, const char *port, const char *pass);
void	set_ssl_on(void);
void	set_ssl_off(void);
bool	ssl_is_enabled(void);

void	cmd_connect(const char *);
void	cmd_disconnect(const char *);
__SWIRC_END_DECLS

#endif
