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
#include <stdexcept>
#include <vector>

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
#include "../tls-server.h"

#include "dcc.h"
#include "theme.h" /* url_to_file() */

#if WIN32
#define stat _stat
#endif

/****************************************************************
*                                                               *
*  ------------------ Structure definitions ------------------  *
*                                                               *
****************************************************************/

class dcc_get {
public:
	std::string nick;

	std::string	filename;
	intmax_t	filesize;

	dcc_get();
	dcc_get(const char *, const char *, intmax_t, uint32_t, uint16_t);

private:
	uint32_t	addr;
	uint16_t	port;
};

dcc_get::dcc_get()
{
	this->nick.assign("");
	this->filename.assign("");
	this->filesize = 0;
	this->addr = 0;
	this->port = 0;
}

dcc_get::dcc_get(const char *p_nick, const char *p_filename,
    intmax_t p_filesize, uint32_t p_addr, uint16_t p_port)
{
	this->nick.assign(p_nick);
	this->filename.assign(p_filename);
	this->filesize = p_filesize;
	this->addr = p_addr;
	this->port = htons(p_port);
}

class dcc_send {
public:
	std::string	nick;
	std::string	full_path;

	dcc_send();
	dcc_send(const char *, const std::string);
	~dcc_send();

	const char	*get_filename(void);
	intmax_t	 get_filesize(void);

private:
	char		 buf[255];
	struct stat	*sb;
};

dcc_send::dcc_send()
{
	this->nick.assign("");
	this->full_path.assign("");

	BZERO(this->buf, sizeof this->buf);
	this->sb = nullptr;
}

dcc_send::dcc_send(const char *p_nick, const std::string p_full_path)
{
	this->nick.assign(p_nick);
	this->full_path.assign(p_full_path);

	BZERO(this->buf, sizeof this->buf);
	this->sb = new struct stat;

	errno = 0;

	if (stat(p_full_path.c_str(), this->sb) != 0) {
		char strerrbuf[MAXERROR] = { '\0' };

		delete this->sb;
		throw std::runtime_error(xstrerror(errno, strerrbuf,
		    sizeof strerrbuf));
	}
}

dcc_send::~dcc_send()
{
	delete this->sb;
}

const char *
dcc_send::get_filename(void)
{
	char *cp, *full_path_copy;

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
dcc_send::get_filesize(void)
{
	if (this->sb == nullptr)
		return 0;
	return static_cast<intmax_t>(this->sb->st_size);
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

static std::vector<dcc_get>	get_db;
static std::vector<dcc_send>	send_db;

/****************************************************************
*                                                               *
*  ---------------------    Functions    ---------------------  *
*                                                               *
****************************************************************/

static bool
subcmd_ok(const char *subcmd)
{
	if (strings_match(subcmd, "close"))
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
subcmd_close()
{
}

static void
subcmd_get()
{
}

static void
subcmd_list()
{
}

static void
subcmd_send(const char *nick, const char *file)
{
	static const size_t	maxfile = 400;
	struct integer_context	intctx("dcc_port", 1024, 65535, 8080);

	if (nick == nullptr || file == nullptr) {
		printtext_print("err", "insufficient args");
		return;
	} else if (!is_valid_nickname(nick)) {
		printtext_print("err", "invalid nickname");
		return;
	} else if (xstrnlen(file, maxfile + 1) > maxfile) {
		printtext_print("err", "filename too long");
		return;
	}

	std::string full_path(dcc::get_upload_dir());

	full_path.append(SLASH);
	full_path.append(file);

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

		dcc_send send_obj(nick, full_path);
		send_db.push_back(send_obj);

		ret = net_send("PRIVMSG %s :\001SW_DCC SEND " "%" PRIu32 " %ld "
		    "%" PRIiMAX " %s\001",
		    nick,
		    addr,
		    config_integer(&intctx),
		    send_obj.get_filesize(),
		    send_obj.get_filename());
		if (ret < 0) {
			send_db.pop_back();
			throw std::runtime_error("cannot send");
		}
	} catch (const std::runtime_error &e) {
		printtext_print("err", "%s", e.what());
		return;
	}
}

/*
 * usage: /dcc [close|get|list|send] [args]
 */
void
cmd_dcc(const char *data)
{
	char			*dcopy;
	char			*last = const_cast<char *>("");
	char			*subcmd, *arg1, *arg2;
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

	if (strings_match(subcmd, "close"))
		subcmd_close();
	else if (strings_match(subcmd, "get"))
		subcmd_get();
	else if (strings_match(subcmd, "list"))
		subcmd_list();
	else if (strings_match(subcmd, "send"))
		subcmd_send(arg1, arg2);
	else
		printtext_print("err", "%s: invalid subcommand", cmd);
	free(dcopy);
}

void
dcc::init(void)
{
	if (config_bool("dcc", true)) {
		struct integer_context intctx("dcc_port", 1024, 65535, 8080);

		tls_server::begin(config_integer(&intctx));
	}
}

void
dcc::deinit(void)
{
	tls_server::end();

	if (get_db.size() > 0)
		get_db.clear();
	if (send_db.size() > 0)
		send_db.clear();
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

		url.append("ext_ip/");
		path.append(SLASH);
		path.append("ext_ip.tmp");

		url_to_file(url.c_str(), path.c_str());

		if (!file_exists(path.c_str()) ||
		    (fileptr = xfopen(path.c_str(), "r")) == nullptr ||
		    fgets(ext_ip, sizeof ext_ip, fileptr) == nullptr ||
		    strchr(ext_ip, '\n') == nullptr) {
			if (fileptr)
				fclose(fileptr);
			if (file_exists(path.c_str())) {
				if (remove(path.c_str()) != 0)
					err_log(errno, "%s: remove", __func__);
			}
			str.assign("");
			addr = INADDR_NONE;
			return false;
		}

		ext_ip[strcspn(ext_ip, "\n")] = '\0';
		fclose(fileptr);
		if (remove(path.c_str()) != 0)
			err_log(errno, "%s: remove", __func__);
		if ((addr = inet_addr(ext_ip)) == INADDR_NONE) {
			str.assign("");
			return false;
		}
		str.assign(ext_ip);
		return true;
	}

	if ((addr = inet_addr(own_ip)) == INADDR_NONE) {
		str.assign("");
		return false;
	}

	str.assign(own_ip);
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

void
dcc::handle_incoming_conn(SSL *ssl)
{
	block_signals();

	switch (SSL_accept(ssl)) {
	case 0:
		debug("%s: SSL_accept: The TLS/SSL handshake was not "
		    "successful", __func__);
		return;
	case 1:
		debug("%s: SSL_accept: The TLS/SSL handshake was successfully "
		    "completed", __func__);
		break;
	default:
		debug("%s: SSL_accept: The TLS/SSL handshake was not "
		    "successful because a fatal error occurred", __func__);
		return;
	}
}

bool
dcc::want_unveil_uploads(void)
{
	const char *dir = Config("dcc_upload_dir");

	if (strings_match(dir, ""))
		return false;
	else if (strncmp(dir, g_home_dir, strlen(g_home_dir)) == 0)
		return false;
	return true;
}
