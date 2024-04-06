/* Ident protocol daemon (RFC 1413)
   Copyright (C) 2022-2024 Markus Uhlin. All rights reserved.

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

#if UNIX
#include <sys/select.h>
#endif

#include <inttypes.h>
#if !defined(BSD) && !defined(WIN32)
#include <random>
#endif
#include <stdexcept>

#if WIN32
extern "C" {
#include "compat/stdlib.h" /* arc4random_uniform() */
}
#endif

#include "config.h"
#include "dataClassify.h"
#include "errHand.h"
#include "identd.hpp"
#include "libUtils.h"
#include "main.h"
#include "nestHome.h"
#include "network.h"
#include "printtext.h"
#include "sig.h"
#include "strHand.h"
#include "strdup_printf.h"

char		 identd::fakename[FAKENAME_LEN] = { '\0' };
volatile bool	 identd::listening = false;
volatile bool	 identd::loop = false;
const char	*identd::name = "identd";
SOCKET		 identd::sock = INVALID_SOCKET;

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

static inline int
get_maxfdp1(void)
{
#if defined(UNIX)
	return (identd::sock + 1);
#elif defined(WIN32)
	return -1;
#endif
}

//lint -sem(get_servport, r_null)
static char *
get_servport(void)
{
	socklen_t len;
	struct sockaddr_storage ss;

	len = sizeof ss;
	memset(&ss, 0, len);

	if (g_socket == INVALID_SOCKET || getsockname(g_socket, reinterpret_cast
	    <struct sockaddr *>(&ss), &len) != 0)
		return nullptr;
	else if (ss.ss_family == AF_INET) {
		struct sockaddr_in *sin;

		sin = reinterpret_cast<struct sockaddr_in *>(&ss);
		return strdup_printf("%" PRIu16, ntohs(sin->sin_port));
	} else if (ss.ss_family == AF_INET6) {
		struct sockaddr_in6 *sin6;

		sin6 = reinterpret_cast<struct sockaddr_in6 *>(&ss);
		return strdup_printf("%" PRIu16, ntohs(sin6->sin6_port));
	}
	return nullptr;
}

static struct sockaddr *
get_sockaddr(const int port, socklen_t &size)
{
	static struct sockaddr_in	sin;
	static struct sockaddr_in6	sin6;

	if (g_socket_address_family != AF_INET6) {
		memset(&sin, 0, sizeof sin);
		sin.sin_family		= AF_INET;
		sin.sin_port		= htons(port);
		sin.sin_addr.s_addr	= INADDR_ANY;
		size = sizeof sin;
		return reinterpret_cast<struct sockaddr *>(&sin);
	}

	memset(&sin6, 0, sizeof sin6);
	sin6.sin6_family	= AF_INET6;
	sin6.sin6_port		= htons(port);
	sin6.sin6_addr		= in6addr_any;
	size = sizeof sin6;
	return reinterpret_cast<struct sockaddr *>(&sin6);
}

static inline int
get_sockdomain(void)
{
	return (g_socket_address_family != AF_INET6 ? AF_INET : AF_INET6);
}

static const char *
get_username(void)
{
	static char *username;

	if ((username = g_cmdline_opts->username) != nullptr)
		return username;
	else if ((username = Config_mod("username")) != nullptr &&
	    !strings_match(username, ""))
		return username;
	else if ((username = g_user) != nullptr && !strings_match(username, ""))
		return username;
	return "noname";
}

static const char *
get_username_fake(void)
{
	static const char legal_index[] =
	    "0123456789"
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    "abcdefghijklmnopqrstuvwxyz";

#if defined(BSD) || defined(WIN32)
	for (size_t i = 0; i < ARRAY_SIZE(identd::fakename); i++) {
		identd::fakename[i] = legal_index[arc4random_uniform
		    (sizeof legal_index - 1)];
	}
#else
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<size_t> dist(0, strlen(legal_index) - 1);

	for (size_t i = 0; i < ARRAY_SIZE(identd::fakename); i++)
		identd::fakename[i] = legal_index[dist(gen)];
#endif

	identd::fakename[ARRAY_SIZE(identd::fakename) - 1] = '\0';
	debug("%s: \"%s\"", __func__, &identd::fakename[0]);
	return &identd::fakename[0];
}

static void
handle_ident_query(const char *server_port, const char *client_port,
    ident_client *cli)
{
	char *port;

	if ((port = get_servport()) == nullptr)
		err_log(0, "%s: get_servport() error", __func__);
	else if (strings_match(port, server_port) && strings_match(client_port,
	    g_last_port))
		identd::send_response(server_port, client_port, cli);
	else
		identd::send_err_resp(server_port, client_port, cli);
	free(port);
}

static bool
query_chars_ok(const char *recvbuf, const int bytes_received)
{
	static const char legal_index[] = "\r\n ,0123456789";

	for (int i = 0; i < bytes_received; i++) {
		if (strchr(legal_index, recvbuf[i]) == nullptr)
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
	block_signals();

	while (identd::loop) {
		if ((bytes_received = net_recv_plain(&ctx, &recvbuf[0],
		    sizeof recvbuf - 1)) < 0) {
			break;
		} else if (bytes_received > 0) {
			char *last = const_cast<char *>("");
			const char *server_port, *client_port;
			static const char sep[] = "\r\n ,";

			if (!query_chars_ok(recvbuf, bytes_received)) {
				printtext_print("err", "%s: forbidden chars in "
				    "query", identd::name);
				break;
			}

			recvbuf[bytes_received] = '\0';

			server_port = strtok_r(&recvbuf[0], sep, &last);
			client_port = strtok_r(nullptr, sep, &last);

			if (server_port == nullptr || client_port == nullptr ||
			    strtok_r(nullptr, sep, &last) != nullptr ||
			    !is_numeric(server_port) ||
			    !is_numeric(client_port) ||
			    *server_port == '0' || *client_port == '0') {
				printtext_print("err", "%s: invalid query",
				    identd::name);
				break;
			}

			handle_ident_query(server_port, client_port, cli);
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
		struct sockaddr *sa;
		socklen_t sa_size = 0;

		if (identd::start_pre_check() == -1) {
			throw std::runtime_error("already listening");
		} else if ((identd::sock = socket(get_sockdomain(), SOCK_STREAM,
		    0)) == INVALID_SOCKET) {
			throw std::runtime_error("unable to create an endpoint "
			    "for communication");
		}

		identd::set_reuseaddr(identd::sock);
		sa = get_sockaddr(port, sa_size);

		if (bind(identd::sock, sa, sa_size) != 0) {
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
	} catch (...) {
		clean_up_socket(identd::sock);
		printtext_print("err", "%s: %s", __func__, "unknown exception");
		return;
	}

	printtext_print("success", "%s: listening on port %d", identd::name,
	    port);
	block_signals();
	identd::listening = true;
	identd::loop = true;

	while (identd::listening) {
		SOCKET			clisock;
		fd_set			readset;
		socklen_t		len = sizeof(struct sockaddr_storage);
		struct sockaddr_storage	cliaddr;
		struct timeval		tv = { 0 };

		FD_ZERO(&readset);
		FD_SET(identd::sock, &readset);

		errno = 0;

		tv.tv_sec	= 3;
		tv.tv_usec	= 111;

		if (select(get_maxfdp1(), &readset, nullptr, nullptr, &tv) ==
		    SOCKET_ERROR) {
			if (errno == EINTR)
				continue;
			break;
		} else if (!FD_ISSET(identd::sock, &readset)) {
			/*
			 * No connection to accept()
			 */

			continue;
		} else if ((clisock = accept(identd::sock, reinterpret_cast
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
identd::send_err_resp(const char *server_port, const char *client_port,
    ident_client *cli)
{
	char	*str;
	int	 len;

	str = strdup_printf("%s , %s : ERROR : NO-USER\r\n", server_port,
	    client_port);
	len = size_to_int(strlen(str));
	errno = 0;

	if (send(cli->get_sock(), str, len, 0) <= 0)
		err_log(errno, "%s: cannot send", __func__);
	free(str);
}

void
identd::send_response(const char *server_port, const char *client_port,
    ident_client *cli)
{
#if defined(UNIX)
#define OPSYS "UNIX"
#elif defined(WIN32)
#define OPSYS "WIN32"
#endif
	char	*str;
	int	 len;

	if (config_bool("identd_fakenames", false)) {
		str = strdup_printf("%s , %s : USERID : %s : %s\r\n",
		    server_port, client_port, OPSYS, get_username_fake());
	} else {
		str = strdup_printf("%s , %s : USERID : %s : %s\r\n",
		    server_port, client_port, OPSYS, get_username());
	}

	len = size_to_int(strlen(str));
	errno = 0;

	if (send(cli->get_sock(), str, len, 0) <= 0)
		err_log(errno, "%s: cannot send", __func__);
	free(str);
}

int
identd::start_pre_check(void)
{
	if (identd::listening || identd::sock != INVALID_SOCKET)
		return -1;
	return 0;
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
