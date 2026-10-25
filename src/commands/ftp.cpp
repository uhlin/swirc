/* The FTP command
   Copyright (C) 2024-2026 Markus Uhlin. All rights reserved.

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
#include <sys/stat.h>

#include <climits>
#ifdef WIN32
#include <io.h>
#endif
#include <stdexcept>

#include "../assertAPI.h"
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
#include "../tls-server.h"

#include "connect.h"
#include "dcc.h" /* list_dir() */
#include "ftp.h"
#include "i18n.h"

#ifdef WIN32
#define stat _stat
#endif

SSL_SESSION	*ftp::ctl_conn_sess = nullptr;

ftp_ctl_conn	*ftp::ctl_conn = nullptr;
ftp_data_conn	*ftp::data_conn = nullptr;

volatile bool	 ftp::cmd_in_progress = false;
volatile bool	 ftp::loop_get_file = false;
volatile bool	 ftp::loop_send_file = false;

static const int VALUE_HANDSHAKE_OK = 1;

static stringarray_t ftp_cmds = {
	"cd ",
	"del ",
	"exit",
	"get ",
	"login",
	"ls ",
	"ls dir",
	"ls up",
	"ls down",
	"mkdir ",
	"pwd",
	"rmdir ",
	"send ",
	"system",
};

static void delete_data_conn(void);
static void print_one_rep(const int, CSTRING);

static bool
recv_and_check_plain(struct network_recv_context *p_ctx, char *p_buf,
    size_t p_sz, CSTRING p_sep)
{
	int bytes_received;

	BZERO(p_buf, p_sz);
	bytes_received = net_recv_plain(p_ctx, p_buf, p_sz - 1);

	if (bytes_received <= 0)
		return false;
	if (memchr(p_buf, 0, bytes_received) != nullptr)
		destroy_null_bytes_exported(p_buf, bytes_received);
	if (strpbrk(p_buf, p_sep) == nullptr)
		return false;
	return true;
}

static bool
recv_and_check_tls(struct network_recv_context *p_ctx, char *p_buf,
    size_t p_sz, CSTRING p_sep, SSL *p_ssl)
{
#ifdef UNIX
#define SOCKET_ERROR -1
#endif
	char	*bufptr;
	int	 buflen;
	int	 bytes_received;

	if (p_ssl == nullptr ||
	    conn_is_closed(p_ssl) ||
	    p_ctx->sock == INVALID_SOCKET)
		return false;
	else if (!SSL_pending(p_ssl)) {
		const int	maxfdp1 = p_ctx->sock + 1;
		fd_set		readset;
		struct timeval	tv;

		tv.tv_sec = p_ctx->sec;
		tv.tv_usec = p_ctx->microsec;

		FD_ZERO(&readset);
		FD_SET(p_ctx->sock, &readset);

		errno = 0;

		if (select(maxfdp1, &readset, nullptr, nullptr, &tv) ==
		    SOCKET_ERROR) {
			if (errno != EINTR)
				err_log(errno, "%s: select", __func__);
			return false;
		} else if (!FD_ISSET(p_ctx->sock, &readset))
			return false;
	}

	BZERO(p_buf, p_sz);

	bufptr = p_buf;
	buflen = p_sz;
	bytes_received = 0;

	do {
		int ret;

		if (conn_is_closed(p_ssl))
			return false;

		ERR_clear_error();

		if ((ret = SSL_read(p_ssl, bufptr, buflen)) > 0) {
			if (BIO_flush(SSL_get_rbio(p_ssl)) != 1)
				debug("%s: error flushing read bio", __func__);
			bytes_received += ret;
			bufptr += ret;
			buflen -= ret;
		} else {
			switch (SSL_get_error(p_ssl, ret)) {
			case SSL_ERROR_NONE:
				sw_assert_not_reached();
				break;
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
				debug("%s: want read / want write", __func__);
				break;
			default:
				return false;
			}
		}

		UNUSED_VAR(bufptr);
		UNUSED_VAR(buflen);
	} while (false); /* buflen > 0 && SSL_pending(p_ssl) */

	if (bytes_received <= 0)
		return false;
	if (memchr(p_buf, 0, bytes_received) != nullptr)
		destroy_null_bytes_exported(p_buf, bytes_received);
	if (strpbrk(p_buf, p_sep) == nullptr)
		return false;
	return true;
}

ftp_ctl_conn::ftp_ctl_conn()
    : sock(INVALID_SOCKET)
    , ssl(nullptr)
    , ssl_ctx(nullptr)
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
	if (this->ssl != nullptr) {
		SSL_free(this->ssl);
		this->ssl = nullptr;
	}
	if (this->ssl_ctx != nullptr) {
		SSL_CTX_free(this->ssl_ctx);
		this->ssl_ctx = nullptr;
	}
	if (this->res != nullptr) {
		freeaddrinfo(this->res);
		this->res = nullptr;
	}
}

/*
 * Copy assignment operator
 */
ftp_ctl_conn &
ftp_ctl_conn::operator=(const ftp_ctl_conn &obj)
{
	if (&obj == this)
		return *this;

	if (!obj.reply_vec.empty()) {
		this->reply_vec.assign(obj.reply_vec.begin(),
				       obj.reply_vec.end());
	}
	if (!obj.message_concat.empty())
		this->message_concat.assign(obj.message_concat);

	this->sock    = obj.sock;
	this->ssl     = nullptr;
	this->ssl_ctx = nullptr;
	this->state   = obj.state;
	this->res     = nullptr;

	memmove(this->buf, obj.buf, sizeof(this->buf));

	return *this;
}

/*
 * Copy constructor
 */
ftp_ctl_conn::ftp_ctl_conn(const ftp_ctl_conn &obj)
    : sock(INVALID_SOCKET)
    , ssl(nullptr)
    , ssl_ctx(nullptr)
    , state(CONCAT_BUFFER_IS_EMPTY)
    , res(nullptr)
{
	if (!obj.reply_vec.empty()) {
		this->reply_vec.assign(obj.reply_vec.begin(),
				       obj.reply_vec.end());
	}
	if (!obj.message_concat.empty())
		this->message_concat.assign(obj.message_concat);

	this->sock    = obj.sock;
	this->ssl     = nullptr;
	this->ssl_ctx = nullptr;
	this->state   = obj.state;
	this->res     = nullptr;

	memmove(this->buf, obj.buf, sizeof(this->buf));
}

/*
 * Move assignment operator
 */
ftp_ctl_conn &
ftp_ctl_conn::operator=(ftp_ctl_conn &&obj) noexcept
{
	if (&obj == this)
		return *this;

	if (this->ssl)
		SSL_free(this->ssl);
	if (this->ssl_ctx)
		SSL_CTX_free(this->ssl_ctx);
	if (this->res)
		freeaddrinfo(this->res);

	try {
		if (!obj.reply_vec.empty()) {
			this->reply_vec.assign(obj.reply_vec.begin(),
					       obj.reply_vec.end());
		}
		if (!obj.message_concat.empty())
			this->message_concat.assign(obj.message_concat);
	} catch (...) {
		err_quit("%s: unexpected exception thrown", __func__);
	}

	this->sock    = obj.sock;
	this->ssl     = obj.ssl;
	this->ssl_ctx = obj.ssl_ctx;
	this->state   = obj.state;
	this->res     = obj.res;

	memmove(this->buf, obj.buf, sizeof(this->buf));

	/* ------------------------------------------------------------ */

	try {
		obj.reply_vec.clear();
		obj.message_concat.clear();
	} catch (...) {
		err_quit("%s: unexpected exception thrown", __func__);
	}

	obj.sock    = INVALID_SOCKET;
	obj.ssl	    = nullptr;
	obj.ssl_ctx = nullptr;
	obj.state   = CONCAT_BUFFER_IS_EMPTY;
	obj.res	    = nullptr;

	BZERO(obj.buf, sizeof(obj.buf));

	return *this;
}

/*
 * Move constructor
 */
ftp_ctl_conn::ftp_ctl_conn(ftp_ctl_conn &&obj) noexcept
    : sock(obj.sock)
    , ssl(obj.ssl)
    , ssl_ctx(obj.ssl_ctx)
    , state(obj.state)
    , res(obj.res)
{
	try {
		if (!obj.reply_vec.empty()) {
			this->reply_vec.assign(obj.reply_vec.begin(),
					       obj.reply_vec.end());
		}
		if (!obj.message_concat.empty())
			this->message_concat.assign(obj.message_concat);
	} catch (...) {
		err_quit("%s: unexpected exception thrown", __func__);
	}

	memmove(this->buf, obj.buf, sizeof(this->buf));

	/* ------------------------------------------------------------ */

	try {
		obj.reply_vec.clear();
		obj.message_concat.clear();
	} catch (...) {
		err_quit("%s: unexpected exception thrown", __func__);
	}

	obj.sock    = INVALID_SOCKET;
	obj.ssl	    = nullptr;
	obj.ssl_ctx = nullptr;
	obj.state   = CONCAT_BUFFER_IS_EMPTY;
	obj.res	    = nullptr;

	BZERO(obj.buf, sizeof(obj.buf));
}

SOCKET
ftp_ctl_conn::get_sock(void) const
{
	return (this->sock);
}

SSL *
ftp_ctl_conn::get_ssl(void)
{
	return (this->ssl);
}

SSL_CTX *
ftp_ctl_conn::get_ssl_ctx(void)
{
	return (this->ssl_ctx);
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

		this->read_and_print(1);
		ftp::set_timeout(this->sock, SO_SNDTIMEO, 4);

		if (ftp::secure()) {
			bool auth_tls = false;

			n_sent = ftp::send_printf(this->sock, nullptr,
			    "AUTH TLS\r\n");
			if (n_sent <= 0)
				throw std::runtime_error(_("Cannot send"));
			while (this->read_reply(1)) {
				for (const FTP_REPLY &rep : this->reply_vec) {
					if (rep.num == 234)
						auth_tls = true;
					print_one_rep(rep.num,
					    rep.text.c_str());
				}
			}

			if (!auth_tls)
				throw std::runtime_error("Auth TLS error");

			if (!ftp::new_ssl_ctx(this->ssl_ctx))
				throw std::runtime_error("SSL context error");
			if ((this->ssl = SSL_new(this->ssl_ctx)) == nullptr)
				throw std::runtime_error("SSL error");
			if (!SSL_set_fd(this->ssl, this->sock))
				 throw std::runtime_error("Set FD error");

			SSL_set_connect_state(this->ssl);
			ERR_clear_error();

			if (SSL_connect(this->ssl) != VALUE_HANDSHAKE_OK)
				throw std::runtime_error("TLS handshake error");
		}

		n_sent = ftp::send_printf(this->sock, this->ssl,
		    "USER %s\r\nPASS %s\r\n%s", array[0], array[1],
		    (this->ssl ? "PBSZ 0\r\nPROT P\r\n" : ""));
		if (n_sent <= 0)
			throw std::runtime_error(_("Cannot send"));

		this->read_and_print(1);
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

	if (num == 125 || num == 150)
		return;
	printtext_print("none", "%s%s%s%d%s %s", b1, "FTP", sep, num, b2, text);
}

void
ftp_ctl_conn::printreps(void)
{
	for (const FTP_REPLY &rep : this->reply_vec)
		print_one_rep(rep.num, rep.text.c_str());
}

void
ftp_ctl_conn::read_and_print(const int timeo)
{
	if (this->sock == INVALID_SOCKET)
		return;
	while (this->read_reply(timeo))
		this->printreps();
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

	errno = 0;

	if (!has_three_digits(numstr)) {
		err_log(0, "%s: expected digits", __func__);
	}
#ifdef HAVE_BCI
	else if (sscanf_s(numstr, "%d%c", &num, &ch, (UINT)sizeof(char)) != 2 ||
		 ch != '-') {
		err_log(errno, "%s: sscanf_s() error", __func__);
	}
#else
	else if (sscanf(numstr, "%d%c", &num, &ch) != 2 || // NOLINT
		 ch != '-') {
		err_log(0, "%s: sscanf() error", __func__);
	}
#endif
	else {
		FTP_REPLY rep(num, token + len);

#if defined(__cplusplus) && __cplusplus >= 201103L
		reply_vec.emplace_back(rep);
#else
		reply_vec.push_back(rep);
#endif
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
	} else if (xsscanf(numstr, "%d", &num) != 1) {
		err_log(0, "%s: sscanf() error", __func__);
	} else {
		FTP_REPLY rep(num, token + len);

#if defined(__cplusplus) && __cplusplus >= 201103L
		reply_vec.emplace_back(rep);
#else
		reply_vec.push_back(rep);
#endif
	}
}

static void
process_reply(CSTRING token, std::vector<FTP_REPLY> &reply_vec)
{
	char		numstr[5] = { '\0' };
	const size_t	len = strspn(token, "0123456789-");

	if (strspn(token, "bcdlrswx-") == 10) {
		FTP_REPLY rep(1000, token);

#if defined(__cplusplus) && __cplusplus >= 201103L
		reply_vec.emplace_back(rep);
#else
		reply_vec.push_back(rep);
#endif
	} else if (len >= sizeof numstr) {
		err_log(0, "%s: initial segment too long", __func__);
	} else if (len == 4) {
		handle_length_four(numstr, token, len, reply_vec);
	} else if (len == 3) {
		handle_length_three(numstr, token, len, reply_vec);
	} else if (len == 0) {
		FTP_REPLY rep(0, token);

#if defined(__cplusplus) && __cplusplus >= 201103L
		reply_vec.emplace_back(rep);
#else
		reply_vec.push_back(rep);
#endif
	} else {
		err_log(0, "%s: unexpected length %zu: \"%s\"", __func__, len,
		    token);
	}
}

static inline bool
shall_assign_concat(CSTRING token,
    const std::string &last_token,
    const enum message_concat_state state)
{
	return (!last_token.empty() && state == CONCAT_BUFFER_IS_EMPTY &&
	    strings_match(last_token.c_str(), token));
}

static inline bool
shall_append_concat(const int loop_run, const enum message_concat_state state)
{
	return (loop_run == 0 && state == CONCAT_BUFFER_CONTAIN_DATA);
}

numrep_t
ftp_ctl_conn::read_reply(const int timeo)
{
	CSTRING				token;
	auto				tokstate = const_cast<STRING>("");
	int				loop_run = 0;
	network_recv_context		ctx(this->sock, 0, timeo, 345000);
	static chararray_t		sep = "\r\n";
	std::string			last_token("");

	if (this->sock == INVALID_SOCKET)
		return 0;

	if (this->ssl) {
		if (!recv_and_check_tls(&ctx, this->buf, sizeof this->buf, sep,
		    this->ssl))
			return 0;
	} else {
		if (!recv_and_check_plain(&ctx, this->buf, sizeof this->buf,
		    sep))
			return 0;
	}

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
    : full_path(nullptr)
    , path(nullptr)
    , filesz(-1)
    , fileptr(nullptr)
    , sock(INVALID_SOCKET)
    , ssl(nullptr)
    , ssl_ctx(nullptr)
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
    : full_path(nullptr)
    , path(nullptr)
    , filesz(-1)
    , fileptr(nullptr)
    , sock(INVALID_SOCKET)
    , ssl(nullptr)
    , ssl_ctx(nullptr)
    , host_str(nullptr)
    , port_str(nullptr)
    , state(CONCAT_BUFFER_IS_EMPTY)
    , res(nullptr)
    , port(0)
{
	static CSTRING format = " Entering Passive Mode "
	    "(%hhu,%hhu,%hhu,%hhu,%hhu,%hhu).";

	if (xsscanf(text, format,
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
	/*
	 * Close first...
	 */
	if (this->sock != INVALID_SOCKET) {
		ftp_closesocket(this->sock);
		this->sock = INVALID_SOCKET;
	}

	if (this->ssl != nullptr) {
		SSL_free(this->ssl);
		this->ssl = nullptr;
	}
	if (this->ssl_ctx != nullptr) {
		SSL_CTX_free(this->ssl_ctx);
		this->ssl_ctx = nullptr;
	}

	free(this->full_path);
	free(this->path);

	if (this->fileptr != nullptr) {
		if (fclose(this->fileptr) != 0)
			err_log(errno, "%s: fclose error", __func__);
		this->fileptr = nullptr;
	}

	free(this->host_str);
	free(this->port_str);

	if (this->res != nullptr) {
		freeaddrinfo(this->res);
		this->res = nullptr;
	}
}

/*
 * Copy assignment operator
 */
ftp_data_conn &
ftp_data_conn::operator=(const ftp_data_conn &obj)
{
	if (&obj == this)
		return *this;

	if (obj.full_path)
		this->full_path = sw_strdup(obj.full_path);
	if (obj.path)
		this->path = sw_strdup(obj.path);

	this->filesz = obj.filesz;

	if (!obj.vec.empty())
		this->vec.assign(obj.vec.begin(), obj.vec.end());
	if (!obj.message_concat.empty())
		this->message_concat.assign(obj.message_concat);

	this->fileptr = nullptr;
	this->sock    = obj.sock;
	this->ssl     = nullptr;
	this->ssl_ctx = nullptr;

	if (obj.host_str)
		this->host_str = sw_strdup(obj.host_str);
	if (obj.port_str)
		this->port_str = sw_strdup(obj.port_str);

	memmove(this->buf, obj.buf, sizeof(this->buf));

	this->state = obj.state;
	this->res   = nullptr;
	this->port  = obj.port;

	memmove(this->h, obj.h, sizeof(this->h));
	memmove(this->p, obj.p, sizeof(this->p));

	return *this;
}

/*
 * Copy constructor
 */
ftp_data_conn::ftp_data_conn(const ftp_data_conn &obj)
    : full_path(nullptr)
    , path(nullptr)
    , filesz(0)
    , fileptr(nullptr)
    , sock(INVALID_SOCKET)
    , ssl(nullptr)
    , ssl_ctx(nullptr)
    , host_str(nullptr)
    , port_str(nullptr)
    , state(CONCAT_BUFFER_IS_EMPTY)
    , res(nullptr)
    , port(0)
{
	if (obj.full_path)
		this->full_path = sw_strdup(obj.full_path);
	if (obj.path)
		this->path = sw_strdup(obj.path);

	this->filesz = obj.filesz;

	if (!obj.vec.empty())
		this->vec.assign(obj.vec.begin(), obj.vec.end());
	if (!obj.message_concat.empty())
		this->message_concat.assign(obj.message_concat);

	this->fileptr = nullptr;
	this->sock    = obj.sock;
	this->ssl     = nullptr;
	this->ssl_ctx = nullptr;

	if (obj.host_str)
		this->host_str = sw_strdup(obj.host_str);
	if (obj.port_str)
		this->port_str = sw_strdup(obj.port_str);

	memmove(this->buf, obj.buf, sizeof(this->buf));

	this->state = obj.state;
	this->res   = nullptr;
	this->port  = obj.port;

	memmove(this->h, obj.h, sizeof(this->h));
	memmove(this->p, obj.p, sizeof(this->p));
}

/*
 * Move assignment operator
 */
ftp_data_conn &
ftp_data_conn::operator=(ftp_data_conn &&obj) noexcept
{
	if (&obj == this)
		return *this;

	free(this->full_path);
	free(this->path);

	if (this->fileptr)
		(void) fclose(this->fileptr);
	if (this->sock != INVALID_SOCKET)
		ftp_closesocket(this->sock);
	if (this->ssl)
		SSL_free(this->ssl);
	if (this->ssl_ctx)
		SSL_CTX_free(this->ssl_ctx);
	if (this->res)
		freeaddrinfo(this->res);

	try {
		if (!obj.vec.empty())
			this->vec.assign(obj.vec.begin(), obj.vec.end());
		if (!obj.message_concat.empty())
			this->message_concat.assign(obj.message_concat);
	} catch (...) {
		err_quit("%s: unexpected exception thrown", __func__);
	}

	this->full_path = obj.full_path;
	this->path	= obj.path;
	this->filesz	= obj.filesz;
	this->fileptr	= obj.fileptr;
	this->sock	= obj.sock;
	this->ssl       = obj.ssl;
	this->ssl_ctx   = obj.ssl_ctx;
	this->host_str	= obj.host_str;
	this->port_str	= obj.port_str;

	memmove(this->buf, obj.buf, sizeof(this->buf));

	this->state = obj.state;
	this->res   = obj.res;
	this->port  = obj.port;

	memmove(this->h, obj.h, sizeof(this->h));
	memmove(this->p, obj.p, sizeof(this->p));

	/* ------------------------------------------------------------ */

	try {
		obj.vec.clear();
		obj.message_concat.clear();
	} catch (...) {
		err_quit("%s: unexpected exception thrown", __func__);
	}

	obj.full_path = nullptr;
	obj.path      = nullptr;
	obj.filesz    = 0;
	obj.fileptr   = nullptr;
	obj.sock      = INVALID_SOCKET;
	obj.ssl       = nullptr;
	obj.ssl_ctx   = nullptr;
	obj.host_str  = nullptr;
	obj.port_str  = nullptr;

	BZERO(obj.buf, sizeof(obj.buf));

	obj.state = CONCAT_BUFFER_IS_EMPTY;
	obj.res	  = nullptr;
	obj.port  = 0;

	BZERO(obj.h, sizeof(obj.h));
	BZERO(obj.p, sizeof(obj.p));

	return *this;
}

/*
 * Move constructor
 */
ftp_data_conn::ftp_data_conn(ftp_data_conn &&obj) noexcept
    : full_path(obj.full_path)
    , path(obj.path)
    , filesz(obj.filesz)
    , fileptr(obj.fileptr)
    , sock(obj.sock)
    , ssl(obj.ssl)
    , ssl_ctx(obj.ssl_ctx)
    , host_str(obj.host_str)
    , port_str(obj.port_str)
    , state(obj.state)
    , res(obj.res)
    , port(obj.port)
{
	try {
		if (!obj.vec.empty())
			this->vec.assign(obj.vec.begin(), obj.vec.end());
		if (!obj.message_concat.empty())
			this->message_concat.assign(obj.message_concat);
	} catch (...) {
		err_quit("%s: unexpected exception thrown", __func__);
	}

	memmove(this->buf, obj.buf, sizeof(this->buf));

	memmove(this->h, obj.h, sizeof(this->h));
	memmove(this->p, obj.p, sizeof(this->p));

	/* ------------------------------------------------------------ */

	try {
		obj.vec.clear();
		obj.message_concat.clear();
	} catch (...) {
		err_quit("%s: unexpected exception thrown", __func__);
	}

	obj.full_path = nullptr;
	obj.path      = nullptr;
	obj.filesz    = 0;
	obj.fileptr   = nullptr;
	obj.sock      = INVALID_SOCKET;
	obj.ssl       = nullptr;
	obj.ssl_ctx   = nullptr;
	obj.host_str  = nullptr;
	obj.port_str  = nullptr;

	BZERO(obj.buf, sizeof(obj.buf));

	obj.state = CONCAT_BUFFER_IS_EMPTY;
	obj.res	  = nullptr;
	obj.port  = 0;

	BZERO(obj.h, sizeof(obj.h));
	BZERO(obj.p, sizeof(obj.p));
}

bool
ftp_data_conn::connect_passive(void)
{
	struct sockaddr_in sin;

	if (this->sock != INVALID_SOCKET)
		ftp_closesocket(this->sock);
	if ((this->sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		printtext_print("err", "%s: %s", __func__,
		    _("Failed to create an endpoint for communication"));
		return false;
	}

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

static bool
zero_truncate(FILE *fileptr)
{
#if defined(UNIX)
	return (ftruncate(fileno(fileptr), 0) == 0);
#elif defined(WIN32)
	return ((errno = _chsize_s(fileno(fileptr), 0)) == 0);
#endif
}

static void
get_bytes(const std::string &str)
{
	CSTRING			bytes_str;
	STRING			str_copy = sw_strdup(str.c_str());
	auto			tokstate = const_cast<STRING>("");
	static chararray_t	sep	 = " \r\n";

	(void) strtok_r(str_copy, sep, &tokstate);
	(void) strtok_r(nullptr, sep, &tokstate);
	(void) strtok_r(nullptr, sep, &tokstate);
	(void) strtok_r(nullptr, sep, &tokstate);

	if ((bytes_str = strtok_r(nullptr, sep, &tokstate)) == nullptr ||
	    !is_numeric(bytes_str) ||
	    xsscanf(bytes_str, "%jd", &ftp::data_conn->filesz) != 1) {
		printtext_print("warn", "failed to get the file size");
		ftp::data_conn->filesz = -1;
	}

	free(str_copy);
}

static void
print_complete(CSTRING path, double part, double total, bool (&state)[3])
{
	double val;

	if (state[2])
		return;

	val = percentage(part, total);

	if (val >= 75.0 && !state[2]) {
		printtext_print("success", "%s: %.2f%% complete", path, val);
		state[2] = true;
	} else if (val >= 50.0 && !state[1]) {
		printtext_print("success", "%s: %.2f%% complete", path, val);
		state[1] = true;
	} else if (val >= 25.0 && !state[0]) {
		printtext_print("success", "%s: %.2f%% complete", path, val);
		state[0] = true;
	} else {
		napms(1);
	}
}

static int
recv_bytes_tls(struct network_recv_context *p_ctx, char *p_buf, size_t p_sz,
    SSL *p_ssl)
{
	char	*bufptr;
	int	 buflen;
	int	 bytes_received;

	if (p_ssl == nullptr ||
	    conn_is_closed(p_ssl) ||
	    p_ctx->sock == INVALID_SOCKET)
		return 0;
	else if (!SSL_pending(p_ssl)) {
		const int	maxfdp1 = p_ctx->sock + 1;
		fd_set		readset;
		struct timeval	tv;

		tv.tv_sec = p_ctx->sec;
		tv.tv_usec = p_ctx->microsec;

		FD_ZERO(&readset);
		FD_SET(p_ctx->sock, &readset);

		errno = 0;

		if (select(maxfdp1, &readset, nullptr, nullptr, &tv) ==
		    SOCKET_ERROR)
			return (errno == EINTR ? 0 : -1);
		else if (!FD_ISSET(p_ctx->sock, &readset))
			return 0;
	}

	BZERO(p_buf, p_sz);

	bufptr = p_buf;
	buflen = p_sz;
	bytes_received = 0;

	do {
		int ret;

		if (conn_is_closed(p_ssl))
			return -1;

		ERR_clear_error();

		if ((ret = SSL_read(p_ssl, bufptr, buflen)) > 0) {
			if (BIO_flush(SSL_get_rbio(p_ssl)) != 1)
				debug("%s: error flushing read bio", __func__);
			bytes_received += ret;
			bufptr += ret;
			buflen -= ret;
		} else {
			switch (SSL_get_error(p_ssl, ret)) {
			case SSL_ERROR_NONE:
				sw_assert_not_reached();
				break;
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
				debug("%s: want read / want write", __func__);
				break;
			default:
				return -1;
			}
		}

		UNUSED_VAR(bufptr);
		UNUSED_VAR(buflen);
	} while (false); /* buflen > 0 && SSL_pending(p_ssl) */

	return bytes_received;
}

void
ftp_data_conn::get_file(void)
{
	bool		proceed = true;
	bool		printed[3] = { false };
	char		unit = 'B';
	double		size = 0.0;
	int		bytes_received;
	intmax_t	total = 0;

	while (ftp::ctl_conn->read_reply(1)) {
		for (const FTP_REPLY &rep : ftp::ctl_conn->reply_vec) {
			if (rep.num == 211 || rep.num == 213) {
				/* null */;
			} else if (rep.num == 450 || rep.num == 550) {
				proceed = false;
				print_one_rep(rep.num, rep.text.c_str());
			} else if (rep.num == 1000) {
				get_bytes(rep.text);
//				print_one_rep(rep.num, rep.text.c_str());
			} else {
				print_one_rep(rep.num, rep.text.c_str());
			}
		}
	}

	if (!proceed)
		return;
	if (this->full_path == nullptr || this->path == nullptr)
		return;
	if ((this->fileptr = xfopen(this->full_path, "ab")) == nullptr) {
		printtext_print("err", "open failed: %s", this->full_path);
		return;
	}
	if (!zero_truncate(this->fileptr)) {
		printtext_print("err", "change size error");
		return;
	}

	struct network_recv_context recv_ctx(this->sock, 0, 3, 0);

	if (this->filesz != -1) {
		dcc::get_file_size(this->filesz, size, unit);
		printtext_print("success", "getting: %s (%.1f%c)...",
		    this->path, size, unit);
	} else
		printtext_print("success", "getting: %s...", this->path);

	(void) atomic_swap_bool(&ftp::loop_get_file, true);

	while (atomic_load_bool(&ftp::loop_get_file)) {
		if (!isValid(this->fileptr) || this->sock == INVALID_SOCKET)
			break;

		BZERO(this->buf, sizeof this->buf);

		if (this->ssl) {
			bytes_received = recv_bytes_tls(&recv_ctx, this->buf,
			    sizeof this->buf - 1, this->ssl);
		} else {
			bytes_received = net_recv_plain(&recv_ctx, this->buf,
			    sizeof this->buf - 1);
		}

		if (bytes_received > 0) {
			if (fwrite(this->buf, 1, bytes_received,
			    this->fileptr) !=
			    static_cast<size_t>(bytes_received)) {
				printtext_print("err", "file write error");
				break;
			} else
				total += bytes_received;
			if (this->filesz != -1) {
				print_complete(this->path,
				    TO_DBL(total), TO_DBL(this->filesz),
				    printed);
			}
		} else if (bytes_received == 0) {
			/* continue */;
		} else if (bytes_received < 0) {
			break;
		} else {
			sw_assert_not_reached();
		}
	}

	if (this->ssl)
		ftp::shutdown_ssl(this->ssl);

	ftp::ctl_conn->read_and_print(1);
	fclose_and_null(&this->fileptr);
	dcc::get_file_size(total, size, unit);
	printtext_print("success", "wrote: %s (%.1f%c)", this->full_path,
	    size, unit);
}

SOCKET
ftp_data_conn::get_sock(void) const
{
	return (this->sock);
}

SSL *
ftp_data_conn::get_ssl(void)
{
	return (this->ssl);
}

SSL_CTX *
ftp_data_conn::get_ssl_ctx(void)
{
	return (this->ssl_ctx);
}

numstr_t
ftp_data_conn::list_fetch(const int timeo)
{
	CSTRING				token;
	auto				tokstate = const_cast<STRING>("");
	int				loop_run = 0;
	network_recv_context		ctx(this->sock, 0, timeo, 233000);
	static chararray_t		sep = "\r\n";
	std::string			last_token("");

	if (this->sock == INVALID_SOCKET)
		return 0;

	if (this->ssl) {
		if (!recv_and_check_tls(&ctx, this->buf, sizeof this->buf, sep,
		    this->ssl))
			return 0;
	} else {
		if (!recv_and_check_plain(&ctx, this->buf, sizeof this->buf,
		    sep))
			return 0;
	}

	this->vec.clear();

	if (!is_terminated(this->buf, sizeof this->buf))
		last_token.assign(get_last_token(this->buf));

	if (this->state == CONCAT_BUFFER_CONTAIN_DATA &&
	    this->buf[0] == '\r' &&
	    this->buf[1] == '\n') {
		this->vec.push_back(this->message_concat);

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

#if defined(__cplusplus) && __cplusplus >= 201103L
		this->vec.emplace_back(token);
#else
		this->vec.push_back(token);
#endif
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

void
ftp_data_conn::send_file(void)
{
	bool		printed[3] = { false };
	bool		proceed = true;
	char		unit = 'B';
	double		size = 0.0;
	int		bytes_sent = 0;
	intmax_t	total = 0;
	size_t		bytes = 0;
	size_t		bytes_read = 0;
	uintmax_t	bytes_rem = this->filesz;

	while (ftp::ctl_conn->read_reply(3)) {
		for (const FTP_REPLY &rep : ftp::ctl_conn->reply_vec) {
			if (rep.num == 421 ||
			    rep.num == 450 ||
			    rep.num == 452 ||
			    rep.num == 500 ||
			    rep.num == 501 ||
			    rep.num == 530 ||
			    rep.num == 532 ||
			    rep.num == 550 ||
			    rep.num == 553)
				proceed = false;
			print_one_rep(rep.num, rep.text.c_str());
		}
	}

	if (!proceed)
		return;
	if (this->full_path == nullptr || this->path == nullptr)
		return;
	if ((this->fileptr = xfopen(this->full_path, "rb")) == nullptr) {
		printtext_print("err", "open failed: %s", this->full_path);
		return;
	}

	dcc::get_file_size(this->filesz, size, unit);
	printtext_print("success", "sending: %s (%.1f%c)...", this->path,
	    size, unit);

	(void) atomic_swap_bool(&ftp::loop_send_file, true);

	while (atomic_load_bool(&ftp::loop_send_file) &&
	    bytes_rem > 0 &&
	    total != this->filesz) {
		static const size_t bufsize = sizeof this->buf;

		bytes = (bytes_rem < bufsize ? bytes_rem : bufsize);
		if (!isValid(this->fileptr) || this->sock == INVALID_SOCKET)
			break;
		BZERO(this->buf, bufsize);

		if ((bytes_read =
		    fread(this->buf, 1, bytes, this->fileptr)) == 0) {
			printtext_print("err", _("%s: file read error"),
			    __func__);
			break;
		} else if ((bytes_sent = ftp::send_bytes(this->sock, this->ssl,
		    this->buf, size_to_int(bytes_read))) <= 0) {
			break;
		} else if (static_cast<unsigned int>(bytes_sent) !=
		    bytes_read) {
			printtext_print("err", _("%s: bytes sent mismatch "
			    "bytes read"), __func__);
			break;
		} else {
			bytes_rem -= bytes_sent;
			total += bytes_sent;

			print_complete(this->path,
			    TO_DBL(total), TO_DBL(this->filesz),
			    printed);
		}
	}

	if (this->ssl)
		ftp::shutdown_ssl(this->ssl);

	ftp::ctl_conn->read_and_print(1);
	fclose_and_null(&this->fileptr);
	dcc::get_file_size(total, size, unit);

	if (bytes_rem > 0 || total != this->filesz) {
		printtext_print("warn", "sent: %s (%.1f%c, incomplete)",
		    this->path, size, unit);
	} else {
		printtext_print("success", "sent: %s (%.1f%c)",
		    this->path, size, unit);
	}
}

bool
ftp_data_conn::shake_hands(void)
{
	if (this->ssl_ctx == nullptr) {
		if (!ftp::new_ssl_ctx(this->ssl_ctx))
			return false;
	}
	if (this->ssl == nullptr) {
		if ((this->ssl = SSL_new(this->ssl_ctx)) == nullptr)
			return false;
	}

	if (!SSL_set_fd(this->ssl, this->sock))
		return false;

	if (ftp::ctl_conn_sess != nullptr) {
		if (!SSL_set_session(this->ssl, ftp::ctl_conn_sess))
			debug("%s: SSL_set_session() error", __func__);
	} else
		debug("%s: no session", __func__);

	SSL_set_connect_state(this->ssl);
	ERR_clear_error();

	return (SSL_connect(this->ssl) == VALUE_HANDSHAKE_OK);
}

static bool
subcmd_ok(CSTRING cmd)
{
	if (strings_match(cmd, "cd"))
		return true;
	else if (strings_match(cmd, "del"))
		return true;
	else if (strings_match(cmd, "exit"))
		return true;
	else if (strings_match(cmd, "get"))
		return true;
	else if (strings_match(cmd, "login"))
		return true;
	else if (strings_match(cmd, "ls"))
		return true;
	else if (strings_match(cmd, "mkdir"))
		return true;
	else if (strings_match(cmd, "pwd"))
		return true;
	else if (strings_match(cmd, "rmdir"))
		return true;
	else if (strings_match(cmd, "send"))
		return true;
	else if (strings_match(cmd, "system"))
		return true;
	return false;
}

static void
perform_ftp_cmd(CSTRING cmd, CSTRING arg)
{
	SOCKET	 sock;
	SSL	*ssl;
	int	 n_sent;

	if (arg == nullptr || strings_match(arg, "")) {
		printtext_print("err", "%s", _("Insufficient arguments"));
		return;
	} else if (ftp::ctl_conn == nullptr) {
		printtext_print("err", "%s", _("No control connection"));
		return;
	} else if (ftp::ctl_conn->get_sock() == INVALID_SOCKET) {
		printtext_print("err", "%s", _("Invalid network socket"));
		return;
	}

	sock   = ftp::ctl_conn->get_sock();
	ssl    = ftp::ctl_conn->get_ssl();
	n_sent = ftp::send_printf(sock, ssl, "%s %s\r\n", cmd, arg);

	if (n_sent <= 0) {
		printtext_print("err", "%s", _("Cannot send"));
		return;
	}

	ftp::ctl_conn->read_and_print(0);
}

static void
perform_simple_ftp_cmd(CSTRING cmd)
{
	SOCKET	 sock;
	SSL	*ssl;
	int	 n_sent;

	if (ftp::ctl_conn == nullptr)
		return;
	else if (ftp::ctl_conn->get_sock() == INVALID_SOCKET) {
		printtext_print("err", "%s", _("Invalid network socket"));
		return;
	}

	sock   = ftp::ctl_conn->get_sock();
	ssl    = ftp::ctl_conn->get_ssl();
	n_sent = ftp::send_printf(sock, ssl, "%s", cmd);

	if (n_sent <= 0)
		return;

	ftp::ctl_conn->read_and_print(0);
}

static void
subcmd_cd(CSTRING pathname)
{
	perform_ftp_cmd("CWD", pathname);
}

static void
subcmd_del(CSTRING pathname)
{
	perform_ftp_cmd("DELE", pathname);
}

static void
subcmd_exit(void)
{
	SOCKET	 sock;
	SSL	*ssl;

	if (ftp::ctl_conn == nullptr)
		return;

	sock = ftp::ctl_conn->get_sock();
	ssl  = ftp::ctl_conn->get_ssl();

	ftp::set_timeout(sock, SO_SNDTIMEO, 1);

	if (ftp::data_conn) {
		(void) ftp::send_printf(sock, ssl, "ABOR\r\nQUIT\r\n");

		ftp::shutdown_sock(ftp::data_conn->get_sock());

		(void) atomic_swap_bool(&ftp::loop_get_file, false);
		(void) atomic_swap_bool(&ftp::loop_send_file, false);

		while (atomic_load_bool(&ftp::cmd_in_progress))
			napms(1);
		delete_data_conn();
	} else {
		(void) ftp::send_printf(sock, ssl, "QUIT\r\n");
	}

	ftp::ctl_conn->read_and_print(1);

	delete ftp::ctl_conn;
	ftp::ctl_conn = nullptr;
}

static void
subcmd_get(CSTRING path)
{
	SOCKET	 sock;
	SSL	*ssl;
	int	 n_sent;

	if (path == nullptr || strings_match(path, "")) {
		printtext_print("err", "%s", _("Insufficient arguments"));
		return;
	} else if (is_whitespace(path)) {
		printtext_print("err", "%s", _("Blank file path"));
		return;
	} else if (!ftp::passive()) {
		return;
	} else if (!ftp::data_conn->connect_passive()) {
		delete_data_conn();
		return;
	}

	ftp::data_conn->full_path = strdup_printf("%s%s%s", g_ftp_download_dir,
	    SLASH, path);
	ftp::data_conn->path = sw_strdup(path);

	sock   = ftp::ctl_conn->get_sock();
	ssl    = ftp::ctl_conn->get_ssl();
	n_sent = ftp::send_printf(sock, ssl,
	    "STAT %s\r\nTYPE L 8\r\nRETR %s\r\n", path, path);

	if (n_sent <= 0) {
		delete_data_conn();
		return;
	}

	if (ftp::secure()) {
		if (!ftp::data_conn->shake_hands()) {
			printtext_print("err", "%s: TLS handshake failed",
			    __func__);
			delete_data_conn();
			return;
		}
	}

	ftp::do_cmd_detached("get file");
}

static void
subcmd_login(void)
{
	if (ftp::ctl_conn) {
		printtext_print("err", "A control connection already exists. "
		    "/ftp exit?");
		return;
	}

	ftp::do_cmd_detached("login");
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
		printtext_print("err", "%s", _("Insufficient arguments"));
	} else if (strings_match(arg, "dir")) {
		ftp::do_cmd_detached("ls dir");
	} else if (strings_match(arg, "up")) {
		list_dir(ftp::get_upload_dir());
	} else if (strings_match(arg, "down")) {
		list_dir(g_ftp_download_dir);
	} else {
		printtext_print("err", "what? dir, uploads or downloads?");
	}
}

static void
subcmd_mkdir(CSTRING path)
{
	perform_ftp_cmd("MKD", path);
}

static void
subcmd_pwd(void)
{
	perform_simple_ftp_cmd("PWD\r\n");
}

static void
subcmd_rmdir(CSTRING path)
{
	perform_ftp_cmd("RMD", path);
}

static void
subcmd_send(CSTRING path)
{
	SOCKET		 sock;
	SSL		*ssl;
	int		 n_sent;
	struct stat	 sb = { 0 };

	if (path == nullptr || strings_match(path, "")) {
		printtext_print("err", "%s", _("Insufficient arguments"));
		return;
	} else if (is_whitespace(path)) {
		printtext_print("err", "%s", _("Blank file path"));
		return;
	} else if (!ftp::passive()) {
		return;
	} else if (!ftp::data_conn->connect_passive()) {
		delete_data_conn();
		return;
	}

	ftp::data_conn->full_path = strdup_printf("%s%s%s",
	    ftp::get_upload_dir(), SLASH, path);
	ftp::data_conn->path = sw_strdup(path);

	if (!is_regular_file(ftp::data_conn->full_path)) {
		printtext_print("err", "%s: file doesn't exist or isn't a "
		    "regular file", ftp::data_conn->full_path);
		delete_data_conn();
		return;
	} else if (check_path(ftp::get_upload_dir(), ftp::data_conn->full_path)
	    == ERR) {
		printtext_print("err", "check path error");
		delete_data_conn();
		return;
	} else if (stat(ftp::data_conn->full_path, &sb) != 0) {
		printtext_print("err", "%s: couldn't stat file",
		    ftp::data_conn->full_path);
		delete_data_conn();
		return;
	} else if (sb.st_size == 0) {
		printtext_print("err", "%s: zero size",
		    ftp::data_conn->full_path);
		delete_data_conn();
		return;
	} else {
		ftp::data_conn->filesz = static_cast<intmax_t>(sb.st_size);
	}

	sock   = ftp::ctl_conn->get_sock();
	ssl    = ftp::ctl_conn->get_ssl();
	n_sent = ftp::send_printf(sock, ssl, "TYPE L 8\r\nSTOR %s\r\n", path);

	if (n_sent <= 0) {
		delete_data_conn();
		return;
	}

	if (ftp::secure()) {
		if (!ftp::data_conn->shake_hands()) {
			printtext_print("err", "%s: TLS handshake failed",
			    __func__);
			delete_data_conn();
			return;
		}
	}

	ftp::do_cmd_detached("send file");
}

static void
subcmd_system(void)
{
	perform_simple_ftp_cmd("SYST\r\n");
}

/*
 * usage:
 *     /ftp cd <path>
 *     /ftp del <path>
 *     /ftp exit
 *     /ftp get <file>
 *     /ftp login
 *     /ftp ls [dir|up|down]
 *     /ftp mkdir <path>
 *     /ftp pwd
 *     /ftp rmdir <path>
 *     /ftp send <file>
 *     /ftp system
 */
void
cmd_ftp(CSTRING data)
{
	CSTRING			arg[2];
	CSTRING			subcmd;
	STRING			dcopy;
	auto			last = const_cast<STRING>("");
	static chararray_t	cmd = "/ftp";
	static chararray_t	sep = "\n";
	static const size_t	MAXARG = 300;

	if (strings_match(data, "")) {
		printtext_print("err", "%s", _("Insufficient arguments"));
		return;
	}

	dcopy = sw_strdup(data);
	(void) strFeed(dcopy, 1);

	if ((subcmd = strtok_r(dcopy, sep, &last)) == nullptr) {
		printf_and_free(dcopy, "%s: insufficient args", cmd);
		return;
	} else if (!subcmd_ok(subcmd)) {
		printf_and_free(dcopy, "%s: invalid subcommand '%s'", cmd,
		    subcmd);
		return;
	}

	if ((arg[0] = strtok_r(nullptr, sep, &last)) != nullptr) {
		if (strlen(arg[0]) > MAXARG) {
			printf_and_free(dcopy, "%s: arg too long (exceeds %zu)",
			    cmd, MAXARG);
			return;
		}
	}

	if (strings_match(subcmd, "cd"))
		subcmd_cd(arg[0]);
	else if (strings_match(subcmd, "del"))
		subcmd_del(arg[0]);
	else if (strings_match(subcmd, "exit"))
		subcmd_exit();
	else if (strings_match(subcmd, "get"))
		subcmd_get(arg[0]);
	else if (strings_match(subcmd, "login"))
		subcmd_login();
	else if (strings_match(subcmd, "ls"))
		subcmd_ls(arg[0]);
	else if (strings_match(subcmd, "mkdir"))
		subcmd_mkdir(arg[0]);
	else if (strings_match(subcmd, "pwd"))
		subcmd_pwd();
	else if (strings_match(subcmd, "rmdir"))
		subcmd_rmdir(arg[0]);
	else if (strings_match(subcmd, "send"))
		subcmd_send(arg[0]);
	else if (strings_match(subcmd, "system"))
		subcmd_system();
	else
		printtext_print("err", "%s: invalid subcommand", cmd);
	free(dcopy);
}

void
ftp_init(void)
{
	debug("%s called", __func__);
#ifdef WIN32
	winsock_init_doit();
#endif
}

void
ftp_deinit(void)
{
	delete ftp::ctl_conn;
	ftp::ctl_conn = nullptr;

	delete ftp::data_conn;
	ftp::data_conn = nullptr;
}

PTEXTBUF
get_list_of_matching_ftp_cmds(CSTRING search_var)
{
	PTEXTBUF	matches = textbuf_new();
	const size_t	varlen = strlen(search_var);

	for (auto cmd : ftp_cmds) {
		if (!strncmp(search_var, cmd, varlen))
			textbuf_emplace_back(__func__, matches, cmd, 0);
	}

	if (textbuf_size(matches) == 0) {
		textbuf_destroy(matches);
		return nullptr;
	}

	return matches;
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

static int
verify_callback(int ok, X509_STORE_CTX *ctx)
{
	if (!ok) {
		PRINTTEXT_CONTEXT ptext_ctx;
		X509 *cert = X509_STORE_CTX_get_current_cert(ctx);
		char issuer[256]  = { '\0' };
		char subject[256] = { '\0' };
		const int depth = X509_STORE_CTX_get_error_depth(ctx);
		const int err   = X509_STORE_CTX_get_error(ctx);

		if (err == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT ||
		    err == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN)
			return 1;

		(void) X509_NAME_oneline(X509_get_issuer_name(cert), issuer,
		    sizeof issuer);
		(void) X509_NAME_oneline(X509_get_subject_name(cert), subject,
		    sizeof subject);

		printtext_context_init(&ptext_ctx, g_status_window,
		    TYPE_SPEC1_WARN, true);

		printtext(&ptext_ctx, "Error with certificate at depth: %d",
		    depth);
		printtext(&ptext_ctx, "  issuer  = %s", issuer);
		printtext(&ptext_ctx, "  subject = %s", subject);
		printtext(&ptext_ctx, "Reason: %s",
		    X509_verify_cert_error_string(err));
	}

	return ok;
}

static void
set_ciphers_doit(SSL_CTX *ctx, const char *list, bool &ok)
{
	if (!SSL_CTX_set_cipher_list(ctx, list))
		ok = false;
}

static void
set_ciphers(SSL_CTX *ctx)
{
	bool		assignment_ok = true;
	immutable_cp_t	cs = Config("ftp_cipher_suite");

	if (strings_match(cs, "secure") || strings_match(cs, "SECURE"))
		set_ciphers_doit(ctx, g_suite_secure, assignment_ok);
	else if (strings_match(cs, "compat") || strings_match(cs, "COMPAT"))
		set_ciphers_doit(ctx, g_suite_compat, assignment_ok);
	else if (strings_match(cs, "legacy") || strings_match(cs, "LEGACY"))
		set_ciphers_doit(ctx, g_suite_legacy, assignment_ok);
	else if (strings_match(cs, "all") || strings_match(cs, "ALL"))
		set_ciphers_doit(ctx, g_suite_all, assignment_ok);
	else
		set_ciphers_doit(ctx, g_suite_compat, assignment_ok);

	if (!assignment_ok)
		throw std::runtime_error("no valid ciphers");
}

bool
ftp::new_ssl_ctx(SSL_CTX *&p_ssl_ctx)
{
	if (p_ssl_ctx != nullptr)
		return true;

	try {
		if ((p_ssl_ctx = SSL_CTX_new(TLS_client_method())) ==
		    nullptr)
			throw std::runtime_error("out of memory");

		std::string ca_file(g_home_dir);
		std::string certfile(g_home_dir);

		(void) ca_file.append(SLASH).append(PEM_FILE);
		(void) certfile.append(SLASH).append(CLIENT_PEM);

		if (is_regular_file(ca_file.c_str()) &&
		    !SSL_CTX_load_verify_locations(p_ssl_ctx, ca_file.c_str(),
		    nullptr))
			throw std::runtime_error("load verify locations error");
		if (!SSL_CTX_set_default_verify_paths(p_ssl_ctx))
			throw std::runtime_error("set default verify paths "
			    "error");
		if (is_regular_file(certfile.c_str())) {
			if (SSL_CTX_use_certificate_chain_file(p_ssl_ctx,
			    certfile.c_str()) != 1) {
				throw std::runtime_error("use certificate "
				    "chain file error");
			}
			if (SSL_CTX_use_PrivateKey_file(p_ssl_ctx,
			    certfile.c_str(), SSL_FILETYPE_PEM) != 1) {
				throw std::runtime_error("use private key file "
				    "error");
			}
		}

		SSL_CTX_set_verify(p_ssl_ctx, SSL_VERIFY_PEER,
		    verify_callback);
		SSL_CTX_set_verify_depth(p_ssl_ctx, 4);

		if (!SSL_CTX_set_min_proto_version(p_ssl_ctx,
		    TLS1_2_VERSION)) {
			throw std::runtime_error("error setting minimum "
			    "supported protocol version");
		}

		set_ciphers(p_ssl_ctx);
	} catch (const std::runtime_error &e) {
		if (p_ssl_ctx != nullptr) {
			SSL_CTX_free(p_ssl_ctx);
			p_ssl_ctx = nullptr;
		}

		printtext_print("warn", "%s: %s", __func__, e.what());
		return false;
	}

	return true;
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
	} else if ((sock = ftp::ctl_conn->get_sock()) == INVALID_SOCKET) {
		printtext_print("err", "%s: %s", __func__,
		    _("Invalid network socket"));
		return false;
	} else if (ftp::send_printf(sock, ftp::ctl_conn->get_ssl(),
	    "PASV\r\n") <= 0) {
		printtext_print("err", "%s: %s", __func__,
		    _("Cannot send"));
		return false;
	}

	while (ftp::ctl_conn->read_reply(0)) {
		for (const FTP_REPLY &rep : ftp::ctl_conn->reply_vec) {
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

bool
ftp::secure(void)
{
	return (config_bool("ftp_secure", false));
}

void
ftp::get_file(void)
{
	if (ftp::data_conn == nullptr)
		return;
	ftp::data_conn->get_file();
	delete_data_conn();
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

void
ftp::login(void)
{
	delete ftp::ctl_conn;
	ftp::ctl_conn = new ftp_ctl_conn();
	ftp::ctl_conn->login();
}

void
ftp::ls_dir(void)
{
	SOCKET	 sock;
	SSL	*ssl;
	int	 n_sent;

	if (!ftp::passive())
		return;
	if (!ftp::data_conn->connect_passive()) {
		delete_data_conn();
		return;
	}

	sock   = ftp::ctl_conn->get_sock();
	ssl    = ftp::ctl_conn->get_ssl();
	n_sent = ftp::send_printf(sock, ssl, "LIST\r\n");

	if (n_sent <= 0) {
		delete_data_conn();
		return;
	}

	if (ftp::secure()) {
		if (!ftp::data_conn->shake_hands()) {
			printtext_print("err", "%s: TLS handshake failed",
			    __func__);
			delete_data_conn();
			return;
		}
	}

	while (ftp::data_conn->list_fetch(3))
		ftp::data_conn->list_print();

	delete_data_conn();

	ftp::ctl_conn->read_and_print(1);
}

static int
send_bytes_plain(SOCKET p_sock, CSTRING p_buf, const int p_len)
{
	int bytes_sent;

	errno = 0;

#if defined(UNIX)
	if ((bytes_sent = send(p_sock, p_buf, p_len, 0)) == SOCKET_ERROR)
		return (errno == EAGAIN || errno == EWOULDBLOCK ? 0 : -1);
#elif defined(WIN32)
	if ((bytes_sent = send(p_sock, p_buf, p_len, 0)) == SOCKET_ERROR)
		return (WSAGetLastError() == WSAEWOULDBLOCK ? 0 : -1);
#endif

	return bytes_sent;
}

static int
send_bytes_tls(SOCKET p_sock, SSL *p_ssl, CSTRING p_buf, const int p_len)
{
	const char	*bufptr = nullptr;
	int		 buflen = 0;
	int		 bytes_sent = 0;

	if (p_ssl == nullptr ||
	    conn_is_closed(p_ssl) ||
	    p_sock == INVALID_SOCKET ||
	    strlen(p_buf) > INT_MAX)
		return -1;

	bufptr = p_buf;
	buflen = p_len;

	while (buflen > 0) {
		int ret;

		if (conn_is_closed(p_ssl))
			break;

		ERR_clear_error();

		if ((ret = SSL_write(p_ssl, bufptr, buflen)) > 0) {
			if (BIO_flush(SSL_get_wbio(p_ssl)) != 1)
				debug("%s: error flushing write bio", __func__);
			bytes_sent += ret;
			bufptr += ret;
			buflen -= ret;
		} else {
			switch (SSL_get_error(p_ssl, ret)) {
			case SSL_ERROR_NONE:
				sw_assert_not_reached();
				break;
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
				debug("%s: want read / want write", __func__);
				continue;
			}

			return -1;
		}
	}

	return bytes_sent;
}

int
ftp::send_bytes(SOCKET sock, SSL *ssl, CSTRING buf, const int len)
{
	return (ssl ? send_bytes_tls(sock, ssl, buf, len) :
	    send_bytes_plain(sock, buf, len));
}

void
ftp::send_file(void)
{
	if (ftp::data_conn == nullptr)
		return;
	ftp::data_conn->send_file();
	delete_data_conn();
}

int
ftp::send_printf(SOCKET sock, SSL *ssl, CSTRING fmt, ...) // NOLINT
{
	char	*buffer = nullptr;
	int	 n_sent = 0;
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

	if (ssl) {
		n_sent = send_bytes_tls(sock, ssl, buffer,
		    static_cast<int>(strlen(buffer)));
	} else {
		n_sent = send_bytes_plain(sock, buffer,
		    static_cast<int>(strlen(buffer)));
	}

	free_and_null(&buffer);
	return n_sent;
}

void
ftp::shutdown_sock(SOCKET sock)
{
	if (sock == INVALID_SOCKET)
		return;
#if defined(UNIX)
	if (shutdown(sock, SHUT_RDWR) == -1)
		err_log(errno, "%s: shutdown", __func__);
#elif defined(WIN32)
	if (shutdown(sock, SD_BOTH) != 0)
		err_log(errno, "%s: shutdown", __func__);
#endif
	ftp_closesocket(sock);
}

void
ftp::shutdown_ssl(SSL *ssl)
{
	if (ssl == nullptr)
		return;
	if (!(SSL_get_shutdown(ssl) & SSL_SENT_SHUTDOWN)) {
		switch (SSL_shutdown(ssl)) {
		case 0:
			debug("%s: SSL_shutdown: not yet finished", __func__);
			(void) SSL_shutdown(ssl);
			break;
		case 1:
			/* success! */
			break;
		default:
			err_log(0, "%s: SSL_shutdown: error", __func__);
			break;
		}
	}
}

bool
ftp::want_unveil_uploads(void)
{
	CSTRING		dir = Config("ftp_upload_dir");
	size_t		len[2] = { 0, 0 };

	if (strings_match(dir, ""))
		return false;

	len[0] = strlen(dir);
	len[1] = strlen(g_home_dir);

	if (len[0] >= len[1] && strncmp(dir, g_home_dir, MIN(len[0], len[1])) ==
	    STRINGS_MATCH)
		return false;
	return true;
}
