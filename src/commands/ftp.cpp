/* The FTP command
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

#include "common.h"

#if __OpenBSD__
#include <sys/param.h>
#endif

#include <stdexcept>

#include "../config.h"
#include "../dataClassify.h"
#include "../errHand.h"
#include "../filePred.h"
#include "../libUtils.h"
#include "../main.h"
#include "../nestHome.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../strdup_printf.h"
#include "../theme.h"

#include "dcc.h" /* list_dir() */
#include "ftp.h"
#include "i18n.h"

#define RECV_AND_CHECK(p_microsec)\
	do {\
		int m_bytes_received;\
		struct network_recv_context m_recv_ctx(this->sock, 0, timeo,\
		    p_microsec);\
\
		BZERO(this->buf, sizeof this->buf);\
		m_bytes_received = net_recv_plain(&m_recv_ctx, this->buf,\
		    sizeof this->buf - 1);\
\
		if (m_bytes_received <= 0)\
			return 0;\
		if (memchr(this->buf, 0, m_bytes_received) != nullptr)\
			destroy_null_bytes_exported(this->buf,\
			    m_bytes_received);\
		if (strpbrk(this->buf, sep) == nullptr)\
			return 0;\
	} while (false)

ftp_ctl_conn	*ftp::ctl_conn = nullptr;
ftp_data_conn	*ftp::data_conn = nullptr;

static void delete_data_conn(void);
static void print_one_rep(const int, CSTRING);

ftp_ctl_conn::ftp_ctl_conn()
    : sock(INVALID_SOCKET)
    , state(CONCAT_BUFFER_IS_EMPTY)
    , res(nullptr)
{
	BZERO(this->buf, sizeof this->buf);
	this->message_concat.assign("");
}

ftp_ctl_conn::~ftp_ctl_conn()
{
	if (this->sock != INVALID_SOCKET) {
		ftp_closesocket(this->sock);
		this->sock = INVALID_SOCKET;
	}

	if (this->res != nullptr) {
		freeaddrinfo(this->res);
		this->res = nullptr;
	}
}

SOCKET
ftp_ctl_conn::get_sock(void) const
{
	return (this->sock);
}

static bool
establish_conn(SOCKET &sock, const struct addrinfo *res)
{
	for (const struct addrinfo *rp = res; rp; rp = rp->ai_next) {
		if ((sock = socket(rp->ai_family, rp->ai_socktype,
		    rp->ai_protocol)) == INVALID_SOCKET)
			continue;

		ftp::set_timeout(sock, SO_RCVTIMEO, FTP_TEMP_RECV_TIMEOUT);
		ftp::set_timeout(sock, SO_SNDTIMEO, FTP_TEMP_SEND_TIMEOUT);

		if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
			ftp::set_timeout(sock, SO_RCVTIMEO,
			    FTP_DEFAULT_RECV_TIMEOUT);
			ftp::set_timeout(sock, SO_SNDTIMEO,
			    FTP_DEFAULT_SEND_TIMEOUT);
			return true;
		} else {
			ftp_closesocket(sock);
			sock = INVALID_SOCKET;
		}
	} /* for */

	return false;
}

void
ftp_ctl_conn::login(void)
{
	CSTRING		array[2];
	const size_t	PASSMAX = 100;
	immutable_cp_t	host = Config("ftp_host");
	immutable_cp_t	port = Config("ftp_port");
	int		n_sent;
	long int	dummy = 0;

	array[0] = Config("ftp_user");
	array[1] = Config("ftp_pass");

	try {
		if (!is_valid_username(array[0])) {
			throw std::runtime_error(_("Invalid username"));
		} else if (strlen(array[1]) > PASSMAX) {
			throw std::runtime_error(_("Too long password"));
		} else if (!is_valid_hostname(host)) {
			throw std::runtime_error(_("Invalid hostname"));
		} else if (!getval_strtol(port, 1, 65535, &dummy)) {
			throw std::runtime_error(_("Invalid port number"));
		} else if ((this->res = net_addr_resolve(host, port)) ==
			   nullptr) {
			throw std::runtime_error(_("Unable to get a list of "
			    "IP addresses"));
		} else if (!establish_conn(this->sock, this->res)) {
			throw std::runtime_error(_("Failed to establish a "
			    "connection"));
		}

		while (this->read_reply(1))
			this->printreps();

		ftp::set_timeout(sock, SO_SNDTIMEO, 4);
		n_sent = ftp::send_printf(this->sock, "USER %s\r\nPASS %s\r\n",
		    array[0],
		    array[1]);
		if (n_sent <= 0)
			throw std::runtime_error(_("Cannot send"));

		while (this->read_reply(1))
			this->printreps();
	} catch (const std::exception &ex) {
		immutable_cp_t	b1 = LEFT_BRKT;
		immutable_cp_t	b2 = RIGHT_BRKT;

		printtext_print("err", "%s%s%s %s: %s", b1, "FTP", b2, __func__,
		    ex.what());
	}
}

static void
print_one_rep(const int num, CSTRING text)
{
	immutable_cp_t	b1 = Theme("notice_inner_b1");
	immutable_cp_t	b2 = Theme("notice_inner_b2");
	immutable_cp_t	sep = Theme("notice_sep");

	printtext_print("none", "%s%s%s%d%s %s", b1, "FTP", sep, num, b2, text);
}

void
ftp_ctl_conn::printreps(void)
{
	for (FTP_REPLY &rep : this->reply_vec)
		print_one_rep(rep.num, rep.text.c_str());
}

static bool
is_terminated(CSTRING buf, const size_t size)
{
	size_t last;

	if ((last = strlen(buf) - 1) >= size)
		err_exit(EOVERFLOW, "%s", __func__);

	return (buf[last] == '\r' || buf[last] == '\n');
}

static std::string
get_last_token(CSTRING buffer)
{
	CSTRING last_token;

	if ((last_token = strrchr(buffer, '\n')) == nullptr)
		err_quit("%s: unable to locate newline", __func__);

	std::string out(++last_token);
	return out;
}

static inline bool
has_three_digits(CSTRING str)
{
	return (sw_isdigit(str[0]) &&
		sw_isdigit(str[1]) &&
		sw_isdigit(str[2]));
}

static void
handle_length_four(char (&numstr)[5], CSTRING token, const size_t len,
    std::vector<FTP_REPLY> &reply_vec)
{
	char	ch = 'a';
	int	num = 0;

	memcpy(numstr, token, len);
	numstr[len] = '\0';

	if (!has_three_digits(numstr)) {
		err_log(0, "%s: expected digits", __func__);
	} else if (sscanf(numstr, "%d%c", &num, &ch) != 2 ||
		   ch != '-') {
		err_log(0, "%s: sscanf() error", __func__);
	} else {
		FTP_REPLY rep(num, token + len);

		reply_vec.push_back(rep);
	}
}

static void
handle_length_three(char (&numstr)[5], CSTRING token, const size_t len,
    std::vector<FTP_REPLY> &reply_vec)
{
	int num = 0;

	memcpy(numstr, token, len);
	numstr[len] = '\0';

	if (!has_three_digits(numstr)) {
		err_log(0, "%s: expected digits", __func__);
	} else if (sscanf(numstr, "%d", &num) != 1) {
		err_log(0, "%s: sscanf() error", __func__);
	} else {
		FTP_REPLY rep(num, token + len);

		reply_vec.push_back(rep);
	}
}

static void
process_reply(CSTRING token, std::vector<FTP_REPLY> &reply_vec)
{
	char		numstr[5] = { '\0' };
	const size_t	len = strspn(token, "0123456789-");

	if (len >= sizeof numstr) {
		err_log(0, "%s: initial segment too long", __func__);
	} else if (len == 4) {
		handle_length_four(numstr, token, len, reply_vec);
	} else if (len == 3) {
		handle_length_three(numstr, token, len, reply_vec);
	} else if (len == 0) {
		FTP_REPLY rep(0, token);

		reply_vec.push_back(rep);
	} else {
		err_log(0, "%s: unexpected length %zu: \"%s\"", __func__, len,
		    token);
	}
}

static inline bool
shall_assign_concat(CSTRING token,
    const std::string &last_token,
    const enum message_concat_state &state)
{
	return (!last_token.empty() && state == CONCAT_BUFFER_IS_EMPTY &&
	    strings_match(last_token.c_str(), token));
}

static inline bool
shall_append_concat(const int loop_run, const enum message_concat_state &state)
{
	return (loop_run == 0 && state == CONCAT_BUFFER_CONTAIN_DATA);
}

numrep_t
ftp_ctl_conn::read_reply(const int timeo)
{
	CSTRING				token;
	STRING				tokstate = const_cast<STRING>("");
	int				loop_run = 0;
	static chararray_t		sep = "\r\n";
	std::string			last_token("");

	RECV_AND_CHECK(222000);

	this->reply_vec.clear();

	if (!is_terminated(this->buf, sizeof this->buf))
		last_token.assign(get_last_token(this->buf));

	if (this->state == CONCAT_BUFFER_CONTAIN_DATA &&
	    this->buf[0] == '\r' &&
	    this->buf[1] == '\n') {
		process_reply(this->message_concat.c_str(), this->reply_vec);

		this->message_concat.assign("");
		this->state = CONCAT_BUFFER_IS_EMPTY;
	}

	for (STRING str = this->buf;
	    (token = strtok_r(str, sep, &tokstate)) != nullptr;
	    str = nullptr) {
		if (shall_assign_concat(token, last_token, this->state)) {
			this->message_concat.assign(last_token);
			this->state = CONCAT_BUFFER_CONTAIN_DATA;

			return (this->reply_vec.size());
		} else if (shall_append_concat(loop_run, this->state)) {
			this->message_concat.append(token);
			this->state = CONCAT_BUFFER_IS_EMPTY;

			token = this->message_concat.c_str();
		}

		process_reply(token, this->reply_vec);
		loop_run++;
	} /* for */

	return (this->reply_vec.size());
}

ftp_data_conn::ftp_data_conn()
    : sock(INVALID_SOCKET)
    , host_str(nullptr)
    , port_str(nullptr)
    , state(CONCAT_BUFFER_IS_EMPTY)
    , res(nullptr)
    , port(0)
{
	BZERO(this->buf, sizeof this->buf);

	this->message_concat.assign("");

	this->h[0] = this->h[1] = this->h[2] = this->h[3] = 0;
	this->p[0] = this->p[1] = 0;
}

ftp_data_conn::ftp_data_conn(CSTRING text)
    : sock(INVALID_SOCKET)
    , host_str(nullptr)
    , port_str(nullptr)
    , state(CONCAT_BUFFER_IS_EMPTY)
    , res(nullptr)
    , port(0)
{
	static CSTRING format = " Entering Passive Mode "
	    "(%hhu,%hhu,%hhu,%hhu,%hhu,%hhu).";

	if (sscanf(text, format,
	    &this->h[0], &this->h[1], &this->h[2], &this->h[3],
	    &this->p[0], &this->p[1]) != 6)
		throw std::runtime_error("too few items assigned");

	BZERO(this->buf, sizeof this->buf);

	this->host_str = strdup_printf("%u.%u.%u.%u",
	    this->h[0], this->h[1], this->h[2], this->h[3]);
	this->port = (this->p[0] * 256 + this->p[1]);
	this->port_str = strdup_printf("%u", this->port);

	this->message_concat.assign("");
}

ftp_data_conn::~ftp_data_conn()
{
	if (this->sock != INVALID_SOCKET) {
		ftp_closesocket(this->sock);
		this->sock = INVALID_SOCKET;
	}

	free(host_str);
	free(port_str);

	if (this->res != nullptr) {
		freeaddrinfo(this->res);
		this->res = nullptr;
	}
}

bool
ftp_data_conn::connect_passive(void)
{
	struct sockaddr_in sin;

	if (this->sock == INVALID_SOCKET &&
	    (this->sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		return false;

	memset(&sin, 0, sizeof sin);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(this->port);
	sin.sin_addr.s_addr = inet_addr(this->host_str);

	if (connect(this->sock, reinterpret_cast<struct sockaddr *>(&sin),
	    sizeof sin) != 0) {
		printtext_print("err", "%s: %s", __func__,
		    _("Failed to establish a connection"));
		return false;
	}

	return true;
}

SOCKET
ftp_data_conn::get_sock(void) const
{
	return (this->sock);
}

numstr_t
ftp_data_conn::list_fetch(const int timeo)
{
	CSTRING				token;
	STRING				tokstate = const_cast<STRING>("");
	int				loop_run = 0;
	static chararray_t		sep = "\r\n";
	std::string			last_token("");

	RECV_AND_CHECK(222000);

	this->vec.clear();

	if (!is_terminated(this->buf, sizeof this->buf))
		last_token.assign(get_last_token(this->buf));

	if (this->state == CONCAT_BUFFER_CONTAIN_DATA &&
	    this->buf[0] == '\r' &&
	    this->buf[1] == '\n') {
		this->vec.push_back(this->message_concat.c_str());

		this->message_concat.assign("");
		this->state = CONCAT_BUFFER_IS_EMPTY;
	}

	for (STRING str = this->buf;
	    (token = strtok_r(str, sep, &tokstate)) != nullptr;
	    str = nullptr) {
		if (shall_assign_concat(token, last_token, this->state)) {
			this->message_concat.assign(last_token);
			this->state = CONCAT_BUFFER_CONTAIN_DATA;

			return (this->vec.size());
		} else if (shall_append_concat(loop_run, this->state)) {
			this->message_concat.append(token);
			this->state = CONCAT_BUFFER_IS_EMPTY;

			token = this->message_concat.c_str();
		}

		this->vec.push_back(token);
		loop_run++;
	} /* for */

	return (this->vec.size());
}

void
ftp_data_conn::list_print(void)
{
	if (this->vec.empty())
		return;
	for (const std::string &str : this->vec)
		print_one_rep(0, str.c_str()); // XXX
}

static bool
subcmd_ok(CSTRING cmd)
{
	if (strings_match(cmd, "exit"))
		return true;
	else if (strings_match(cmd, "login"))
		return true;
	else if (strings_match(cmd, "ls"))
		return true;
	else if (strings_match(cmd, "pwd"))
		return true;
	return false;
}

static void
subcmd_exit(void)
{
	if (ftp::ctl_conn == nullptr)
		return;

	(void) ftp::send_printf(ftp::ctl_conn->get_sock(), "QUIT\r\n");

	while (ftp::ctl_conn->read_reply(1))
		ftp::ctl_conn->printreps();

	delete ftp::ctl_conn;
	ftp::ctl_conn = nullptr;
}

static void
subcmd_login(void)
{
	if (ftp::ctl_conn != nullptr)
		delete ftp::ctl_conn;
	ftp::ctl_conn = new ftp_ctl_conn();
	ftp::ctl_conn->login();
}

static void
delete_data_conn(void)
{
	delete ftp::data_conn;
	ftp::data_conn = nullptr;
}

static void
subcmd_ls(CSTRING arg)
{
	if (arg == nullptr || strings_match(arg, "")) {
		printtext_print("err", "insufficient args");
	} else if (strings_match(arg, "dir")) {
		int n_sent;

		if (!ftp::passive())
			return;
		if (!ftp::data_conn->connect_passive()) {
			delete_data_conn();
			return;
		}

		n_sent = ftp::send_printf(ftp::ctl_conn->get_sock(),
		    "LIST\r\n");
		if (n_sent <= 0) {
			delete_data_conn();
			return;
		}

		while (ftp::data_conn->list_fetch(0))
			ftp::data_conn->list_print();

		delete_data_conn();

		while (ftp::ctl_conn->read_reply(0))
			ftp::ctl_conn->printreps();
	} else if (strings_match(arg, "up")) {
		list_dir(ftp::get_upload_dir());
	} else if (strings_match(arg, "down")) {
		list_dir(g_ftp_download_dir);
	} else {
		printtext_print("err", "what? dir, uploads or downloads?");
	}
}

static void
subcmd_pwd(void)
{
	if (ftp::ctl_conn == nullptr)
		return;

	(void) ftp::send_printf(ftp::ctl_conn->get_sock(), "PWD\r\n");

	while (ftp::ctl_conn->read_reply(0))
		ftp::ctl_conn->printreps();
}

void
cmd_ftp(CSTRING data)
{
	CSTRING			arg[2];
	CSTRING			subcmd;
	STRING			dcopy;
	STRING			last = const_cast<STRING>("");
	static chararray_t	cmd  = "/ftp";
	static chararray_t	sep  = "\n";

	if (strings_match(data, "")) {
		printtext_print("err", "insufficient args");
		return;
	}

	dcopy = sw_strdup(data);
	(void) strFeed(dcopy, 2);

	if ((subcmd = strtok_r(dcopy, sep, &last)) == nullptr) {
		printf_and_free(dcopy, "%s: insufficient args", cmd);
		return;
	} else if (!subcmd_ok(subcmd)) {
		printf_and_free(dcopy, "%s: invalid subcommand '%s'", cmd,
		    subcmd);
		return;
	}

	arg[0] = strtok_r(nullptr, sep, &last);
	arg[1] = strtok_r(nullptr, sep, &last);

	if (strings_match(subcmd, "exit"))
		subcmd_exit();
	else if (strings_match(subcmd, "login"))
		subcmd_login();
	else if (strings_match(subcmd, "ls"))
		subcmd_ls(arg[0]);
	else if (strings_match(subcmd, "pwd"))
		subcmd_pwd();
	else
		printtext_print("err", "%s: invalid subcommand", cmd);
	free(dcopy);
}

static void
create_data_conn(CSTRING text)
{
	if (ftp::data_conn) {
		delete ftp::data_conn;
		ftp::data_conn = nullptr;
	}

	try {
		ftp::data_conn = new ftp_data_conn(text);
	} catch (const std::bad_alloc &e) {
		err_exit(ENOMEM, "%s: %s", __func__, e.what());
	} catch (const std::runtime_error &e) {
		printtext_print("err", "%s: %s", __func__, e.what());
	} catch (...) {
		printtext_print("err", "%s: %s", __func__, "unknown error");
	}
}

bool
ftp::passive(void)
{
	SOCKET sock;

	if (ftp::ctl_conn == nullptr) {
		printtext_print("err", "%s: %s", __func__,
		    _("No control connection"));
		return false;
	} else if (ftp::data_conn != nullptr) {
		printtext_print("err", "%s: %s", __func__,
		    _("Already in passive mode"));
		return false;
	}

	sock = ftp::ctl_conn->get_sock();

	if (ftp::send_printf(sock, "PASV\r\n") <= 0) {
		printtext_print("err", "%s: %s", __func__,
		    _("Cannot send"));
		return false;
	}

	while (ftp::ctl_conn->read_reply(0)) {
		for (FTP_REPLY &rep : ftp::ctl_conn->reply_vec) {
			if (rep.num == 227)
				create_data_conn(rep.text.c_str());
			else
				print_one_rep(rep.num, rep.text.c_str());
		}
	}

	if (ftp::data_conn)
		return true;
	printtext_print("err", "%s", _("Failed to enter passive mode"));
	return false;
}

CSTRING
ftp::get_upload_dir(void)
{
	static CSTRING dir;

	dir = Config("ftp_upload_dir");

#if defined(OpenBSD) && OpenBSD >= 201811
	return (strings_match(dir, "") ? g_ftp_upload_dir : dir);
#else
	if (!is_directory(dir))
		return (g_ftp_upload_dir ? g_ftp_upload_dir : "");
	return dir;
#endif
}

int
ftp::send_printf(SOCKET sock, CSTRING fmt, ...)
{
	char	*buffer;
	int	 n_sent;
	va_list	 ap;

	if (sock == INVALID_SOCKET)
		return -1;
	else if (fmt == nullptr)
		err_exit(EINVAL, "%s", __func__);
	else if (strings_match(fmt, ""))
		return 0;

	va_start(ap, fmt);
	buffer = strdup_vprintf(fmt, ap);
	va_end(ap);

	errno = 0;

#if defined(UNIX)
	if ((n_sent = send(sock, buffer, strlen(buffer), 0)) == -1) {
		free_and_null(&buffer);
		return (errno == EAGAIN || errno == EWOULDBLOCK ? 0 : -1);
	}
#elif defined(WIN32)
	if ((n_sent = send(g_socket, buffer, size_to_int(strlen(buffer)), 0)) ==
	    SOCKET_ERROR) {
		free_and_null(&buffer);
		return (WSAGetLastError() == WSAEWOULDBLOCK ? 0 : -1);
	}
#endif

	free_and_null(&buffer);
	return n_sent;
}

bool
ftp::want_unveil_uploads(void)
{
	CSTRING		dir = Config("ftp_upload_dir");
	size_t		len1, len2;

	if (strings_match(dir, ""))
		return false;

	len1 = strlen(dir);
	len2 = strlen(g_home_dir);

	if (len1 >= len2 && strncmp(dir, g_home_dir, MIN(len1, len2)) ==
	    STRINGS_MATCH)
		return false;
	return true;
}
