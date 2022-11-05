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

#include "dataClassify.h"
#include "errHand.h"
#include "identd.hpp"
#include "network.h"
#include "printtext.h"

SOCKET		 identd::sock = INVALID_SOCKET;
bool		 identd::listening = false;
bool		 identd::loop = false;
const char	*identd::name = "identd";

static void
clean_up_socket(SOCKET &sock)
{
	if (sock != INVALID_SOCKET) {
		errno = 0;

#if defined(UNIX)
		if (close(sock) != 0)
			err_log(errno, "%s: close", __func__);
#elif defined(WIN32)
		if (closesocket(sock) != 0) {
			err_log(errno, "%s: closesocket (error code = %d)",
			    __func__, WSAGetLastError());
		}
#endif

		sock = INVALID_SOCKET;
	}
}

static bool
query_chars_ok(const char *recvbuf, const int bytes_received)
{
	static const char legal_index[] = "\r\n ,0123456789";

	for (int i = 0; i < bytes_received; i++) {
		if (strchr(legal_index, recvbuf[i]) == NULL)
			return false;
	}

	return true;
}

void
identd::enter_loop(ident_client *cli)
{
	char recvbuf[20] = { '\0' };
	int bytes_received;
	struct network_recv_context ctx(cli->get_sock(), 0, 3, 0);

	printtext_print("success", "%s: %s connected", identd::name,
	    cli->get_ip());

	while (identd::loop) {
		if ((bytes_received = net_recv_plain(&ctx, &recvbuf[0],
		    sizeof recvbuf - 1)) < 0) {
			break;
		} else if (bytes_received > 0) {
			char *last = const_cast<char *>("");
			char *server_port, *client_port;
			static const char sep[] = "\r\n ,";

			if (!query_chars_ok(recvbuf, bytes_received)) {
				printtext_print("err", "%s: forbidden chars in "
				    "query", identd::name);
				break;
			}

			recvbuf[bytes_received] = '\0';

			server_port = strtok_r(&recvbuf[0], sep, &last);
			client_port = strtok_r(NULL, sep, &last);

			if (server_port == NULL || client_port == NULL ||
			    strtok_r(NULL, sep, &last) != NULL ||
			    !is_numeric(server_port) ||
			    !is_numeric(client_port) ||
			    *server_port == '0' || *client_port == '0') {
				printtext_print("err", "%s: invalid query",
				    identd::name);
				break;
			}

			printtext_print(NULL, "%s: server port: %s",
			    identd::name, server_port);
			printtext_print(NULL, "%s: client port: %s",
			    identd::name, client_port);
			break;
		}
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
		clean_up_socket(identd::sock);
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
		    <struct sockaddr *>(&cliaddr), &len)) < 0) {
			(void) napms(222);
			continue;
		}

		identd::com_with_client(new ident_client(clisock, cliaddr));
	}

	clean_up_socket(identd::sock);
	printtext_print("warn", "%s: stopped listening", identd::name);
}

void
identd::stop(void)
{
	printtext_print("warn", "%s: stopping daemon...", identd::name);

	identd::listening = false;
	identd::loop = false;

	if (identd::sock != INVALID_SOCKET) {
		errno = 0;

#if defined(UNIX)
		if (shutdown(identd::sock, SHUT_RDWR) != 0)
			err_log(errno, "%s: shutdown", identd::name);
#elif defined(WIN32)
		if (shutdown(identd::sock, SD_BOTH) != 0) {
			err_log(errno, "%s: shutdown (error code = %d)",
			    identd::name, WSAGetLastError());
		}
#endif
	}
}
