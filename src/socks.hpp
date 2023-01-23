#ifndef SOCKS_HPP
#define SOCKS_HPP
/* socks.hpp  --  SOCKS 5 proxy client
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

#include <arpa/inet.h>

#include <string>
#include <vector>

#if defined(UNIX) && !defined(_SOCKET_DEFINED)
#define _SOCKET_DEFINED 1
typedef int SOCKET;
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#define SOCKS_NMETHODS	0x01
#define SOCKS_RSV	0x00
#define SOCKS_VER	0x05

typedef unsigned char socks_byte_t;

#define METHOD_NO_AUTH		0x00
#define METHOD_GSSAPI		0x01
#define METHOD_USER_PASS	0x02

#define CMD_CONNECT		0x01
#define CMD_BIND		0x02
#define CMD_UDP_ASSOCIATE	0x03

#define ATYP_DOMAINNAME		0x03
#define ATYP_IPV4_ADDR		0x01
#define ATYP_IPV6_ADDR		0x04

#define REP_SUCCEEDED			0x00
#define REP_GENERAL_FAILURE		0x01
#define REP_CONN_DISALL_BY_RULESET	0x02
#define REP_NETWORK_UNREACHABLE		0x03
#define REP_HOST_UNREACHABLE		0x04
#define REP_CONNECTION_REFUSED		0x05
#define REP_TTL_EXPIRED			0x06
#define REP_COMMAND_NOT_SUPP		0x07
#define REP_ADDRTYPE_NOT_SUPP		0x08

class socks_conn_req {
public:
	socks_conn_req();
	socks_conn_req(const char *, const char *, long int);

	std::vector<socks_byte_t> request;

private:
	socks_byte_t	 dst_port[2];
	socks_byte_t	 ipv4_addr[4];
	socks_byte_t	 ipv6_addr[16];
	uint16_t	 net16;
};

namespace socks
{
	int	 connect(const char *, const char *, std::string &);
	socks_byte_t
		 inttoatyp(const long int);
	void	 strError(socks_byte_t, std::string &);
	bool	 yesno(void);

	int	 read(SOCKET, socks_byte_t *, const int);
	int	 write(SOCKET, socks_byte_t *, const int);
}

#endif
