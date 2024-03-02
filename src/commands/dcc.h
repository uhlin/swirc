#ifndef CMD_DCC_H
#define CMD_DCC_H

#include <openssl/err.h>
#include <openssl/ssl.h>

#if WIN32
#include <ws2tcpip.h>
#endif

#include <stdint.h> /* uint32_t */

#ifdef __cplusplus
#include <string>
#endif

#if defined(UNIX)
#define PATH_SEP '/'
#elif defined(WIN32)
#define PATH_SEP '\\'
#endif

#if defined(UNIX) && !defined(_SOCKET_DEFINED)
#define _SOCKET_DEFINED 1
typedef int SOCKET;
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#ifdef __cplusplus
//lint -sem(dcc_get::destroy,cleanup)
class dcc_get {
public:
	std::string nick;

	std::string	filename;
	intmax_t	filesize;

	intmax_t bytes_rem;

	double	size;
	char	unit;

	dcc_get();
	dcc_get(const char *, const char *, intmax_t, uint32_t, uint16_t);
	dcc_get(const dcc_get &);
	dcc_get &operator=(const dcc_get &);
	~dcc_get();

	void destroy(void);
	void finalize_download(void);
	void get_file(void);
	bool has_completed(void);

private:
	FILE		*fileptr;
	SOCKET		 sock;
	SSL		*ssl;
	SSL_CTX		*ssl_ctx;
	uint32_t	 addr;
	uint16_t	 port;

	bool	create_socket(void);
	bool	create_ssl_ctx(void);
	bool	create_ssl_obj(void);
	int	request_file(void);
};
#endif

__SWIRC_BEGIN_DECLS
extern const int	g_one_kilo;
extern const int	g_one_meg;
extern const int	g_one_gig;

void cmd_dcc(const char *);
__SWIRC_END_DECLS

#ifdef __cplusplus
namespace dcc
{
	void init(void);
	void deinit(void);

	void		 add_file(const char *, const char *, const char *,
			     const char *) NONNULL;
	NORETURN void	 exit_thread(void);
	void		 get_file_detached(dcc_get *);
	void		 get_file_size(const intmax_t, double &, char &);
	bool		 get_remote_addr(std::string &, uint32_t &);
	const char	*get_upload_dir(void);
	void		 handle_incoming_conn(SSL *);
	void		 shutdown_conn(SSL *) NONNULL;
	bool		 want_unveil_uploads(void);
}
#endif

#endif
