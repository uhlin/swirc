#ifndef SRC_COMMANDS_FTP_H_
#define SRC_COMMANDS_FTP_H_
/* ftp.h
   Copyright (C) 2024, 2025 Markus Uhlin. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

   - Neither the name of the author nor the names of its contributors may be
     used to endorse or promote products derived from this software without
     specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

#if defined(UNIX)
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

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

#include <stdio.h>

#include "../irc.h"

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

#define SELECT_AND_RUN_CMD()\
	do {\
		if (strings_match(name, "login"))\
			ftp::login();\
		else if (strings_match(name, "ls dir"))\
			ftp::ls_dir();\
		else\
			err_log(0, "%s: incorrect command: '%s'", __func__,\
			    name);\
	} while (false)

__SWIRC_BEGIN_DECLS
void	cmd_ftp(CSTRING);

void	ftp_init(void);
void	ftp_deinit(void);
__SWIRC_END_DECLS

#ifdef __cplusplus
typedef struct tagFTP_REPLY {
	int		num;
	std::string	text;

	tagFTP_REPLY() : num(0)
	{
		this->text.assign("");
	}

	tagFTP_REPLY(int p_num, CSTRING p_text) : num(p_num)
	{
		this->text.assign(p_text);
	}
} FTP_REPLY, *PFTP_REPLY;

typedef std::vector<FTP_REPLY>::size_type numrep_t;
typedef std::vector<std::string>::size_type numstr_t;

class ftp_ctl_conn {
public:
	ftp_ctl_conn();
	~ftp_ctl_conn();

	std::vector<FTP_REPLY> reply_vec;

	SOCKET		get_sock(void) const;
	void		login(void);
	void		printreps(void);
	numrep_t	read_reply(const int);

private:
	SOCKET				 sock;
	char				 buf[2048];
	enum message_concat_state	 state;
	std::string			 message_concat;
	struct addrinfo			*res;
};

class ftp_data_conn {
public:
	ftp_data_conn();
	explicit ftp_data_conn(CSTRING);
	~ftp_data_conn();

	STRING full_path, path;
	std::vector<std::string> vec;

	bool		connect_passive(void);
	void		get_file(void);
	SOCKET		get_sock(void) const;
	numstr_t	list_fetch(const int);
	void		list_print(void);

private:
	FILE				*fileptr;
	SOCKET				 sock;
	STRING				 host_str, port_str;
	char				 buf[2048];
	enum message_concat_state	 state;
	std::string			 message_concat;
	struct addrinfo			*res;
	uint16_t			 port;
	uint8_t				 h[4], p[2];
};

namespace ftp
{
	extern ftp_ctl_conn *ctl_conn;
	extern ftp_data_conn *data_conn;

	void		do_cmd_detached(CSTRING);
	NORETURN void	exit_thread(void);
	void		get_file(void);
	CSTRING		get_upload_dir(void);
	void		login(void);
	void		ls_dir(void);
	bool		passive(void);
	int		send_printf(SOCKET, CSTRING, ...) PRINTFLIKE(2);
#if defined(UNIX)
	void		set_timeout(SOCKET, int, const time_t);
#elif defined(WIN32)
	void		set_timeout(SOCKET, int, const DWORD);
#endif
	bool		want_unveil_uploads(void);
}
#endif

#endif
