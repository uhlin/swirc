#ifndef TLS_SERVER_H
#define TLS_SERVER_H

#include <openssl/ssl.h>

#include <stdint.h> /* uintptr_t */

#define ROOT_PEM	"root.pem"
#define SERVER_CA_PEM	"serverCA.pem"
#define SERVER_PEM	"server.pem"
#define CLIENT_PEM	"client.pem"

#define DH_PEM1 "dh2048.pem"
#define DH_PEM2 "dh4096.pem"

#define SERVER_MIN_PORT 1024
#define SERVER_MAX_PORT 65535

__SWIRC_BEGIN_DECLS
extern const char	g_suite_secure[];
extern const char	g_suite_compat[];
extern const char	g_suite_legacy[];
extern const char	g_suite_all[];

#ifdef WIN32
extern const uintptr_t g_beginthread_failed;
#endif

extern volatile bool	g_accepting_new_connections;
extern volatile bool	g_tls_server_loop;
__SWIRC_END_DECLS

/*lint -sem(tls_server::get_accept_bio, r_null) */
/*lint -sem(tls_server::setup_context, r_null) */
/*lint -sem(tls_server::exit_thread, r_no) */

#ifdef __cplusplus
namespace tls_server
{
	void		 accept_new_connections(const int);
	BIO		*get_accept_bio(const int);
	SSL_CTX		*setup_context(void);

	void		 begin(const int);
	void		 end(void);
	void		 com_with_client(SSL *);
	NORETURN void	 exit_thread(void);
}
#endif

#endif
