/* Copyright (c) 2016-2021 Markus Uhlin <markus.uhlin@bredband.net>
   All rights reserved.

   Permission to use, copy, modify, and distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
   AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
   DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
   PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
   PERFORMANCE OF THIS SOFTWARE. */

#include "common.h"

#ifdef UNIX
#include <sys/select.h>
#endif

#include <openssl/err.h> /* ERR_clear_error() */
#include <openssl/opensslv.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include <limits.h>

#include "assertAPI.h"
#include "config.h"
#include "dataClassify.h"
#include "errHand.h"
#include "libUtils.h"
#include "main.h"
#include "network.h"
#include "printtext.h"
#include "strHand.h"
#include "strdup_printf.h"

#define CAFILE "trusted_roots.pem"
#define CADIR NULL

static SSL_CTX	*ssl_ctx = NULL;
static SSL	*ssl     = NULL;

static volatile bool ssl_object_is_null = true;

#if defined(UNIX)
static pthread_once_t ssl_end_init_done = PTHREAD_ONCE_INIT;
static pthread_once_t ssl_send_init_done = PTHREAD_ONCE_INIT;
static pthread_mutex_t ssl_end_mutex;
static pthread_mutex_t ssl_send_mutex;
#elif defined(WIN32)
static init_once_t ssl_end_init_done = ONCE_INITIALIZER;
static init_once_t ssl_send_init_done = ONCE_INITIALIZER;
static HANDLE ssl_end_mutex;
static HANDLE ssl_send_mutex;
#endif

static const char suite_secure[]   = "TLSv1.3:TLSv1.2+AEAD+ECDHE:TLSv1.2+AEAD+DHE";
static const char suite_compat[]   = "HIGH:!aNULL";
static const char suite_legacy[]   = "ALL:!ADH:!EXP:!LOW:!MD5:@STRENGTH";
static const char suite_insecure[] = "ALL:!aNULL:!eNULL";

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
static void
create_ssl_context_obj(void)
{
    if ((ssl_ctx = SSL_CTX_new(TLS_client_method())) == NULL) {
	err_exit(ENOMEM, "create_ssl_context_obj: "
	    "Unable to create a new SSL_CTX object");
    }

    (void) SSL_CTX_set_mode(ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
    (void) SSL_CTX_set_mode(ssl_ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    (void) SSL_CTX_set_mode(ssl_ctx, SSL_MODE_AUTO_RETRY);
    (void) SSL_CTX_set_mode(ssl_ctx, SSL_MODE_RELEASE_BUFFERS);

    (void) SSL_CTX_set_options(ssl_ctx, (SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3));

    //(void) SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_TLSv1);
    //(void) SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_TLSv1_1);

    if (!SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_2_VERSION)) {
	err_log(0, "create_ssl_context_obj: error setting "
	    "minimum supported protocol version");
    }
}
#else
/* -------------------------------- */
/* OpenSSL version less than v1.1.0 */
/* -------------------------------- */

static void
create_ssl_context_obj_insecure(void)
{
    if ((ssl_ctx = SSL_CTX_new(SSLv23_client_method())) == NULL) {
	err_exit(ENOMEM, "create_ssl_context_obj_insecure: "
	    "Unable to create a new SSL_CTX object");
    } else {
	SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv2);
	SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv3);
    }
}
#endif

static void
set_ciphers(const char *list)
{
    PRINTTEXT_CONTEXT ptext_ctx;
    char strerrbuf[MAXERROR] = { '\0' };

    printtext_context_init(&ptext_ctx, g_status_window, TYPE_SPEC1_WARN, true);

    if (ssl_ctx && list && !SSL_CTX_set_cipher_list(ssl_ctx, list)) {
	printtext(&ptext_ctx, "warning: set_ciphers: bogus cipher list: %s",
		  xstrerror(EINVAL, strerrbuf, MAXERROR));
    }
}

static void
ssl_end_mutex_init(void)
{
	mutex_new(&ssl_end_mutex);
}

static void
ssl_send_mutex_init(void)
{
	mutex_new(&ssl_send_mutex);
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

int
net_ssl_begin(void)
{
	PRINTTEXT_CONTEXT ptext_ctx;
	const char *err_reason = "";
	static const int VALUE_HANDSHAKE_OK = 1;

	if (ssl != NULL) {
		err_reason = "SSL object nonnull";
		goto err;
	} else if ((ssl = SSL_new(ssl_ctx)) == NULL) {
		err_exit(ENOMEM, "net_ssl_begin: Unable to create a new "
		    "SSL object");
	} else {
		(void) atomic_swap_bool(&ssl_object_is_null, false);
	}

	if (!SSL_set_fd(ssl, g_socket)) {
		err_reason = "Unable to associate the global socket fd with "
		    "the SSL object";
		goto err;
	}

	SSL_set_connect_state(ssl);

	if (SSL_connect(ssl) != VALUE_HANDSHAKE_OK) {
		err_reason = "TLS/SSL handshake failed!";
		goto err;
	}

	return 0;

  err:
	printtext_context_init(&ptext_ctx, g_status_window, TYPE_SPEC1_FAILURE,
	    true);
	printtext(&ptext_ctx, "net_ssl_begin: %s", err_reason);
	return -1;
}

void
net_ssl_end(void)
{
#if defined(UNIX)
	if ((errno = pthread_once(&ssl_end_init_done, ssl_end_mutex_init)) != 0)
		err_sys("net_ssl_end: pthread_once");
#elif defined(WIN32)
	if ((errno = init_once(&ssl_end_init_done, ssl_end_mutex_init)) != 0)
		err_sys("net_ssl_end: init_once");
#endif

	mutex_lock(&ssl_end_mutex);
	if (ssl != NULL && !atomic_load_bool(&ssl_object_is_null)) {
		switch (SSL_shutdown(ssl)) {
		case 0:
			debug("net_ssl_end: SSL_shutdown: not yet finished");
			break;
		case 1:
			/* success! */
			break;
		default:
			err_log(0, "net_ssl_end: SSL_shutdown: error");
			break;
		}
		SSL_free(ssl);
		ssl = NULL;
		(void) atomic_swap_bool(&ssl_object_is_null, true);
		(void) napms(10);
	}
	mutex_unlock(&ssl_end_mutex);
}

int
net_ssl_check_hostname(const char *host, unsigned int flags)
{
	X509	*cert = NULL;
	int	 ret = ERR;

	if (ssl == NULL || (cert = SSL_get_peer_certificate(ssl)) == NULL ||
	    host == NULL) {
		if (cert)
			X509_free(cert);
		return ERR;
	}
	ret = (X509_check_host(cert, host, 0, flags, NULL) > 0 ? OK : ERR);
	X509_free(cert);
	return ret;
}

int
net_ssl_send(const char *fmt, ...)
{
	char *buf = NULL;
	char *bufptr = NULL;
	int buflen = 0;
	int n_sent = 0;
	va_list ap;

#if defined(UNIX)
	if ((errno = pthread_once(&ssl_send_init_done, ssl_send_mutex_init)) !=
	    0)
		err_sys("net_ssl_send: pthread_once");
#elif defined(WIN32)
	if ((errno = init_once(&ssl_send_init_done, ssl_send_mutex_init)) != 0)
		err_sys("net_ssl_send: init_once");
#endif

	mutex_lock(&ssl_send_mutex);

	if (fmt == NULL || ssl == NULL) {
		mutex_unlock(&ssl_send_mutex);
		return -1;
	}

	va_start(ap, fmt);
	buf = strdup_vprintf(fmt, ap);
	va_end(ap);

	/* message terminate */
	if (!g_icb_mode)
		realloc_strcat(&buf, "\r\n");

	if (strlen(buf) > INT_MAX) {
		free(buf);
		mutex_unlock(&ssl_send_mutex);
		return -1;
	}

	bufptr = buf;
	buflen = (int) strlen(buf);

	while (buflen > 0) {
		ERR_clear_error();
		const int ret = SSL_write(ssl, bufptr, buflen);

		if (ret > 0) {
			if (BIO_flush(SSL_get_wbio(ssl)) != 1)
				debug("net_ssl_send: error flushing write bio");
			n_sent += ret;
			bufptr += ret;
			buflen -= ret;
		} else {
			switch (SSL_get_error(ssl, ret)) {
			case SSL_ERROR_NONE:
				sw_assert_not_reached();
				break;
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
				debug("net_ssl_send: want read / want write");
				continue;
			}

			free(buf);
			mutex_unlock(&ssl_send_mutex);
			return -1;
		}
	}

	free(buf);
	mutex_unlock(&ssl_send_mutex);
	return n_sent;
}

int
net_ssl_recv(struct network_recv_context *ctx, char *recvbuf, int recvbuf_size)
{
#ifdef UNIX
#define SOCKET_ERROR -1
#endif
	if (ctx == NULL || recvbuf == NULL || ssl == NULL)
		return -1;
	if (!SSL_pending(ssl)) {
		const int	maxfdp1 = ctx->sock + 1;
		fd_set		readset;
		struct timeval	tv;

		tv.tv_sec = ctx->sec;
		tv.tv_usec = ctx->microsec;

		FD_ZERO(&readset);
		FD_SET(ctx->sock, &readset);

		errno = 0;

		if (select(maxfdp1, &readset, NULL, NULL, &tv) == SOCKET_ERROR)
			return (errno == EINTR ? 0 : -1);
		else if (!FD_ISSET(ctx->sock, &readset))
			return 0;
	}

	char	*bufptr = recvbuf;
	int	 buflen = recvbuf_size;
	int	 bytes_received = 0;

	do {
		ERR_clear_error();
		const int ret = SSL_read(ssl, bufptr, buflen);

		if (ret > 0) {
			bytes_received += ret;
			bufptr += ret;
			buflen -= ret;
		} else {
			switch (SSL_get_error(ssl, ret)) {
			case SSL_ERROR_NONE:
				sw_assert_not_reached();
				break;
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
				debug("net_ssl_recv: want read / want write");
				break;
			default:
				return -1;
			}
		}
	} while (buflen > 0 && SSL_pending(ssl));

	return bytes_received;
}

void
net_ssl_init(void)
{
	PRINTTEXT_CONTEXT ptext_ctx;
	char strerrbuf[MAXERROR] = { '\0' };

	printtext_context_init(&ptext_ctx, g_status_window, TYPE_SPEC1_WARN,
	    true);

	SSL_load_error_strings();
	(void) SSL_library_init();

	if (RAND_load_file("/dev/urandom", 1024) <= 0) {
		printtext(&ptext_ctx, "net_ssl_init: "
		    "Error seeding the PRNG! (%s)",
		    xstrerror(ENOSYS, strerrbuf, MAXERROR));
	}

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	create_ssl_context_obj();
#else
#pragma message("Consider updating your TLS/SSL library")
	create_ssl_context_obj_insecure();
#endif

	if (config_bool("ssl_verify_peer", true)) {
#ifdef WIN32
		if (!SSL_CTX_load_verify_locations(ssl_ctx, CAFILE, CADIR)) {
			printtext(&ptext_ctx, "net_ssl_init: Error loading "
			    "CA file and/or directory");
		}
#endif
		if (!SSL_CTX_set_default_verify_paths(ssl_ctx)) {
			printtext(&ptext_ctx, "net_ssl_init: Error loading "
			    "default CA file and/or directory");
		}
		SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, verify_callback);
		SSL_CTX_set_verify_depth(ssl_ctx, 4);
	} else {
		printtext(&ptext_ctx, "net_ssl_init: "
		    "Certificate verification is disabled: Option set to NO?");
	}

	const char *cs = Config("cipher_suite");

	if (strings_match(cs, "secure") || strings_match(cs, "SECURE"))
		set_ciphers(suite_secure);
	else if (strings_match(cs, "compat") || strings_match(cs, "COMPAT"))
		set_ciphers(suite_compat);
	else if (strings_match(cs, "legacy") || strings_match(cs, "LEGACY"))
		set_ciphers(suite_legacy);
	else if (strings_match(cs, "all") || strings_match(cs, "ALL") ||
		 strings_match(cs, "insecure") || strings_match(cs, "INSECURE"))
		set_ciphers(suite_insecure);
	else
		set_ciphers(suite_compat);
}

void
net_ssl_deinit(void)
{
	net_ssl_end();

	if (ssl_ctx) {
		SSL_CTX_free(ssl_ctx);
		ssl_ctx = NULL;
	}
}
