/* Platform independent networking routines
   Copyright (C) 2014-2024 Markus Uhlin. All rights reserved.

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

#include <netdb.h>
#include <unistd.h> /* close() */
#endif

#include <exception>
#include <stdexcept>
#include <string.h>

#include "assertAPI.h"
#include "config.h"
#include "dataClassify.h"
#include "errHand.h"
#include "i18n.h"
#include "icb.h"
#include "identd.hpp"
#include "irc.h"
#include "libUtils.h"
#include "main.h"
#include "netsplit.h"
#include "network.h"
#include "printtext.h"
#include "sig.h"
#include "socks.hpp"
#include "strHand.h"

#include "commands/connect.h"
#include "commands/dcc.h"
#include "commands/sasl-scram-sha.h"

#include "events/cap.h"
#include "events/welcome.h"

/****************************************************************
*                                                               *
*  ------------------ Structure definitions ------------------  *
*                                                               *
****************************************************************/

class reconnect_context {
public:
	long int	backoff_delay;
	long int	delay;
	long int	delay_max;
	long int	retries;
	long int	retry;

	reconnect_context()
	{
		this->backoff_delay	= 0;
		this->delay		= 0;
		this->delay_max		= 0;
		this->retries		= 0;
		this->retry		= 0;

		this->initialized = false;
	}

	void
	init(void)
	{
		this->backoff_delay	= get_reconnect_backoff_delay();
		this->delay		= get_reconnect_delay();
		this->delay_max		= get_reconnect_delay_max();
		this->retries		= get_reconnect_retries();
		this->retry		= 0;
	}

	bool
	is_initial_attempt(void) const
	{
		return (this->retry <= 1);
	}

	bool
	is_initialized(void)
	{
		return atomic_load_bool(&this->initialized);
	}

	void
	set_init(bool yesno)
	{
		(void) atomic_swap_bool(&this->initialized, yesno);
	}

private:
	volatile bool initialized;
};

/****************************************************************
*                                                               *
*  -------------- Objects with external linkage --------------  *
*                                                               *
****************************************************************/

/*
 * net_send_fake() store the sent data into this buffer
 */
char g_sent[600] = { '\0' };

NET_SEND_FN	net_send = net_send_plain;
NET_RECV_FN	net_recv = net_recv_plain;

volatile bool	g_connection_in_progress = false;
volatile bool	g_connection_lost = false;
volatile bool	g_irc_listening = false;
volatile bool	g_on_air = false;

char	g_last_server[1024] = { 0 };
char	g_last_port[32] = { 0 };
char	g_last_pass[256] = { 0 };

int g_socket_address_family = AF_UNSPEC;

/****************************************************************
*                                                               *
*  -------------- Objects with internal linkage --------------  *
*                                                               *
****************************************************************/

static reconnect_context reconn_ctx;
static const int RECVBUF_SIZE = 2048;

/****************************************************************
*                                                               *
*  ---------------------    Functions    ---------------------  *
*                                                               *
****************************************************************/

static void	 send_icb_login_packet(const struct network_connect_context *)
		     NONNULL;
static void	 send_reg_cmds(const struct network_connect_context *)
		     NONNULL;

static void
check_conn_fail()
{
	if (!atomic_load_bool(&g_on_air) ||
	    (ssl_is_enabled() && net_ssl_begin() == -1))
		throw std::runtime_error(_("Failed to establish a connection"));
}

static void
check_hostname(const char *host, PPRINTTEXT_CONTEXT ctx)
{
	if (ssl_is_enabled() && config_bool("hostname_checking", true)) {
		if (net_ssl_check_hostname(host, 0) != OK)
			throw std::runtime_error(_("Hostname checking failed!"));
		else
			printtext(ctx, "%s", _("Hostname checking OK!"));
	}
}

static int
conn_check()
{
	if (g_icb_mode) {
		const int msglen = 1;

		if (net_send("%cn", msglen) == -1)
			return -1;
	} else {
		if (g_server_hostname != NULL) {
			if (net_send("PING %s", g_server_hostname) == -1)
				return -1;
		}
	}

	return 0;
}

static inline void
destroy_null_bytes(char *recvbuf, const int bytes_received)
{
	int	i, j;

	if (bytes_received < 1)
		return;
	for (i = bytes_received - 1; i >= 0; i--) {
		if (recvbuf[i] != '\0')
			break;
	}
	for (j = 0; j < i; j++) {
		if (recvbuf[j] == '\0')
			recvbuf[j] = 'X';
	}
}

static void
connect_hook(void)
{
	g_sasl_scram_sha_got_first_msg = false;
}

static void
establish_conn(struct addrinfo *res, PPRINTTEXT_CONTEXT ctx)
{
	for (struct addrinfo *rp = res; rp; rp = rp->ai_next) {
		if ((g_socket = socket(rp->ai_family, rp->ai_socktype,
		    rp->ai_protocol)) == INVALID_SOCKET)
			continue;

		net_set_recv_timeout(TEMP_RECV_TIMEOUT);
		net_set_send_timeout(TEMP_SEND_TIMEOUT);

		if (connect(g_socket, rp->ai_addr, rp->ai_addrlen) == 0) {
			printtext(ctx, "%s", _("Connected!"));

			atomic_swap_bool(&g_on_air, true);

			net_set_recv_timeout(DEFAULT_RECV_TIMEOUT);
			net_set_send_timeout(DEFAULT_SEND_TIMEOUT);
			break;
		} else {
			CLOSE_GLOBAL_SOCKET();
			g_socket = INVALID_SOCKET;
		}
	} /* for */
}

static int
get_and_handle_remaining_bytes(const int bytes_remaining,
    struct network_recv_context *ctx, const char *recvbuf, const int length)
{
	char	*tmp = NULL;
	char	*concat = NULL;

	try {
		int	bytes_received;
		size_t	concatSize;

		if (bytes_remaining <= 0 || ctx == NULL || recvbuf == NULL ||
		    length < 0 || length > UCHAR_MAX)
			throw std::runtime_error("invalid arguments");

		tmp = static_cast<char *>(xmalloc(bytes_remaining + 1));
		tmp[bytes_remaining] = '\0';

		if ((bytes_received = net_recv(ctx, tmp, bytes_remaining)) !=
		    bytes_remaining) {
			throw std::runtime_error("read bytes mismatch "
			    "remaining");
		}
		if (memchr(tmp, 0, bytes_received) != NULL)
			destroy_null_bytes(tmp, bytes_received);

		concatSize = strlen(recvbuf) + strlen(tmp) + 1;
		concat = static_cast<char *>(xmalloc(concatSize));

		if (sw_strcpy(concat, recvbuf, concatSize) == 0 &&
		    sw_strcat(concat, tmp, concatSize) == 0)
			icb_irc_proxy(length, concat[0], &concat[1]);
		else
			throw std::runtime_error("insufficient buffer size");

		free(tmp);
		free(concat);
		return OK;
	} catch (const std::runtime_error &e) {
		err_log(0, "%s: %s", __func__, e.what());
		free(tmp);
		free(concat);
	}

	return ERR;
}

static void
get_ip_addresses(struct addrinfo *&res, const char *server, const char *port,
    PPRINTTEXT_CONTEXT ctx)
{
	if ((res = net_addr_resolve(server, port)) == NULL) {
		throw std::runtime_error(_("Unable to get a list of IP "
		    "addresses"));
	} else {
		printtext(ctx, "%s", _("Get a list of IP addresses completed"));
	}
}

static void
handle_conn_err(PPRINTTEXT_CONTEXT ptext_ctx, const char *what,
    long int *sleep_time_seconds, conn_res_t &conn_res)
{
	ptext_ctx->spec_type = TYPE_SPEC1_FAILURE;
	printtext(ptext_ctx, "%s", what);

	atomic_swap_bool(&g_on_air, false);

	net_ssl_end();

	if (g_socket != INVALID_SOCKET) {
		CLOSE_GLOBAL_SOCKET();
		g_socket = INVALID_SOCKET;
	}

	if (reconn_ctx.retry++ < reconn_ctx.retries) {
		if (reconn_ctx.is_initial_attempt())
			*sleep_time_seconds = reconn_ctx.delay;
		else
			*sleep_time_seconds += reconn_ctx.backoff_delay;

		/* --------------------------------------------- */

		if (*sleep_time_seconds > reconn_ctx.delay_max)
			*sleep_time_seconds = reconn_ctx.delay_max;

		(void) atomic_swap_bool(&g_connection_in_progress, false);
		conn_res = SHOULD_RETRY_TO_CONNECT;
		return;
	}

	net_connect_clean_up();
	conn_res = CONNECTION_FAILED;
}

static void
save_last_server(const char *server, const char *port, const char *password)
{
	int ret;

	if ((ret = snprintf(g_last_server, sizeof g_last_server, "%s", server)) < 0 ||
	    static_cast<size_t>(ret) >= sizeof g_last_server)
		err_log(EOVERFLOW, "%s: cannot save server", __func__);
	if ((ret = snprintf(g_last_port, sizeof g_last_port, "%s", port)) < 0 ||
	    static_cast<size_t>(ret) >= sizeof g_last_port)
		err_log(EOVERFLOW, "%s: cannot save port", __func__);
	if ((ret = snprintf(g_last_pass, sizeof g_last_pass, "%s", password)) < 0 ||
	    static_cast<size_t>(ret) >= sizeof g_last_pass)
		err_log(EOVERFLOW, "%s: cannot save password", __func__);
}

static void
select_send_and_recv_funcs()
{
	if (ssl_is_enabled()) {
		net_send = net_ssl_send;
		net_recv = net_ssl_recv;
	} else {
		net_send = net_send_plain;
		net_recv = net_recv_plain;
	}
}

/*
 * ICB Login Packet
 * ----------------
 * (Fields: Minimum: 5, Maximum: 7)
 *
 * Field 0: Login id of user. Required.
 * Field 1: Nickname to use upon login into ICB. Required.
 * Field 2: Default group to log into in ICB, or do group who of. A null string
 *          for who listing will show all groups. Required.
 * Field 3: Login command. Required. Currently one of the following:
 * - "login" log into ICB
 * - "w" just show who is currently logged into ICB
 * Field 4: Password to authenticate the user to ICB. Required, but often blank.
 * Field 5: If when logging in, default group (field 2) does not exist, create
 *          it with this status. Optional.
 * Field 6: Protocol level. Optional. Deprecated.
 *
 * Thus the ICB Login Packet has the following layout:
 * aLoginid^ANickname^ADefaultGroup^ACommand^APass^AGroupStatus^AProtocolLevel
 */
static void
send_icb_login_packet(const struct network_connect_context *ctx)
{
	char		msg[ICB_MESSAGE_MAX] = { '\0' };
	int		msglen, ret;
	std::string	str("a");

	str.append(ctx->username).append(ICB_FIELD_SEP);
	str.append(ctx->nickname).append(ICB_FIELD_SEP);
	str.append("1").append(ICB_FIELD_SEP);
	str.append("login").append(ICB_FIELD_SEP);
	str.append(ctx->password ? ctx->password : "").append(ICB_FIELD_SEP);

	ret = snprintf(msg, ARRAY_SIZE(msg), "%s", str.c_str());

	if (ret < 0 || static_cast<size_t>(ret) >= ARRAY_SIZE(msg)) {
		err_log(ENOBUFS, "%s", __func__);
		return;
	}

	msglen = static_cast<int>(strlen(msg));
	irc_set_my_nickname(ctx->nickname);

	if (net_send("%c%s", msglen, msg) < 0)
		err_log(ENOTCONN, "%s", __func__);
}

static void
send_reg_cmds(const struct network_connect_context *ctx)
{
	PRINTTEXT_CONTEXT ptext_ctx;

	if (ctx->password)
		(void) net_send("PASS %s", ctx->password);
	(void) net_send("NICK %s", ctx->nickname);
	(void) net_send("USER %s 8 * :%s", ctx->username, ctx->rl_name);

	printtext_context_init(&ptext_ctx, g_status_window, TYPE_SPEC1_SUCCESS,
	    true);
	if (config_bool("account_notify", true)) {
		if (net_send("CAP REQ :account-notify") > 0)
			printtext(&ptext_ctx, "Requesting account notify");
	}
	if (config_bool("away_notify", false)) {
		if (net_send("CAP REQ :away-notify") > 0)
			printtext(&ptext_ctx, "Requesting away notify");
	}
	if (config_bool("batch", true)) {
		if (net_send("CAP REQ :batch") > 0)
			printtext(&ptext_ctx, "Requesting batch");
	}
	if (config_bool("chghost", true)) {
		if (net_send("CAP REQ :chghost") > 0)
			printtext(&ptext_ctx, "Requesting change host");
	}
	if (config_bool("extended_join", true)) {
		if (net_send("CAP REQ :extended-join") > 0)
			printtext(&ptext_ctx, "Requesting extended join");
	}
	if (config_bool("invite_notify", false)) {
		if (net_send("CAP REQ :invite-notify") > 0)
			printtext(&ptext_ctx, "Requesting invite notify");
	}
	if (config_bool("multi_prefix", true)) {
		if (net_send("CAP REQ :multi-prefix") > 0)
			printtext(&ptext_ctx, "Requesting multi prefix");
	}
	if (config_bool("server_time", true)) {
		if (net_send("CAP REQ :server-time") > 0)
			printtext(&ptext_ctx, "Requesting server time");
	}
	if (sasl_is_enabled()) {
		if (strings_match(get_sasl_mechanism(), "PLAIN") &&
		    !ssl_is_enabled()) {
			ptext_ctx.spec_type = TYPE_SPEC1_WARN;
			printtext(&ptext_ctx, "SASL mechanism matches PLAIN "
			    "and TLS/SSL is not enabled. Not requesting SASL "
			    "authentication.");
		} else {
			if (net_send("CAP REQ :sasl") > 0) {
				printtext(&ptext_ctx, "Requesting SASL "
				    "authentication");
			}
		}
	}
}

static bool
should_check_connection()
{
	static int times_called = 0;

	if (times_called > 15) {
		times_called = 0;
		return true;
	}

	times_called ++;
	return false;
}

bool
sasl_is_enabled(void)
{
	if (!g_sasl_authentication)
		return false;
	return config_bool("sasl", false);
}

conn_res_t
net_connect(const struct network_connect_context *ctx,
    long int *sleep_time_seconds)
{
	PRINTTEXT_CONTEXT	 ptext_ctx;
	conn_res_t		 conn_res = CONNECTION_FAILED;
	struct addrinfo		*res = NULL;
	struct integer_context	 intctx("identd_port", 113, 65535, 113);

	if (ctx == NULL || sleep_time_seconds == NULL)
		err_exit(EINVAL, "%s", __func__);
	else if (atomic_load_bool(&g_connection_in_progress))
		return CONNECTION_FAILED;
	else
		(void) atomic_swap_bool(&g_connection_in_progress, true);

	if (!reconn_ctx.is_initialized()) {
		reconn_ctx.init();
		reconn_ctx.set_init(true);
	}

	printtext_context_init(&ptext_ctx, g_status_window, TYPE_SPEC1, true);
	printtext(&ptext_ctx, _("Connecting to %s (%s)"), ctx->server,
	    ctx->port);
	connect_hook();

	try {
		ptext_ctx.spec_type = TYPE_SPEC1_SUCCESS;

		save_last_server(ctx->server, ctx->port, (ctx->password ?
		    ctx->password : ""));
		if (config_bool("identd", false))
			identd::start(config_integer(&intctx));
		if (!socks::yesno()) {
			get_ip_addresses(res, ctx->server, ctx->port,
			    &ptext_ctx);
		} else {
			get_ip_addresses(res,
			    Config("socks_host"), Config("socks_port"),
			    &ptext_ctx);
		}

		establish_conn(res, &ptext_ctx);

		if (res)
			freeaddrinfo(res);
		if (socks::yesno()) {
			std::string err("");

			if (socks::connect(ctx->server, ctx->port, err) == -1)
				throw std::runtime_error(err);
		}

		select_send_and_recv_funcs();
		check_conn_fail();
		if (!socks::yesno())
			check_hostname(ctx->server, &ptext_ctx);

		event_welcome_cond_init();
		net_spawn_listen_thread();

		if (g_icb_mode)
			send_icb_login_packet(ctx);
		else
			send_reg_cmds(ctx);

		if (!event_welcome_is_signaled()) {
			event_welcome_cond_destroy();
			throw std::runtime_error("Event welcome not signaled!");
		}

		event_welcome_cond_destroy();
	} catch (const std::bad_alloc &e) {
		err_exit(ENOMEM, "%s: %s", __func__, e.what());
	} catch (const std::runtime_error &e) {
		handle_conn_err(&ptext_ctx, e.what(), sleep_time_seconds,
		    conn_res);
		return conn_res;
	} catch (...) {
		handle_conn_err(&ptext_ctx, "unknown exception",
		    sleep_time_seconds, conn_res);
		return conn_res;
	}

	if (!g_icb_mode)
		window_foreach_rejoin_all_channels();

	net_connect_clean_up();
	return CONNECTION_ESTABLISHED;
}

int
net_send_fake(const char *fmt, ...)
{
	int bytes_sent;
	va_list ap;

	va_start(ap, fmt);
	bytes_sent = vsnprintf(g_sent, ARRAY_SIZE(g_sent), fmt, ap);
	va_end(ap);

	return bytes_sent;
}

struct addrinfo *
net_addr_resolve(const char *host, const char *port)
{
	struct addrinfo *res = NULL;
	struct addrinfo hints;

	if (host == NULL || port == NULL)
		return NULL;

	BZERO(&hints, sizeof hints);
	hints.ai_flags     = AI_CANONNAME;
	hints.ai_family    = g_socket_address_family;
	hints.ai_socktype  = SOCK_STREAM;
	hints.ai_protocol  = 0;
	hints.ai_addrlen   = 0;
	hints.ai_addr      = NULL;
	hints.ai_canonname = NULL;
	hints.ai_next      = NULL;

	return (getaddrinfo(host, port, &hints, &res) != 0 ? NULL : res);
}

struct server *
server_new(const char *host, const char *port, const char *pass)
{
	struct server *server =
	    static_cast<struct server *>(xmalloc(sizeof *server));

	server->host = sw_strdup(host);
	server->port = sw_strdup(port);
	server->pass = (pass ? sw_strdup(pass) : NULL);

	return server;
}

void
net_connect_clean_up(void)
{
	reconn_ctx.init();
	atomic_swap_bool(&g_connection_in_progress, false);
}

void
destroy_null_bytes_exported(char *buf, const int len)
{
	destroy_null_bytes(buf, len);
}

static int
icb(int &bytes_received, struct network_recv_context *ctx, char *recvbuf)
{
	char	array[10] = { '\0' };
	int	length, ret;

	ret = snprintf(array, ARRAY_SIZE(array), "%d",
	    static_cast<unsigned char>(recvbuf[0]));

	if (ret < 0 || static_cast<size_t>(ret) >= ARRAY_SIZE(array)) {
		err_log(ENOBUFS, "%s", __func__);
		(void) atomic_swap_bool(&g_connection_lost, true);
		return ERR;
	}

	length = atoi(array);
	sw_assert(length >= 0 && length <= UCHAR_MAX);

	if ((bytes_received = net_recv(ctx, recvbuf, length)) == -1) {
		(void) atomic_swap_bool(&g_connection_lost, true);
	} else if (bytes_received != length && length != 0) {
		const int	maxval = MAX(length, bytes_received);
		const int	minval = MIN(length, bytes_received);
		const int	bytes_rem = int_diff(maxval, minval);

		if (get_and_handle_remaining_bytes(bytes_rem, ctx, recvbuf,
		    length) == ERR) {
			err_log(EPROTO, "%s", __func__);
			(void) atomic_swap_bool(&g_connection_lost, true);
			return ERR;
		}
	} else if (bytes_received > 0) {
		if (memchr(recvbuf, 0, bytes_received) != NULL)
			destroy_null_bytes(recvbuf, bytes_received);

		try {
			icb_irc_proxy(length, recvbuf[0], &recvbuf[1]);
		} catch (const std::exception &e) {
			err_log(0, "%s: catched: %s", __func__, e.what());
		}
	}

	return OK;
}

static void
irc(int &bytes_received, struct network_recv_context *ctx, char *recvbuf,
    char **message_concat, enum message_concat_state *state)
{
	if ((bytes_received = net_recv(ctx, recvbuf, RECVBUF_SIZE)) == -1) {
		(void) atomic_swap_bool(&g_connection_lost, true);
	} else if (bytes_received > 0) {
		if (memchr(recvbuf, 0, bytes_received) != NULL)
			destroy_null_bytes(recvbuf, bytes_received);

		try {
			irc_handle_interpret_events(recvbuf, message_concat,
			    state);
		} catch (const std::exception &e) {
			err_log(0, "%s: catched: %s", __func__, e.what());
		}
	}
}

void
net_irc_listen(bool *connection_lost)
{
	PRINTTEXT_CONTEXT		 ptext_ctx;
	char				*message_concat = NULL;
	char				*recvbuf = NULL;
	enum message_concat_state	 state = CONCAT_BUFFER_IS_EMPTY;
	int				 bytes_received = -1;
	struct network_recv_context	 ctx(g_socket, 0, 5, 0);

	if (atomic_load_bool(&g_irc_listening))
		return;
	else
		(void) atomic_swap_bool(&g_irc_listening, true);

	block_signals();
	*connection_lost = false;
	atomic_swap_bool(&g_connection_lost, false);
	recvbuf = static_cast<char *>(xmalloc(RECVBUF_SIZE + 1));
	recvbuf[RECVBUF_SIZE] = '\0';
	irc_init();
	netsplit_init();

	do {
		OPENSSL_cleanse(recvbuf, RECVBUF_SIZE);

		if (g_icb_mode) {
			/*
			 * ICB
			 */

			if ((bytes_received = net_recv(&ctx, recvbuf, 1)) ==
			    -1) {
				(void) atomic_swap_bool(&g_connection_lost,
				    true);
				break;
			} else if (bytes_received != 1) {
				if (atomic_load_bool(&g_icb_processing_names))
					icb_process_event_eof_names();
				goto _conn_check;
			} else if (icb(bytes_received, &ctx, recvbuf) != OK)
				break;
		} else {
			/*
			 * IRC
			 */

			irc(bytes_received, &ctx, recvbuf, &message_concat,
			    &state);
		}

	  _conn_check:
		if (bytes_received == 0 && should_check_connection()) {
			if (conn_check() == -1) {
				(void) atomic_swap_bool(&g_connection_lost,
				    true);
			}
		}

		try {
			// Handle Netsplits
			netsplit_run_bkgd_task();
		} catch (const std::exception &e) {
			err_log(0, "%s: netsplit_run_bkgd_task: %s", __func__,
			    e.what());
		}
	} while (atomic_load_bool(&g_on_air) &&
		 !atomic_load_bool(&g_connection_lost));

	printtext_context_init(&ptext_ctx, g_active_window, TYPE_SPEC1_WARN,
	    true);
	*connection_lost = (atomic_load_bool(&g_on_air) &&
			    atomic_load_bool(&g_connection_lost));
	if (*connection_lost)
		printtext(&ptext_ctx, "%s", _("Connection to IRC server lost"));
	atomic_swap_bool(&g_on_air, false);
	net_ssl_end();
	if (g_socket != INVALID_SOCKET) {
		CLOSE_GLOBAL_SOCKET();
		g_socket = INVALID_SOCKET;
	}
	irc_deinit();
	netsplit_deinit();
	free_and_null(&recvbuf);
	free_and_null(&message_concat);
	printtext(&ptext_ctx, "%s", _("Disconnected"));
	(void) atomic_swap_bool(&g_irc_listening, false);
}

void
net_kill_connection(void)
{
	net_request_disconnect();

	if (g_socket == INVALID_SOCKET)
		return;

	errno = 0;

#if defined(UNIX)
	if (shutdown(g_socket, SHUT_RDWR) == -1)
		err_log(errno, "%s: shutdown", __func__);
#elif defined(WIN32)
	if (shutdown(g_socket, SD_BOTH) != 0)
		err_log(errno, "%s: shutdown", __func__);
#endif
}

void
net_request_disconnect(void)
{
	(void) atomic_swap_bool(&g_disconnect_wanted, true);
	(void) atomic_swap_bool(&g_connection_lost, false);
	(void) atomic_swap_bool(&g_on_air, false);
}

void
server_destroy(struct server *server)
{
	if (server == NULL)
		return;
	free(server->host);
	free(server->port);
	free(server->pass);
	free(server);
}

void
net_set_sock_addr_family_ipv4(void)
{
	g_socket_address_family = AF_INET;
}

void
net_set_sock_addr_family_ipv6(void)
{
	g_socket_address_family = AF_INET6;
}
