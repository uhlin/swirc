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
#include <sys/stat.h>

#ifdef WIN32
#include <io.h>
#endif
#include <limits.h>
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

#include "connect.h"
#include "dcc.h" /* list_dir() */
#include "ftp.h"
#include "i18n.h"

#ifdef WIN32
#define stat _stat
#endif

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

volatile bool	 ftp::cmd_in_progress = false;
volatile bool	 ftp::loop_get_file = false;
volatile bool	 ftp::loop_send_file = false;

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

		this->read_and_print(1);

		ftp::set_timeout(sock, SO_SNDTIMEO, 4);
		n_sent = ftp::send_printf(this->sock, "USER %s\r\nPASS %s\r\n",
		    array[0],
		    array[1]);
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

	if (!has_three_digits(numstr)) {
		err_log(0, "%s: expected digits", __func__);
	} else if (sscanf(numstr, "%d%c", &num, &ch) != 2 ||
		   ch != '-') {
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

	if (this->sock == INVALID_SOCKET)
		return 0;

	RECV_AND_CHECK(333000);

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
	/*
	 * Close first...
	 */
	if (this->sock != INVALID_SOCKET) {
		ftp_closesocket(this->sock);
		this->sock = INVALID_SOCKET;
	}

	free(this->full_path);
	free(this->path);

	if (this->fileptr != nullptr) {
		fclose(this->fileptr);
		this->fileptr = nullptr;
	}

	free(this->host_str);
	free(this->port_str);

	if (this->res != nullptr) {
		freeaddrinfo(this->res);
		this->res = nullptr;
	}
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
	STRING			tokstate = const_cast<STRING>("");
	static chararray_t	sep	 = " \r\n";

	(void) strtok_r(str_copy, sep, &tokstate);
	(void) strtok_r(nullptr, sep, &tokstate);
	(void) strtok_r(nullptr, sep, &tokstate);
	(void) strtok_r(nullptr, sep, &tokstate);

	if ((bytes_str = strtok_r(nullptr, sep, &tokstate)) == nullptr ||
	    !is_numeric(bytes_str) ||
	    sscanf(bytes_str, "%jd", &ftp::data_conn->filesz) != 1) {
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
		napms(10);
	}
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
		bytes_received = net_recv_plain(&recv_ctx, this->buf,
		    sizeof this->buf - 1);

		if (bytes_received > 0) {
			if (fwrite(this->buf, 1, bytes_received,
			    this->fileptr) !=
			    static_cast<size_t>(bytes_received)) {
				printtext_print("err", "file write error");
				break;
			} else
				total += bytes_received;
			if (this->filesz != -1) {
				print_complete(this->path, total, this->filesz,
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

numstr_t
ftp_data_conn::list_fetch(const int timeo)
{
	CSTRING				token;
	STRING				tokstate = const_cast<STRING>("");
	int				loop_run = 0;
	static chararray_t		sep = "\r\n";
	std::string			last_token("");

	if (this->sock == INVALID_SOCKET)
		return 0;

	RECV_AND_CHECK(222000);

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

	while (ftp::ctl_conn->read_reply(1)) {
		for (const FTP_REPLY &rep : ftp::ctl_conn->reply_vec) {
			if (rep.num == 550 ||
			    rep.num == 553) // Permission denied
				proceed = false;
			print_one_rep(rep.num, rep.text.c_str());
		}
	}

	if (!proceed)
		return;
	if (this->full_path == nullptr || this->path == nullptr)
		return;
	if ((this->fileptr = xfopen(this->full_path, "r")) == nullptr) {
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

		if (bytes_rem > SIZE_MAX)
			err_exit(EOVERFLOW, "%s", __func__);
		bytes = (bytes_rem < bufsize ? bytes_rem : bufsize);
		if (!isValid(this->fileptr) || this->sock == INVALID_SOCKET)
			break;
		BZERO(this->buf, bufsize);

		if ((bytes_read =
		    fread(this->buf, 1, bytes, this->fileptr)) == 0) {
			printtext_print("err", _("%s: file read error"),
			    __func__);
			break;
		} else if ((bytes_sent = ftp::send_bytes(this->sock, this->buf,
		    bytes_read)) <= 0) {
			break;
		} else if (static_cast<unsigned int>(bytes_sent) !=
		    bytes_read) {
			printtext_print("err", _("%s: bytes sent mismatch "
			    "bytes read"), __func__);
			break;
		} else {
			bytes_rem -= bytes_sent;
			total += bytes_sent;

			print_complete(this->path, total, this->filesz,
			    printed);
		}
	}

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
	int n_sent;

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

	n_sent = ftp::send_printf(ftp::ctl_conn->get_sock(), "%s %s\r\n",
	    cmd, arg);
	if (n_sent <= 0) {
		printtext_print("err", "%s", _("Cannot send"));
		return;
	}

	ftp::ctl_conn->read_and_print(0);
}

static void
perform_simple_ftp_cmd(CSTRING cmd)
{
	int n_sent;

	if (ftp::ctl_conn == nullptr)
		return;
	else if (ftp::ctl_conn->get_sock() == INVALID_SOCKET) {
		printtext_print("err", "%s", _("Invalid network socket"));
		return;
	}

	n_sent = ftp::send_printf(ftp::ctl_conn->get_sock(), "%s", cmd);
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
	if (ftp::ctl_conn == nullptr)
		return;

	ftp::set_timeout(ftp::ctl_conn->get_sock(), SO_SNDTIMEO, 1);

	if (ftp::data_conn) {
		(void) ftp::send_printf(ftp::ctl_conn->get_sock(),
		    "ABOR\r\nQUIT\r\n");

		ftp::shutdown_sock(ftp::data_conn->get_sock());

		(void) atomic_swap_bool(&ftp::loop_get_file, false);
		(void) atomic_swap_bool(&ftp::loop_send_file, false);

		while (atomic_load_bool(&ftp::cmd_in_progress))
			napms(1);
		delete_data_conn();
	} else {
		(void) ftp::send_printf(ftp::ctl_conn->get_sock(), "QUIT\r\n");
	}

	ftp::ctl_conn->read_and_print(1);

	delete ftp::ctl_conn;
	ftp::ctl_conn = nullptr;
}

static void
subcmd_get(CSTRING path)
{
	int n_sent;

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

	n_sent = ftp::send_printf(ftp::ctl_conn->get_sock(),
	    "STAT %s\r\nTYPE L 8\r\nRETR %s\r\n", path, path);
	if (n_sent <= 0) {
		delete_data_conn();
		return;
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
	int		n_sent;
	struct stat	sb = { 0 };

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

	ftp::data_conn->full_path = strdup_printf("%s%s%s", g_ftp_upload_dir,
	    SLASH, path);
	ftp::data_conn->path = sw_strdup(path);

	if (!is_regular_file(ftp::data_conn->full_path)) {
		printtext_print("err", "%s: file doesn't exist or isn't a "
		    "regular file", ftp::data_conn->full_path);
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

	n_sent = ftp::send_printf(ftp::ctl_conn->get_sock(),
	    "TYPE L 8\r\nSTOR %s\r\n", path);
	if (n_sent <= 0) {
		delete_data_conn();
		return;
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
	STRING			last = const_cast<STRING>("");
	static chararray_t	cmd  = "/ftp";
	static chararray_t	sep  = "\n";

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

	arg[0] = strtok_r(nullptr, sep, &last);

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
	} else if ((sock = ftp::ctl_conn->get_sock()) == INVALID_SOCKET) {
		printtext_print("err", "%s: %s", __func__,
		    _("Invalid network socket"));
		return false;
	} else if (ftp::send_printf(sock, "PASV\r\n") <= 0) {
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
	if (ftp::ctl_conn != nullptr)
		delete ftp::ctl_conn;
	ftp::ctl_conn = new ftp_ctl_conn();
	ftp::ctl_conn->login();
}

void
ftp::ls_dir(void)
{
	int n_sent;

	if (!ftp::passive())
		return;
	if (!ftp::data_conn->connect_passive()) {
		delete_data_conn();
		return;
	}

	n_sent = ftp::send_printf(ftp::ctl_conn->get_sock(), "LIST\r\n");
	if (n_sent <= 0) {
		delete_data_conn();
		return;
	}

	while (ftp::data_conn->list_fetch(3))
		ftp::data_conn->list_print();

	delete_data_conn();

	ftp::ctl_conn->read_and_print(1);
}

int
ftp::send_bytes(SOCKET sock, const void *buf, const int len)
{
	int bytes_sent;

	errno = 0;

#if defined(UNIX)
	if ((bytes_sent = send(sock, buf, len, 0)) == SOCKET_ERROR)
		return (errno == EAGAIN || errno == EWOULDBLOCK ? 0 : -1);
#elif defined(WIN32)
	if ((bytes_sent = send(sock, static_cast<CSTRING>(buf), len, 0)) ==
	    SOCKET_ERROR)
		return (WSAGetLastError() == WSAEWOULDBLOCK ? 0 : -1);
#endif

	return bytes_sent;
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
	if ((n_sent = send(sock, buffer, size_to_int(strlen(buffer)), 0)) ==
	    SOCKET_ERROR) {
		free_and_null(&buffer);
		return (WSAGetLastError() == WSAEWOULDBLOCK ? 0 : -1);
	}
#endif

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
