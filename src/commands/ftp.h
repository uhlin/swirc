#ifndef SRC_COMMANDS_FTP_H_
#define SRC_COMMANDS_FTP_H_

#if defined(UNIX)
#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <unistd.h>
#elif defined(WIN32)
#include <intsafe.h> /* DWORD */
#include <ws2tcpip.h>
#endif

#ifdef __cplusplus
#include <string>
#include <vector>
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

#if defined(UNIX)
#define ftp_closesocket(_sock) ((void) close(_sock))
#elif defined(WIN32)
#define ftp_closesocket(_sock) ((void) closesocket(_sock))
#endif

#define FTP_DEFAULT_RECV_TIMEOUT 30
#define FTP_DEFAULT_SEND_TIMEOUT 15

#define FTP_TEMP_RECV_TIMEOUT 4
#define FTP_TEMP_SEND_TIMEOUT 4

__SWIRC_BEGIN_DECLS
void	cmd_ftp(CSTRING);
__SWIRC_END_DECLS

#ifdef __cplusplus
namespace ftp
{
	CSTRING get_upload_dir(void);
	bool want_unveil_uploads(void);
}
#endif

#endif
