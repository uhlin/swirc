#ifndef SRC_COMMANDS_FTP_H_
#define SRC_COMMANDS_FTP_H_

#if defined(UNIX) && !defined(_SOCKET_DEFINED)
#define _SOCKET_DEFINED 1
typedef int SOCKET;
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

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
