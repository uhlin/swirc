/* Ident protocol daemon (RFC 1413)
   Copyright (C) 2022 Markus Uhlin. All rights reserved.

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

#include <stdexcept>

#include "identd.hpp"
#include "printtext.h"

SOCKET		 identd::sock = INVALID_SOCKET;
bool		 identd::listening = false;
bool		 identd::loop = false;
const char	*identd::name = "identd";

void
identd::enter_loop(ident_client *cli)
{
	printtext_print("success", "%s: %s connected", identd::name,
	    cli->get_ip());

	while (identd::loop) {
		/*
		 * TODO: Add code
		 */

		(void) napms(300);
	}

	if (cli->get_sock() != INVALID_SOCKET)
		xclosesocket(cli->get_sock());
	printtext_print("warn", "%s: %s disconnected", identd::name,
	    cli->get_ip());
}

void
identd::listen_on_port(const int port)
{
	try {
		struct sockaddr_in service;

		errno = 0;

		if ((identd::sock = socket(AF_INET, SOCK_STREAM, 0)) ==
		    INVALID_SOCKET) {
			throw std::runtime_error("unable to create an endpoint "
			    "for communication");
		}

		memset(&service, 0, sizeof service);
		service.sin_family = AF_INET;
		service.sin_port = htons(port);
		service.sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(identd::sock, reinterpret_cast<struct sockaddr *>
		    (&service), sizeof service) != 0) {
			throw std::runtime_error("unable to bind a name to "
			    "a socket");
		} else if (listen(identd::sock, 10) != 0) {
			throw std::runtime_error("unable to listen for "
			    "connections on a socket");
		}
	} catch (const std::runtime_error &e) {
		if (identd::sock != INVALID_SOCKET)
			xclosesocket(identd::sock);
		identd::sock = INVALID_SOCKET;
		printtext_print("err", "%s: %s", __func__, e.what());
		return;
	}

	printtext_print("success", "%s: listening on port %d", identd::name,
	    port);
	identd::listening = true;
	identd::loop = true;

	while (identd::listening) {
		SOCKET			clisock;
		struct sockaddr_in	cliaddr;
		socklen_t		len = sizeof cliaddr;

		if ((clisock = accept(identd::sock, reinterpret_cast
		    <struct sockaddr *>(&cliaddr), &len)) < 0)
			continue;

		identd::com_with_client(new ident_client(clisock, cliaddr));
	}

	printtext_print("warn", "%s: stopped listening", identd::name);
}

void
identd::stop(void)
{
	printtext_print("warn", "%s: stopping daemon...", identd::name);
	identd::listening = false;
	identd::loop = false;
}
