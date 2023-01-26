/* socks.cpp  --  SOCKS 5 proxy client
   Copyright (C) 2023 Markus Uhlin. All rights reserved.

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

#ifdef UNIX
#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netdb.h>
#endif

#include <inttypes.h>
#include <stdexcept>
#include <string.h>

#include "assertAPI.h"
#include "config.h"
#include "errHand.h"
#include "libUtils.h"
#include "network.h"
#include "printtext.h"
#include "socks.hpp"

#define RESERVED 0x00

static const size_t FQDN_MAX = 255;

/*
 * Send the version identifier
 */
static int
send_vid(std::string &err)
{
	socks_byte_t vid[] = { SOCKS_VER, SOCKS_NMETHODS, METHOD_NO_AUTH };

	if (socks::write(g_socket, &vid[0], sizeof vid) != sizeof vid) {
		err.assign(__func__).append(": send error");
		return -1;
	}
	err.assign("");
	return 0;
}

/*
 * Read the VID response
 */
static int
read_vid_resp(std::string &err)
{
	socks_byte_t resp[2] = { '\0', '\0' };

	if (socks::read(g_socket, &resp[0], sizeof resp) != sizeof resp) {
		(void) err.assign(__func__).append(": read error");
		return -1;
	} else if (resp[0] != SOCKS_VER) {
		(void) err.assign(__func__).append(": bad version");
		return -1;
	} else if (resp[1] != METHOD_NO_AUTH) {
		(void) err.assign(__func__).append(": bad method");
		return -1;
	}
	(void) err.assign("");
	return 0;
}

/*
 * Send the connection request
 */
static int
send_conn_req(const char *host, const char *port, std::string &err)
{
	struct integer_context	 intctx("socks_atyp", 0, 2, 0);

	try {
		socks_conn_req	 scr(host, port, config_integer(&intctx));
		const int	 size = static_cast<int>(scr.request.size());

		if (socks::write(g_socket, scr.request.data(), size) != size)
			throw std::runtime_error("send error");
	} catch (const std::runtime_error &e) {
		err.assign(__func__).append(": ").append(e.what());
		return -1;
	} catch (...) {
		err.assign(__func__).append(": ").append("unknown error");
		return -1;
	}

	return 0;
}

/*
 * Read the connection request response
 */
static int
read_conn_req_resp(std::string &err)
{
	socks_byte_t		 bnd_port[2] = { 0,0 };
	socks_byte_t		 preamble[4] = { 0,0,0,0 };
	std::string		 str("");
	uint16_t		 net16 = 0;

	if (socks::read(g_socket, &preamble[0], sizeof preamble) !=
	    sizeof preamble) {
		err.assign(__func__).append(": read error");
		return -1;
	} else if (preamble[0] != SOCKS_VER) {
		err.assign(__func__).append(": bad version");
		return -1;
	} else if (preamble[1] != REP_SUCCEEDED) {
		socks::strError(preamble[1], err);
		return -1;
	}

	if (preamble[3] == ATYP_DOMAINNAME) {
		err.assign(__func__).append(": not implemented");
		return -1;
	} else if (preamble[3] == ATYP_IPV4_ADDR) {
		char		 buf[INET_ADDRSTRLEN] = { '\0' };
		socks_byte_t	 bnd_addr[4] = { '\0' };

		if (socks::read(g_socket, &bnd_addr[0], sizeof bnd_addr) !=
		    sizeof bnd_addr) {
			err.assign(__func__).append(": read error (addr)");
			return -1;
		} else if (socks::read(g_socket, &bnd_port[0], sizeof bnd_port)
		    != sizeof bnd_port) {
			err.assign(__func__).append(": read error (port)");
			return -1;
		} else if (inet_ntop(AF_INET, &bnd_addr[0], buf, sizeof buf) ==
		    nullptr) {
			err.assign(__func__).append(": IPv4 addr error");
			return -1;
		}

		(void) memcpy(&net16, &bnd_port[0], sizeof(uint16_t));
		(void) str.assign(buf);
	} else if (preamble[3] == ATYP_IPV6_ADDR) {
		char		 buf[INET6_ADDRSTRLEN] = { '\0' };
		socks_byte_t	 bnd_addr[16] = { '\0' };

		if (socks::read(g_socket, &bnd_addr[0], sizeof bnd_addr) !=
		    sizeof bnd_addr) {
			err.assign(__func__).append(": read error (addr)");
			return -1;
		} else if (socks::read(g_socket, &bnd_port[0], sizeof bnd_port)
		    != sizeof bnd_port) {
			err.assign(__func__).append(": read error (port)");
			return -1;
		} else if (inet_ntop(AF_INET6, &bnd_addr[0], buf, sizeof buf) ==
		    nullptr) {
			err.assign(__func__).append(": IPv6 addr error");
			return -1;
		}

		(void) memcpy(&net16, &bnd_port[0], sizeof(uint16_t));
		(void) str.assign(buf);
	} else {
		err.assign(__func__).append(": unsupported address type");
		return -1;
	}

	printtext_print("success", "SOCKS success! (Bound to %s on port %"
	    PRIu16 ")", str.c_str(), ntohs(net16));
	return 0;
}

socks_conn_req::socks_conn_req()
{
	BZERO(this->dst_port, sizeof this->dst_port);
	BZERO(this->ipv4_addr, sizeof this->ipv4_addr);
	BZERO(this->ipv6_addr, sizeof this->ipv6_addr);
	this->net16 = 0;
}

static void
domainname(const char *host, std::vector<socks_byte_t> &fqdn,
    std::vector<socks_byte_t> &req)
{
	size_t len;

	if ((len = strlen(host)) > FQDN_MAX)
		len = FQDN_MAX;
	for (size_t i = 0; i < len; i++)
		fqdn.push_back(static_cast<socks_byte_t>(host[i]));

	req.push_back(SOCKS_VER);
	req.push_back(CMD_CONNECT);
	req.push_back(RESERVED);
	req.push_back(ATYP_DOMAINNAME);
	req.push_back(static_cast<socks_byte_t>(fqdn.size()));

	for (const socks_byte_t &b : fqdn)
		req.push_back(b);
}

socks_conn_req::socks_conn_req(const char *host, const char *port, long int li)
{
#define PB_DST_PORT()\
	do {\
		(void) memcpy(this->dst_port, &this->net16,\
		    sizeof this->dst_port);\
		this->request.push_back(this->dst_port[0]);\
		this->request.push_back(this->dst_port[1]);\
	} while (0)
	long int	 val = 0;
	uint16_t	 host16 = 0;

	BZERO(this->dst_port, sizeof this->dst_port);
	sw_static_assert(sizeof this->dst_port == sizeof(uint16_t),
	    "sizes mismatch");

	BZERO(this->ipv4_addr, sizeof this->ipv4_addr);
	BZERO(this->ipv6_addr, sizeof this->ipv6_addr);

	if (!getval_strtol(port, 0, UINT16_MAX, &val))
		throw std::runtime_error("bad port");
	host16 = static_cast<uint16_t>(val);
	this->net16 = htons(host16);

	if (socks::inttoatyp(li) == ATYP_DOMAINNAME) {
		domainname(host, this->fqdn, this->request);
		PB_DST_PORT();
	} else if (socks::inttoatyp(li) == ATYP_IPV4_ADDR) {
		/*
		 * IPv4
		 */

		if (inet_pton(AF_INET, host, this->ipv4_addr) != 1)
			throw std::runtime_error("IPv4 addr conversion failed");

		this->request.push_back(SOCKS_VER);
		this->request.push_back(CMD_CONNECT);
		this->request.push_back(RESERVED);
		this->request.push_back(ATYP_IPV4_ADDR);

		for (const socks_byte_t &b : this->ipv4_addr)
			this->request.push_back(b);
		PB_DST_PORT();
	} else if (socks::inttoatyp(li) == ATYP_IPV6_ADDR) {
		/*
		 * IPv6
		 */

		if (inet_pton(AF_INET6, host, this->ipv6_addr) != 1)
			throw std::runtime_error("IPv6 addr conversion failed");

		this->request.push_back(SOCKS_VER);
		this->request.push_back(CMD_CONNECT);
		this->request.push_back(RESERVED);
		this->request.push_back(ATYP_IPV6_ADDR);

		for (const socks_byte_t &b : this->ipv6_addr)
			this->request.push_back(b);
		PB_DST_PORT();
	} else
		throw std::runtime_error("invalid address type");
}

int
socks::connect(const char *host, const char *port, std::string &err)
{
	if (send_vid(err) == -1 || read_vid_resp(err) == -1)
		return -1;
	else if (send_conn_req(host, port, err) == -1 ||
	    read_conn_req_resp(err) == -1)
		return -1;
	return 0;
}

socks_byte_t
socks::inttoatyp(const long int li)
{
	if (li == 0)
		return ATYP_DOMAINNAME;
	else if (li == 1)
		return ATYP_IPV4_ADDR;
	else if (li == 2)
		return ATYP_IPV6_ADDR;
	err_log(EINVAL, "%s: bad address type %ld", __func__, li);
	return ATYP_IPV4_ADDR;
}

void
socks::strError(socks_byte_t code, std::string &msg)
{
	if (code == REP_SUCCEEDED)
		(void)msg.assign("Succeeded");
	else if (code == REP_GENERAL_FAILURE)
		(void)msg.assign("General SOCKS server failure");
	else if (code == REP_CONN_DISALL_BY_RULESET)
		(void)msg.assign("Connection not allowed by ruleset");
	else if (code == REP_NETWORK_UNREACHABLE)
		(void)msg.assign("Network unreachable");
	else if (code == REP_HOST_UNREACHABLE)
		(void)msg.assign("Host unreachable");
	else if (code == REP_CONNECTION_REFUSED)
		(void)msg.assign("Connection refused");
	else if (code == REP_TTL_EXPIRED)
		(void)msg.assign("TTL expired");
	else if (code == REP_COMMAND_NOT_SUPP)
		(void)msg.assign("Command not supported");
	else if (code == REP_ADDRTYPE_NOT_SUPP)
		(void)msg.assign("Address type not supported");
	else
		(void)msg.assign("Unknown error");
}

bool
socks::yesno(void)
{
	return config_bool("socks", false);
}

int
socks::read(SOCKET sock, void *buf, const int len)
{
	int nread;

#if defined(UNIX)
	if ((nread = recv(sock, buf, len, 0)) == SOCKET_ERROR)
		return -1;
#elif defined(WIN32)
	if ((nread = recv(sock, static_cast<char *>(buf), len, 0)) ==
	    SOCKET_ERROR)
		return -1;
#endif
	return nread;
}

int
socks::write(SOCKET sock, const void *buf, const int len)
{
	int nwritten;

#if defined(UNIX)
	if ((nwritten = send(sock, buf, len, 0)) == SOCKET_ERROR)
		return -1;
#elif defined(WIN32)
	if ((nwritten = send(sock, static_cast<const char *>(buf), len, 0)) ==
	    SOCKET_ERROR)
		return -1;
#endif
	return nwritten;
}
