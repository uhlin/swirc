#ifndef CMD_DCC_H
#define CMD_DCC_H

#include <openssl/ssl.h>

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
	bool		 get_remote_addr(std::string &, uint32_t &);
	const char	*get_upload_dir(void);
	void		 handle_incoming_conn(SSL *);
	bool		 want_unveil_uploads(void);
}
#endif

#endif
