#ifndef CMD_DCC_H
#define CMD_DCC_H

#include <openssl/err.h>
#include <openssl/ssl.h>

#if WIN32
#include <intsafe.h> /* DWORD */
#include <ws2tcpip.h>
#endif

#include <stdint.h> /* uint32_t */
#ifdef __cplusplus
#include <string>
#endif
#include <time.h>

#include "../textBuffer.h"

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

#define TO_DBL(_var) static_cast<double>(_var)

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

	time_t	start;
	time_t	stop;

	dcc_get();
	dcc_get(const char *, const char *, intmax_t, uint32_t, uint16_t);
	dcc_get(const dcc_get &);
	dcc_get &operator=(const dcc_get &);
	~dcc_get();

	void	destroy(void);
	void	finalize_download(void);
	void	get_file(void);
	bool	has_completed(void) const;

	bool	is_locked(void) const;
	void	set_lock(int);

private:
	FILE		*fileptr;
	SOCKET		 sock;
	SSL		*ssl;
	SSL_CTX		*ssl_ctx;
	uint32_t	 addr;
	uint16_t	 port;

	int lock;

	bool	create_socket(void);
	bool	create_ssl_ctx(void);
	bool	create_ssl_obj(void);
	int	request_file(void);
};
#endif

typedef enum {
	TYPE_symlink,

	TYPE_block_file,
	TYPE_character_file,
	TYPE_directory,
	TYPE_fifo,
	TYPE_regular_file,
	TYPE_socket,

	TYPE_other,
	TYPE_unknown
} FileType;

__SWIRC_BEGIN_DECLS
extern const int	g_one_kilo;
extern const int	g_one_meg;
extern const int	g_one_gig;

//lint -sem(get_list_of_matching_dcc_cmds, r_null)

double		percentage(double part, double total);
void		list_dir(const char *);
PTEXTBUF	get_list_of_matching_dcc_cmds(const char *);

void cmd_dcc(const char *);

void dcc_init(void);
void dcc_deinit(void);

int imax_to_int(const char *, const intmax_t);
__SWIRC_END_DECLS

#ifdef __cplusplus
namespace dcc
{
	void		 add_file(const char *, const char *, const char *,
			     const char *) NONNULL;
	NORETURN void	 exit_thread(void);
	void		 get_file_detached(dcc_get *);
	void		 get_file_size(const intmax_t, double &, char &);
	bool		 get_remote_addr(std::string &, uint32_t &);
	const char	*get_upload_dir(void);
	void		 handle_incoming_conn(SSL *);
#if defined(UNIX)
	void		 set_recv_timeout(SOCKET, const time_t);
#elif defined(WIN32)
	void		 set_recv_timeout(SOCKET, const DWORD);
#endif
	void		 shutdown_conn(SSL *);
	bool		 want_unveil_uploads(void);
}
#endif

#endif
