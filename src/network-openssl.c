/* Copyright (c) 2016-2018 Markus Uhlin <markus.uhlin@bredband.net>
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
#include "errHand.h"
#include "libUtils.h"
#include "network.h"
#include "printtext.h"
#include "strHand.h"
#include "strdup_printf.h"

static SSL_CTX	*ssl_ctx = NULL;
static SSL	*ssl     = NULL;

static const char *suite_secure   = "TLSv1.2+AEAD+ECDHE:TLSv1.2+AEAD+DHE";
static const char *suite_compat   = "HIGH:!aNULL";
static const char *suite_legacy   = "ALL:!ADH:!EXP:!LOW:!MD5:@STRENGTH";
static const char *suite_insecure = "ALL:!aNULL:!eNULL";

static int
verify_callback(int ok, X509_STORE_CTX *ctx)
{
    PRINTTEXT_CONTEXT ptext_ctx;
    X509	*cert         = X509_STORE_CTX_get_current_cert(ctx);
    char	 issuer[256]  = "";
    char	 subject[256] = "";
    const int	 depth        = X509_STORE_CTX_get_error_depth(ctx);
    const int	 err          = X509_STORE_CTX_get_error(ctx);

    printtext_context_init(&ptext_ctx, g_status_window, TYPE_SPEC1_WARN, true);

    X509_NAME_oneline(X509_get_issuer_name(cert), issuer, sizeof issuer);
    X509_NAME_oneline(X509_get_subject_name(cert), subject, sizeof subject);

    if (!ok) {
	printtext(&ptext_ctx, "Error with certificate at depth: %d", depth);
	printtext(&ptext_ctx, "  issuer  = %s", issuer);
	printtext(&ptext_ctx, "  subject = %s", subject);
	printtext(&ptext_ctx, "Reason: %s", X509_verify_cert_error_string(err));
    } else {
	/*Cert verification OK!*/;
    }

    return (ok);
}

static void
set_ciphers(const char *list)
{
    PRINTTEXT_CONTEXT ptext_ctx;
    char strerrbuf[MAXERROR] = { '\0' };

    printtext_context_init(&ptext_ctx, g_status_window, TYPE_SPEC1_WARN, true);

    if (ssl_ctx && list && !SSL_CTX_set_cipher_list(ssl_ctx, list))
	printtext(&ptext_ctx, "warning: set_ciphers: bogus cipher list: %s",
		  xstrerror(EINVAL, strerrbuf, MAXERROR));
}

void
net_ssl_init(void)
{
    PRINTTEXT_CONTEXT ptext_ctx;
    char strerrbuf[MAXERROR] = { '\0' };

    printtext_context_init(&ptext_ctx, g_status_window, TYPE_SPEC1_WARN, true);

    SSL_load_error_strings();
    SSL_library_init();

    if (RAND_load_file("/dev/urandom", 1024) <= 0) {
	printtext(&ptext_ctx, "net_ssl_init: "
	    "Error seeding the PRNG! (%s)",
	    xstrerror(ENOSYS, strerrbuf, MAXERROR));
    }

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    if ((ssl_ctx = SSL_CTX_new(TLS_client_method())) == NULL) {
	err_exit(ENOMEM, "net_ssl_init: Unable to create a new SSL_CTX object");
    } else {
	SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv2);
	SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv3);
	SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_TLSv1);
	SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_TLSv1_1);
    }
#else
    if ((ssl_ctx = SSL_CTX_new(SSLv23_client_method())) == NULL) {
	err_exit(ENOMEM, "net_ssl_init: Unable to create a new SSL_CTX object");
    } else {
	SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv2);
	SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv3);
    }
#endif

    if (config_bool_unparse("ssl_verify_peer", true) &&
	SSL_CTX_set_default_verify_paths(ssl_ctx)) {
	SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, verify_callback);
	SSL_CTX_set_verify_depth(ssl_ctx, 4);
#ifdef WIN32
	if (!SSL_CTX_load_verify_locations(ssl_ctx, "trusted_roots.pem", NULL))
	    printtext(&ptext_ctx, "net_ssl_init: "
		"Error setting default locations for trusted CA certificates");
#endif
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
    else if (strings_match(cs, "insecure") || strings_match(cs, "INSECURE"))
	set_ciphers(suite_insecure);
    else
	set_ciphers(suite_compat);
}

void
net_ssl_deinit(void)
{
    if (ssl) {
	SSL_shutdown(ssl);
	SSL_free(ssl);
	ssl = NULL;
    }
    if (ssl_ctx) {
	SSL_CTX_free(ssl_ctx);
	ssl_ctx = NULL;
    }
}

void
net_ssl_end(void)
{
    if (ssl) {
	SSL_shutdown(ssl);
	SSL_free(ssl);
	ssl = NULL;
    }
}

int
net_ssl_begin(void)
{
    PRINTTEXT_CONTEXT ptext_ctx;
    const int VALUE_HANDSHAKE_OK = 1;

    printtext_context_init(&ptext_ctx, g_status_window, TYPE_SPEC1_FAILURE,
	true);

    if ((ssl = SSL_new(ssl_ctx)) == NULL)
	err_exit(ENOMEM, "net_ssl_begin: Unable to create a new SSL object");
    else if (!SSL_set_fd(ssl, g_socket))
	printtext(&ptext_ctx, "net_ssl_begin: "
	    "Unable to associate the global socket fd with the SSL object");
    else if (SSL_connect(ssl) != VALUE_HANDSHAKE_OK)
	printtext(&ptext_ctx, "net_ssl_begin: Handshake NOT ok!");
    else
	return (0);

    return (-1);
}

int
net_ssl_send(const char *fmt, ...)
{
    char	*buf    = NULL;
    int		 buflen = 0;
    int		 n_sent = 0;
    va_list	 ap;

    if (!ssl)
	return -1;

    va_start(ap, fmt);
    buf = strdup_vprintf(fmt, ap);
    va_end(ap);

    /* message terminate */
    realloc_strcat(&buf, "\r\n");

    if (strlen(buf) > INT_MAX) {
	free(buf);
	return -1;
    }

    buflen = (int) strlen(buf);
    ERR_clear_error();

    if ((n_sent = SSL_write(ssl, buf, buflen)) > 0) {
	free(buf);
	return n_sent;
    }

    free(buf);

    switch (SSL_get_error(ssl, n_sent)) {
    case SSL_ERROR_NONE:
	return 0;
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
	err_log(0, "net_ssl_send: operation did not complete");
	return 0;
    }

    return -1;
}

int
net_ssl_recv(struct network_recv_context *ctx, char *recvbuf, int recvbuf_size)
{
#ifdef UNIX
#define SOCKET_ERROR -1
#endif
    const int maxfdp1 = ctx->sock + 1;
    fd_set readset;
    struct timeval tv = {
	.tv_sec  = ctx->sec,
	.tv_usec = ctx->microsec,
    };

    if (!ssl)
	return -1;

    FD_ZERO(&readset);
    FD_SET(ctx->sock, &readset);

    errno = 0;

    if (select(maxfdp1, &readset, NULL, NULL, &tv) == SOCKET_ERROR) {
	return errno == EINTR ? 0 : -1;
    } else if (!FD_ISSET(ctx->sock, &readset)) {
	return 0;
    } else {
	int bytes_received = 0;

	ERR_clear_error();
	if ((bytes_received = SSL_read(ssl, recvbuf, recvbuf_size)) > 0)
	    return bytes_received;
	switch (SSL_get_error(ssl, bytes_received)) {
	case SSL_ERROR_NONE:
	    return 0;
	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_WRITE:
	    err_log(0, "net_ssl_recv: want read / want write");
	    return 0;
	}
	return -1;
    }

    /*NOTREACHED*/ sw_assert_not_reached();
    /*NOTREACHED*/ return -1;
}

int
net_ssl_check_hostname(const char *host, unsigned int flags)
{
    X509	*cert = NULL;
    int		 ret  = ERR;

    if (ssl == NULL || (cert = SSL_get_peer_certificate(ssl)) == NULL ||
	host == NULL) {
	if (cert)
	    X509_free(cert);
	return ERR;
    }
    ret = X509_check_host(cert, host, 0, flags, NULL) > 0 ? OK : ERR;
    X509_free(cert);
    return ret;
}
