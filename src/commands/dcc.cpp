/* commands/dcc.cpp
   Copyright (C) 2024 Markus Uhlin. All rights reserved.

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

#include <sys/types.h>
#include <sys/stat.h>

#if UNIX
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <inttypes.h>
#if WIN32
#include <io.h>
#endif
#include <stdexcept>
#if UNIX
#include <unistd.h>
#endif
#include <utility>
#include <vector>

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
#include "../sig.h"
#include "../strHand.h"
#include "../theme.h"
#include "../tls-server.h"

#include "dcc.h"
#include "theme.h" /* url_to_file() */

#define DCC_FILE_MAX_SIZE	(g_one_gig * static_cast<intmax_t>(10))
#define DCC_FILE_REQ_SIZE	130
#define DCC_IO_BYTES		256

#if WIN32
#define stat _stat
#endif

/****************************************************************
*                                                               *
*  ------------------ Structure definitions ------------------  *
*                                                               *
****************************************************************/

dcc_get::dcc_get() : filesize(0)
    , bytes_rem(0)
    , size(0.0)
    , unit('B')
    , start(g_time_error)
    , stop(g_time_error)
    , fileptr(nullptr)
    , sock(INVALID_SOCKET)
    , ssl(nullptr)
    , ssl_ctx(nullptr)
    , addr(0)
    , port(0)
{
	this->nick.assign("");
	this->filename.assign("");
}

dcc_get::dcc_get(const char *p_nick,
    const char *p_filename,
    intmax_t p_filesize,
    uint32_t p_addr,
    uint16_t p_port) : filesize(p_filesize)
    , bytes_rem(p_filesize)
    , size(0.0)
    , unit('B')
    , start(g_time_error)
    , stop(g_time_error)
    , fileptr(nullptr)
    , sock(INVALID_SOCKET)
    , ssl(nullptr)
    , ssl_ctx(nullptr)
    , addr(p_addr)
    , port(htons(p_port))
{
	this->nick.assign(p_nick);
	this->filename.assign(p_filename);

	dcc::get_file_size(p_filesize, this->size, this->unit);
}

dcc_get::dcc_get(const dcc_get &obj) : nick(obj.nick)
    , filename(obj.filename)
    , filesize(obj.filesize)
    , bytes_rem(obj.bytes_rem)
    , size(obj.size)
    , unit(obj.unit)
    , start(g_time_error)
    , stop(g_time_error)
    , fileptr(nullptr)
    , sock(INVALID_SOCKET)
    , ssl(nullptr)
    , ssl_ctx(nullptr)
    , addr(obj.addr)
    , port(obj.port)
{
	debug("%s: copy constructor called", __func__);
}

dcc_get &dcc_get::operator=(const dcc_get &obj)
{
	if (&obj == this)
		return *this;

	this->nick       = obj.nick;
	this->filename   = obj.filename;
	this->filesize   = obj.filesize;
	this->bytes_rem  = obj.bytes_rem;
	this->size       = obj.size;
	this->unit       = obj.unit;
	this->start      = obj.start;
	this->stop       = obj.stop;
	this->fileptr    = nullptr;
	this->sock       = INVALID_SOCKET;
	this->ssl        = nullptr;
	this->ssl_ctx    = nullptr;
	this->addr       = obj.addr;
	this->port       = obj.port;

	debug("%s: copy assignment called", __func__);
	return *this;
}

dcc_get::~dcc_get()
{
	debug("%s: destructor called", __func__);

	try {
		this->destroy();
	} catch (...) {
		/* null */;
	}
}

static void
read_and_write(SSL *ssl, FILE *fp, intmax_t &bytes_rem)
{
	while (bytes_rem > 0) {
		char			buf[DCC_IO_BYTES] = { '\0' };
		int			ret;
		static const int	bufsize = static_cast<int>(sizeof buf);

		ERR_clear_error();

		if ((ret = SSL_read(ssl, addrof(buf[0]), (bytes_rem < bufsize ?
		    bytes_rem : bufsize))) > 0) {
			if (fwrite(addrof(buf[0]), 1, ret, fp) !=
			    static_cast<size_t>(ret))
				throw std::runtime_error("Write error");
			(void) fflush(fp);
			bytes_rem -= ret;
		} else {
			switch (SSL_get_error(ssl, ret)) {
			case SSL_ERROR_NONE:
				sw_assert_not_reached();
				break;
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
				debug("%s: want read / want write", __func__);
				break;
			default:
				const unsigned long int err =
				    ERR_peek_last_error();
				throw std::runtime_error(ERR_error_string(err,
				    nullptr));
			}
		}
	}
}

void
dcc_get::destroy(void)
{
	if (this->sock != INVALID_SOCKET) {
#if defined(UNIX)
		if (shutdown(this->sock, SHUT_RDWR) == -1)
			err_log(errno, "%s: shutdown", __func__);
		(void) close(this->sock);
#elif defined(WIN32)
		if (shutdown(this->sock, SD_BOTH) != 0)
			err_log(errno, "%s: shutdown", __func__);
		(void) closesocket(this->sock);
#endif
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
}

void
dcc_get::finalize_download(void)
{
	dcc::shutdown_conn(this->ssl);
}

void
dcc_get::get_file(void)
{
	if (!this->create_socket()) {
		printtext_print("err", "%s: Error creating the socket",
		    __func__);
		return;
	} else if (!this->create_ssl_ctx()) {
		printtext_print("err", "%s: Error creating the SSL context",
		    __func__);
		return;
	} else if (!this->create_ssl_obj()) {
		printtext_print("err", "%s: Error creating the SSL object",
		    __func__);
		return;
	}

	try {
		static const int VALUE_HANDSHAKE_OK = 1;
		struct sockaddr_in sin;

		memset(&sin, 0, sizeof sin);
		sin.sin_family		= AF_INET;
		sin.sin_port		= this->port;
		sin.sin_addr.s_addr	= this->addr;

		if (connect(this->sock, reinterpret_cast<struct sockaddr *>
		    (&sin), sizeof sin) == SOCKET_ERROR)
			throw std::runtime_error("Cannot connect");
		else if (!SSL_set_fd(this->ssl, this->sock))
			throw std::runtime_error("Set FD error");

		SSL_set_connect_state(this->ssl);

		if (SSL_connect(this->ssl) != VALUE_HANDSHAKE_OK)
			throw std::runtime_error("TLS/SSL handshake failed!");
		else if (this->request_file() == ERR)
			throw std::runtime_error("Send error");
		else if (g_dcc_download_dir == nullptr)
			throw std::runtime_error("Null dir");

		std::string path(g_dcc_download_dir);
		(void) path.append(SLASH).append(this->filename);

		if ((this->fileptr = xfopen(path.c_str(), "ab")) == nullptr)
			throw std::runtime_error("Open failed");

#if defined(UNIX)
		if (ftruncate(fileno(this->fileptr), 0) != 0)
			throw std::runtime_error("Change size error");
#elif defined(WIN32)
		if ((errno = _chsize_s(fileno(this->fileptr), 0)) != 0)
			throw std::runtime_error("Change size error");
#endif

		(void) fseek(this->fileptr, 0L, SEEK_END);

		this->start = time(nullptr);
		read_and_write(this->ssl, this->fileptr, this->bytes_rem);
		this->stop = time(nullptr);

		fclose_and_null(addrof(this->fileptr));
		printtext_print("success", "%s: wrote: %s", __func__,
		    path.c_str());
	} catch (const std::runtime_error &e) {
		fclose_and_null(addrof(this->fileptr));
		printtext_print("err", "%s: %s", __func__, e.what());
		return;
	}
}

double
dcc_get::get_percent(void) const
{
	double ret;

	if (this->bytes_rem < 0 || size == 0.0)
		return 0.0;
	ret = ((this->bytes_rem / size) * 100.0);
	return ret;
}

bool
dcc_get::has_completed(void) const
{
	if (!(this->filesize > 0))
		return false;
	return (this->bytes_rem == 0);
}

bool
dcc_get::create_socket(void)
{
	if (this->sock != INVALID_SOCKET)
		return true;
	else if ((this->sock = socket(AF_INET, SOCK_STREAM, 0)) ==
	    INVALID_SOCKET)
		return false;
	return true;
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

bool
dcc_get::create_ssl_ctx(void)
{
	if (this->ssl_ctx != nullptr)
		return true;

	try {
		if ((this->ssl_ctx = SSL_CTX_new(TLS_client_method())) ==
		    nullptr)
			throw std::runtime_error("out of memory");

		std::string ca_file(g_home_dir);
		std::string certfile(g_home_dir);

		(void) ca_file.append(SLASH).append(ROOT_PEM);
		(void) certfile.append(SLASH).append(CLIENT_PEM);

		if (!SSL_CTX_load_verify_locations(this->ssl_ctx,
		    ca_file.c_str(), nullptr)) {
			throw std::runtime_error("load verify locations error");
		} else if (!SSL_CTX_set_default_verify_paths(this->ssl_ctx)) {
			throw std::runtime_error("set default verify paths "
			    "error");
		} else if (SSL_CTX_use_certificate_chain_file(this->ssl_ctx,
		    certfile.c_str()) != 1) {
			throw std::runtime_error("use certificate chain file "
			    "error");
		} else if (SSL_CTX_use_PrivateKey_file(this->ssl_ctx,
		    certfile.c_str(), SSL_FILETYPE_PEM) != 1) {
			throw std::runtime_error("use private key file error");
		}

		SSL_CTX_set_verify(this->ssl_ctx, SSL_VERIFY_PEER,
		    verify_callback);
		SSL_CTX_set_verify_depth(this->ssl_ctx, 4);

		if (!SSL_CTX_set_min_proto_version(this->ssl_ctx,
		    TLS1_2_VERSION)) {
			throw std::runtime_error("error setting minimum "
			    "supported protocol version");
		}
	} catch (const std::runtime_error &e) {
		if (this->ssl_ctx != nullptr) {
			SSL_CTX_free(this->ssl_ctx);
			this->ssl_ctx = nullptr;
		}

		printtext_print("warn", "%s: %s", __func__, e.what());
		return false;
	}

	return true;
}

bool
dcc_get::create_ssl_obj(void)
{
	if (this->ssl != nullptr)
		return true;
	else if (this->ssl_ctx == nullptr)
		return false;
	else if ((this->ssl = SSL_new(this->ssl_ctx)) == nullptr)
		return false;
	return true;
}

int
dcc_get::request_file(void)
{
	char		 buf[DCC_FILE_REQ_SIZE];
	const char	*bufptr;
	int		 buflen;

	memset(buf, 0, sizeof buf);

	if (sw_strcpy(buf, g_my_nickname, sizeof buf) != 0 ||
	    sw_strcat(buf, "\r\n", sizeof buf) != 0 ||
	    sw_strcat(buf, this->filename.c_str(), sizeof buf) != 0)
		return ERR;

	bufptr = addrof(buf[0]);
	buflen = sizeof buf;

	while (buflen > 0) {
		int ret;

		ERR_clear_error();

		if ((ret = SSL_write(this->ssl, bufptr, buflen)) > 0) {
			if (BIO_flush(SSL_get_wbio(this->ssl)) != 1)
				debug("%s: error flushing write bio", __func__);
			bufptr += ret;
			buflen -= ret;
		} else {
			switch (SSL_get_error(this->ssl, ret)) {
			case SSL_ERROR_NONE:
				sw_assert_not_reached();
				break;
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
				debug("%s: want read / want write", __func__);
				continue;
			}

			return ERR;
		}
	}

	return OK;
}

class dcc_send {
public:
	std::string	 nick;
	std::string	 full_path;

	FILE		*fileptr;
	intmax_t	 bytes_rem;

	double	size;
	char	unit;

	dcc_send();
	dcc_send(const char *, const std::string);
	~dcc_send();

	const char	*get_filename(void);
	intmax_t	 get_filesize(void) const;
	double		 get_percent(void) const;
	bool		 has_completed(void) const;

private:
	char		 buf[255];
	struct stat	 sb;
};

dcc_send::dcc_send() : fileptr(nullptr)
    , bytes_rem(-1)
    , size(0.0)
    , unit('B')
{
	this->nick.assign("");
	this->full_path.assign("");

	BZERO(addrof(this->buf[0]), sizeof this->buf);
	BZERO(addrof(this->sb), sizeof this->sb);
}

dcc_send::dcc_send(const char *p_nick, const std::string p_full_path)
    : fileptr(nullptr)
    , bytes_rem(-1)
    , size(0.0)
    , unit('B')
{
	this->nick.assign(p_nick);
	this->full_path.assign(p_full_path);

	BZERO(addrof(this->buf[0]), sizeof this->buf);
	BZERO(addrof(this->sb), sizeof this->sb);

	errno = 0;

	if (stat(p_full_path.c_str(), addrof(this->sb)) != 0) {
		char strerrbuf[MAXERROR] = { '\0' };

		throw std::runtime_error(xstrerror(errno, strerrbuf,
		    sizeof strerrbuf));
	}

	dcc::get_file_size(this->get_filesize(), this->size, this->unit);
}

dcc_send::~dcc_send()
{
	fclose_and_null(addrof(this->fileptr));
}

const char *
dcc_send::get_filename(void)
{
	char		*full_path_copy;
	const char	*cp;

	if (!strings_match(this->buf, ""))
		return addrof(this->buf[0]);
	if (this->full_path.empty())
		return "";

	full_path_copy = sw_strdup(this->full_path.c_str());

	if ((cp = strrchr(full_path_copy, PATH_SEP)) == nullptr ||
	    sw_strcpy(this->buf, cp + 1, sizeof this->buf) != 0) {
		free(full_path_copy);
		return "";
	}

	free(full_path_copy);
	return addrof(this->buf[0]);
}

intmax_t
dcc_send::get_filesize(void) const
{
	return static_cast<intmax_t>(this->sb.st_size);
}

double
dcc_send::get_percent(void) const
{
	double ret;

	if (this->bytes_rem < 0 || size == 0.0)
		return 0.0;
	ret = ((this->bytes_rem / size) * 100.0);
	return ret;
}

bool
dcc_send::has_completed(void) const
{
	return (this->bytes_rem == 0);
}

/****************************************************************
*                                                               *
*  -------------- Objects with external linkage --------------  *
*                                                               *
****************************************************************/

const int	g_one_kilo = 1000;
const int	g_one_meg = 1000000;
const int	g_one_gig = 1000000000;

/****************************************************************
*                                                               *
*  -------------- Objects with internal linkage --------------  *
*                                                               *
****************************************************************/

static const std::vector<dcc_get>::size_type	GET_DB_MAX = 100;
static const std::vector<dcc_send>::size_type	SEND_DB_MAX = 100;

static std::vector<dcc_get>	get_db;
static std::vector<dcc_send>	send_db;

/****************************************************************
*                                                               *
*  ---------------------    Functions    ---------------------  *
*                                                               *
****************************************************************/

static bool
find_get_obj(const char *nick, const char *file, std::vector<dcc_get>::size_type
    &pos)
{
	pos = 0;

	for (dcc_get &x : get_db) {
		if (strings_match(x.nick.c_str(), nick) &&
		    strings_match(x.filename.c_str(), file))
			return true;

		pos++;
	}

	return false;
}

static bool
find_send_obj(const std::string &nick, const char *filename,
    std::vector<dcc_send>::size_type &pos)
{
	pos = 0;

	for (dcc_send &x : send_db) {
		if (x.nick.compare(nick) == 0 &&
		    strings_match(x.get_filename(), filename))
			return true;

		pos++;
	}

	return false;
}

static void
dup_check_get(const char *nick, const char *file)
{
	std::vector<dcc_get>::size_type pos = 0;

	if (!find_get_obj(nick, file, pos))
		return;
	else if (!get_db[pos].has_completed())
		throw std::runtime_error("a such nick/file already exists");
	else
		get_db.erase(get_db.begin() + pos);
}

static void
dup_check_send(const char *nick, const char *file)
{
	const std::string str(nick);
	std::vector<dcc_send>::size_type pos = 0;

	if (!find_send_obj(str, file, pos))
		return;
	else if (!send_db[pos].has_completed())
		throw std::runtime_error("already sending a such nick/file");
	else
		send_db.erase(send_db.begin() + pos);
}

static bool
subcmd_ok(const char *subcmd)
{
	if (strings_match(subcmd, "clear"))
		return true;
	else if (strings_match(subcmd, "get"))
		return true;
	else if (strings_match(subcmd, "list"))
		return true;
	else if (strings_match(subcmd, "send"))
		return true;
	return false;
}

static void
subcmd_clear()
{
}

static void
subcmd_get(const char *nick, const char *file)
{
	std::vector<dcc_get>::size_type pos = 0;

	if (!is_valid_nickname(nick)) {
		printtext_print("err", "%s: invalid nickname: %s", __func__,
		    nick);
		return;
	} else if (!is_valid_filename(file)) {
		printtext_print("err", "%s: invalid filename: %s", __func__,
		    file);
		return;
	} else if (!find_get_obj(nick, file, pos)) {
		printtext_print("err", "%s: no such nick/file", __func__);
		return;
	}

	dcc::get_file_detached(addrof(get_db[pos]));
}

static void
list_get(void)
{
	PRINTTEXT_CONTEXT	ctx;
	long int		objnum = 0;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC2, true);

	for (dcc_get &x : get_db) {
		printtext(&ctx, "----- %sGet object%s: %ld -----",
		    COLOR1, TXT_NORMAL, objnum);
		printtext(&ctx, "%sFrom%s: %s", COLOR2, TXT_NORMAL,
		    x.nick.c_str());
		printtext(&ctx, "%sName%s: %s", COLOR2, TXT_NORMAL,
		    x.filename.c_str());
		printtext(&ctx, "%sSize%s: %.1f%c", COLOR2, TXT_NORMAL,
		    x.size, x.unit);
		printtext(&ctx, "%sHas completed%s: %s (%.2f)",
		    COLOR2, TXT_NORMAL,
		    (x.has_completed() ? "Yes" : "No"),
		    x.get_percent());
		objnum++;
	}

	ctx.spec_type = TYPE_SPEC1_WARN;

	if (objnum == 0)
		printtext(&ctx, "Zero get objects...");
}

static void
list_send(void)
{
	PRINTTEXT_CONTEXT	ctx;
	long int		objnum = 0;

	printtext_context_init(&ctx, g_active_window, TYPE_SPEC2, true);

	for (dcc_send &x : send_db) {
		printtext(&ctx, "----- %sSend object%s: %ld -----",
		    COLOR1, TXT_NORMAL, objnum);
		printtext(&ctx, "%sTo%s:   %s", COLOR2, TXT_NORMAL,
		    x.nick.c_str());
		printtext(&ctx, "%sName%s: %s", COLOR2, TXT_NORMAL,
		    x.get_filename());
		printtext(&ctx, "%sSize%s: %.1f%c", COLOR2, TXT_NORMAL,
		    x.size, x.unit);
		printtext(&ctx, "%sHas completed%s: %s (%.2f)",
		    COLOR2, TXT_NORMAL,
		    (x.has_completed() ? "Yes" : "No"),
		    x.get_percent());
		objnum++;
	}

	ctx.spec_type = TYPE_SPEC1_WARN;

	if (objnum == 0)
		printtext(&ctx, "Zero send objects...");
}

static void
subcmd_list(const char *what)
{
	if (what == nullptr || strings_match(what, "")) {
		printtext_print("err", "insufficient args");
	} else if (strings_match(what, "get")) {
		list_get();
	} else if (strings_match(what, "send")) {
		list_send();
	} else if (strings_match(what, "all")) {
		list_get();
		list_send();
	} else {
		printtext_print("err", "what? get, send or all");
	}
}

static void
subcmd_send(const char *nick, const char *file)
{
	char			*nick_lc = nullptr;
	struct integer_context	 intctx("dcc_port", 1024, 65535, 8080);

	if (nick == nullptr || file == nullptr) {
		printtext_print("err", "insufficient args");
		return;
	} else if (!is_valid_nickname(nick)) {
		printtext_print("err", "invalid nickname");
		return;
	} else if (!is_valid_filename(file)) {
		printtext_print("err", "invalid filename");
		return;
	} else if (!(send_db.size() < SEND_DB_MAX)) {
		printtext_print("err", "database full");
		return;
	}

	std::string full_path(dcc::get_upload_dir());
	(void) full_path.append(SLASH).append(file);

	if (!file_exists(full_path.c_str())) {
		printtext_print("err", "file doesn't exist");
		return;
	} else if (!is_regular_file(full_path.c_str())) {
		printtext_print("err", "file isn't a regular file");
		return;
	}

	try {
		int		ret = -1;
		std::string	ext_ip("");
		uint32_t	addr = 0;

		if (!dcc::get_remote_addr(ext_ip, addr)) {
			throw std::runtime_error("error getting the remote "
			    "address");
		}

		nick_lc = strToLower(sw_strdup(nick));
		dup_check_send(nick_lc, file);

#if defined(__cplusplus) && __cplusplus >= 201103L
		dcc_send send_obj(nick_lc, std::move(full_path));
#else
		dcc_send send_obj(nick_lc, full_path);
#endif
		if (send_obj.get_filesize() > DCC_FILE_MAX_SIZE)
			throw std::runtime_error("too large file");
		send_db.push_back(send_obj);

		ret = net_send("PRIVMSG %s :\001SW_DCC SEND " "%" PRIu32 " %ld "
		    "%" PRIdMAX " %s\001",
		    nick_lc,
		    addr,
		    config_integer(&intctx),
		    send_obj.get_filesize(),
		    send_obj.get_filename());
		if (ret < 0) {
			send_db.pop_back();
			throw std::runtime_error("cannot send");
		}

		printtext_print("sp1", "sending '%s' to %s (%.1f%c)...",
		    send_obj.get_filename(),
		    nick,
		    send_obj.size,
		    send_obj.unit);
	} catch (const std::runtime_error &e) {
		printtext_print("err", "%s", e.what());
	} catch (...) {
		/* null */;
	}

	free(nick_lc);
}

/*
 * usage: /dcc [clear|get|list|send] [args]
 */
void
cmd_dcc(const char *data)
{
	char			*dcopy;
	char			*last = const_cast<char *>("");
	const char		*subcmd, *arg1, *arg2;
	static const char	 cmd[] = "/dcc";
	static const char	 sep[] = "\n";

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

	arg1 = strtok_r(nullptr, sep, &last);
	arg2 = strtok_r(nullptr, sep, &last);

	if (strings_match(subcmd, "clear"))
		subcmd_clear();
	else if (strings_match(subcmd, "get"))
		subcmd_get(arg1, arg2);
	else if (strings_match(subcmd, "list"))
		subcmd_list(arg1);
	else if (strings_match(subcmd, "send"))
		subcmd_send(arg1, arg2);
	else
		printtext_print("err", "%s: invalid subcommand", cmd);
	free(dcopy);
}

static bool
has_all_certs(void)
{
	std::string path[4];

	(void) path[0].assign(g_home_dir).append(SLASH).append(ROOT_PEM);
	(void) path[1].assign(g_home_dir).append(SLASH).append(SERVER_CA_PEM);
	(void) path[2].assign(g_home_dir).append(SLASH).append(SERVER_PEM);
	(void) path[3].assign(g_home_dir).append(SLASH).append(CLIENT_PEM);

	if (!file_exists(path[0].c_str()) || !file_exists(path[1].c_str()) ||
	    !file_exists(path[2].c_str()) || !file_exists(path[3].c_str()))
		return false;

	return true;
}

void
dcc::init(void)
{
	if (config_bool("dcc", true)) {
		struct integer_context intctx("dcc_port", 1024, 65535, 8080);

		if (!has_all_certs()) {
			printtext_print("warn", "Missing certs. Not starting "
			    "the DCC server. Please create them.");
			printtext_print("warn", "(%s, %s, %s and %s)", ROOT_PEM,
			    SERVER_CA_PEM, SERVER_PEM, CLIENT_PEM);
			return;
		}

		tls_server::begin(config_integer(&intctx));
	}
}

void
dcc::deinit(void)
{
	tls_server::end();

	if (!get_db.empty())
		get_db.clear();
	if (!send_db.empty())
		send_db.clear();
}

void
dcc::add_file(const char *nick, const char *user, const char *host,
    const char *data)
{
	char			*dcopy;
	char			*last = const_cast<char *>("");
	char			*nick_lc = nullptr;
	char			*token[4] = { nullptr };
	static const char	 sep[] = "\n";

	if (!is_valid_nickname(nick) || strings_match(data, ""))
		return;

	dcopy = sw_strdup(data);

	if (strFeed(dcopy, 3) != 3) {
		free(dcopy);
		return;
	}

	token[0] = strtok_r(dcopy, sep, &last);
	token[1] = strtok_r(nullptr, sep, &last);
	token[2] = strtok_r(nullptr, sep, &last);
	token[3] = strtok_r(nullptr, sep, &last);

	if (token[0] == nullptr ||
	    token[1] == nullptr ||
	    token[2] == nullptr ||
	    token[3] == nullptr) {
		free(dcopy);
		return;
	} else if (!is_numeric(token[0]) ||
	    !is_numeric(token[1]) ||
	    !is_numeric(token[2])) {
		free(dcopy);
		return;
	} else if (token[0][0] == '0' ||
	    token[1][0] == '0' ||
	    token[2][0] == '0') {
		free(dcopy);
		return;
	} else if (!is_valid_filename(token[3])) {
		free(dcopy);
		return;
	}

	try {
		uint32_t	addr = 0;
		uint16_t	port = 0;
		intmax_t	filesize = 0;

		if (!(get_db.size() < GET_DB_MAX))
			throw std::runtime_error("database full");
		else if (sscanf(token[0], "%" SCNu32, &addr) != 1)
			throw std::runtime_error("error getting the address");
		else if (sscanf(token[1], "%" SCNu16, &port) != 1)
			throw std::runtime_error("error getting the port");
		else if (sscanf(token[2], "%" SCNdMAX, &filesize) != 1)
			throw std::runtime_error("error getting the file size");
		else if (filesize > DCC_FILE_MAX_SIZE)
			throw std::runtime_error("too large file");

		nick_lc = strToLower(sw_strdup(nick));
		dup_check_get(nick_lc, token[3]);

		dcc_get get_obj(nick_lc, token[3], filesize, addr, port);
		get_db.push_back(get_obj);

		printtext_print("sp3", "%s: added: '%s' (%.1f%c)", __func__,
		    token[3], get_obj.size, get_obj.unit);
		printtext_print("sp3", "%s: from: %s <%s@%s>", __func__,
		    nick, user, host);
		printtext_print("sp2", "To get the file, type:");
		printtext_print("sp2", "  /dcc get %s %s", nick_lc, token[3]);
	} catch (const std::runtime_error &e) {
		printtext_print("err", "%s: %s", __func__, e.what());
		printtext_print("err", "%s: %s <%s@%s>", __func__, nick,
		    user, host);
	}

	free(dcopy);
	free(nick_lc);
}

void
dcc::get_file_size(const intmax_t bytes, double &size, char &unit)
{
	if (bytes <= 0 || bytes > DCC_FILE_MAX_SIZE) {
		size = 0.0;
		unit = 'B';
	} else if (bytes >= g_one_gig) {
		size = static_cast<double>(bytes) / g_one_gig;
		unit = 'G';
	} else if (bytes >= g_one_meg) {
		size = static_cast<double>(bytes) / g_one_meg;
		unit = 'M';
	} else if (bytes >= g_one_kilo) {
		size = static_cast<double>(bytes) / g_one_kilo;
		unit = 'K';
	} else {
		size = static_cast<double>(bytes);
		unit = 'B';
	}
}

bool
dcc::get_remote_addr(std::string &str, uint32_t &addr)
{
	const char *own_ip = Config("dcc_own_ip");

	if (strings_match(own_ip, "")) {
		FILE		*fileptr = nullptr;
		char		 ext_ip[30] = { '\0' };
		std::string	 url(g_swircWebAddr);
		std::string	 path(g_tmp_dir);

		(void) url.append("ext_ip/");
		(void) path.append(SLASH).append("ext_ip.tmp");

		url_to_file(url.c_str(), path.c_str());

		if (!file_exists(path.c_str()) ||
		    (fileptr = xfopen(path.c_str(), "r")) == nullptr ||
		    fgets(ext_ip, sizeof ext_ip, fileptr) == nullptr ||
		    strchr(ext_ip, '\n') == nullptr) {
			if (fileptr)
				(void) fclose(fileptr);
			if (file_exists(path.c_str())) {
				if (remove(path.c_str()) != 0)
					err_log(errno, "%s: remove", __func__);
			}

			(void) str.assign("");
			addr = INADDR_NONE;
			return false;
		}

		ext_ip[strcspn(ext_ip, "\n")] = '\0';
		(void) fclose(fileptr);

		if (remove(path.c_str()) != 0)
			err_log(errno, "%s: remove", __func__);
		if ((addr = inet_addr(ext_ip)) == INADDR_NONE) {
			(void) str.assign("");
			return false;
		}

		(void) str.assign(ext_ip);
		return true;
	}

	if ((addr = inet_addr(own_ip)) == INADDR_NONE) {
		(void) str.assign("");
		return false;
	}

	(void) str.assign(own_ip);
	return true;
}

const char *
dcc::get_upload_dir(void)
{
	static const char *dir;

	dir = Config("dcc_upload_dir");

	if (!is_directory(dir))
		return (g_dcc_upload_dir ? g_dcc_upload_dir : "");
	return dir;
}

static int
read_request(SSL *ssl, std::string &nick, std::string &filename)
{
	char			*bufptr;
	char			*last = const_cast<char *>("");
	char			*token[2] = { nullptr };
	char			 buf[DCC_FILE_REQ_SIZE] = { '\0' };
	int			 buflen;
	static const char	 sep[] = "\r\n";

	bufptr = addrof(buf[0]);
	buflen = sizeof buf;

	do {
		int ret;

		ERR_clear_error();

		if ((ret = SSL_read(ssl, bufptr, buflen)) > 0) {
			bufptr += ret;
			buflen -= ret;
		} else {
			switch (SSL_get_error(ssl, ret)) {
			case SSL_ERROR_NONE:
				sw_assert_not_reached();
				break;
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
				debug("%s: want read / want write", __func__);
				break;
			default:
				return ERR;
			}
		}
	} while (buflen > 0);

	if (memchr(buf, '\0', sizeof buf) == nullptr ||
	    (token[0] = strtok_r(buf, sep, &last)) == nullptr ||
	    (token[1] = strtok_r(nullptr, sep, &last)) == nullptr)
		return ERR;
	else if (!is_valid_nickname(token[0]))
		return ERR;
	else if (!is_valid_filename(token[1]))
		return ERR;

	(void) nick.assign(token[0]);
	(void) filename.assign(token[1]);

	return OK;
}

static int
send_bytes(SSL *ssl, const char *buf, const int bytes, intmax_t &bytes_rem)
{
	const char	*bufptr = buf;
	int		 buflen = bytes;

	while (buflen > 0) {
		int ret;

		ERR_clear_error();

		if ((ret = SSL_write(ssl, bufptr, buflen)) > 0) {
			if (BIO_flush(SSL_get_wbio(ssl)) != 1)
				debug("%s: error flushing write bio", __func__);
			bufptr += ret;
			buflen -= ret;
			bytes_rem -= ret;
		} else {
			switch (SSL_get_error(ssl, ret)) {
			case SSL_ERROR_NONE:
				sw_assert_not_reached();
				break;
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
				debug("%s: want read / want write", __func__);
				continue;
			}

			return ERR;
		}
	}

	return OK;
}

static int
accept_incoming(SSL *ssl)
{
	bool	loop = true;

	while (loop) {
		const int ret = SSL_accept(ssl);

		switch (ret) {
		case 0:
			debug("%s: SSL_accept: The TLS/SSL handshake was not "
			    "successful", __func__);
			return ERR;
		case 1:
			debug("%s: SSL_accept: The TLS/SSL handshake was "
			    "successfully completed", __func__);
			loop = false;
			break;
		default:
			if (SSL_get_error(ssl, ret) == SSL_ERROR_WANT_READ ||
			    SSL_get_error(ssl, ret) == SSL_ERROR_WANT_WRITE) {
				(void) napms(100);
				continue;
			}

			debug("%s: SSL_accept: The TLS/SSL handshake was not "
			    "successful because a fatal error occurred",
			    __func__);
			return ERR;
		}
	}

	return OK;
}

void
dcc::handle_incoming_conn(SSL *ssl)
{
	block_signals();

	if (accept_incoming(ssl) != OK)
		return;

	std::string nick("");
	std::string filename("");

	if (read_request(ssl, nick, filename) != OK) {
		printtext_print("warn", "%s: read request error", __func__);
		return;
	}

	std::vector<dcc_send>::size_type pos = 0;

	if (!find_send_obj(nick, filename.c_str(), pos)) {
		printtext_print("warn", "%s: unable to find the send object",
		    __func__);
		return;
	}

	dcc_send *send_obj = addrof(send_db[pos]);

	if (send_obj->bytes_rem == 0) {
		printtext_print("warn", "%s: already sent file", __func__);
		return;
	} else if (send_obj->fileptr == nullptr && (send_obj->fileptr =
	    xfopen(send_obj->full_path.c_str(), "rb")) == nullptr) {
		printtext_print("warn", "%s: file open error", __func__);
		return;
	}

	if (send_obj->bytes_rem == -1)
		send_obj->bytes_rem = send_obj->get_filesize();

	while (send_obj->bytes_rem > 0) {
		char			buf[DCC_IO_BYTES] = { '\0' };
		int			bytes;
		size_t			bytes_read;
		static const int	bufsize = static_cast<int>(sizeof buf);

		bytes = ((send_obj->bytes_rem < bufsize)
			 ? send_obj->bytes_rem
			 : bufsize);
		bytes_read = fread(buf, 1, bytes, send_obj->fileptr);

		if (bytes_read == 0) {
			fclose_and_null(addrof(send_obj->fileptr));
			printtext_print("err", "%s: file read error", __func__);
			return;
		} else if (send_bytes(ssl, addrof(buf[0]),
		    static_cast<int>(bytes_read), send_obj->bytes_rem) != OK) {
			fclose_and_null(addrof(send_obj->fileptr));
			printtext_print("err", "%s: tls write error", __func__);
			return;
		}
	}

	fclose_and_null(addrof(send_obj->fileptr));
	printtext_print("success", "%s: successfully sent file: %s", __func__,
	    filename.c_str());
	while (!(SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN))
		(void) napms(100);
	dcc::shutdown_conn(ssl);
	send_db.erase(send_db.begin() + pos);
}

void
dcc::shutdown_conn(SSL *ssl)
{
	if (SSL_get_shutdown(ssl) & SSL_SENT_SHUTDOWN) {
		debug("%s: already sent shutdown", __func__);
		return;
	}

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

bool
dcc::want_unveil_uploads(void)
{
	const char	*dir = Config("dcc_upload_dir");
	size_t		 len1, len2;

	if (strings_match(dir, "") || !is_directory(dir))
		return false;

	len1 = strlen(dir);
	len2 = strlen(g_home_dir);

	if (len1 >= len2 && strncmp(dir, g_home_dir, MIN(len1, len2)) ==
	    STRINGS_MATCH)
		return false;
	return true;
}
