/* Copyright (c) 2016 Markus Uhlin <markus.uhlin@bredband.net>
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

#include <openssl/opensslv.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include "assertAPI.h"
#include "config.h"
#include "errHand.h"
#include "libUtils.h"
#include "network.h"
#include "printtext.h"
#include "strdup_printf.h"

static SSL_CTX	*ssl_ctx = NULL;
static SSL	*ssl	 = NULL;

static const char cipher_list[] = "HIGH:@STRENGTH";

static int
verify_callback(int ok, X509_STORE_CTX *ctx)
{
    X509      *cert         = X509_STORE_CTX_get_current_cert(ctx);
    char       issuer[256]  = "";
    char       subject[256] = "";
    const int  depth        = X509_STORE_CTX_get_error_depth(ctx);
    const int  err          = X509_STORE_CTX_get_error(ctx);
    struct printtext_context ptext_ctx = {
	.window	    = g_status_window,
	.spec_type  = TYPE_SPEC1_WARN,
	.include_ts = true,
    };

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

void
net_ssl_init()
{
    struct printtext_context ptext_ctx = {
	.window	    = g_status_window,
	.spec_type  = TYPE_SPEC1_WARN,
	.include_ts = true,
    };

    SSL_load_error_strings();
    SSL_library_init();

    if (RAND_load_file("/dev/urandom", 1024) <= 0) {
	printtext(&ptext_ctx, "net_ssl_init: Error seeding the PRNG! LibreSSL?: %s", strerror(ENOSYS));
    }

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    if (( ssl_ctx = SSL_CTX_new(TLS_client_method()) ) == NULL) {
	err_exit(ENOMEM, "net_ssl_init: Unable to create a new SSL_CTX object");
    } else {
	SSL_CTX_set_options(ssl_ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
			    SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
    }
#else
    if (( ssl_ctx = SSL_CTX_new(SSLv23_client_method()) ) == NULL) {
	err_exit(ENOMEM, "net_ssl_init: Unable to create a new SSL_CTX object");
    } else {
	SSL_CTX_set_options(ssl_ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
    }
#endif

    if (config_bool_unparse("ssl_verify_peer", true) && SSL_CTX_set_default_verify_paths(ssl_ctx)) {
	SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, verify_callback);
	SSL_CTX_set_verify_depth(ssl_ctx, 4);
    } else {
	printtext(&ptext_ctx, "net_ssl_init: Certificate verification is disabled: Option set to NO?");
    }

    if (!SSL_CTX_set_cipher_list(ssl_ctx, cipher_list))
	printtext(&ptext_ctx, "net_ssl_init: Bogus cipher list: %s", strerror(EINVAL));
}

void
net_ssl_deinit()
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
net_ssl_close()
{
    if (ssl) {
	SSL_shutdown(ssl);
	SSL_free(ssl);
	ssl = NULL;
    }
}

int
net_ssl_start()
{
    struct printtext_context ptext_ctx = {
	.window	    = g_status_window,
	.spec_type  = TYPE_SPEC1_FAILURE,
	.include_ts = true,
    };
    const int VALUE_HANDSHAKE_OK = 1;

    if ((ssl = SSL_new(ssl_ctx)) == NULL)
	err_exit(ENOMEM, "net_ssl_start: Unable to create a new SSL object");
    else if (!SSL_set_fd(ssl, g_socket))
	printtext(&ptext_ctx, "net_ssl_start: Unable to associate the global socket fd with the SSL object");
    else if (SSL_connect(ssl) != VALUE_HANDSHAKE_OK)
	printtext(&ptext_ctx, "net_ssl_start: Handshake NOT ok!");
    else
	return (0);

    return (-1);
}

int
net_ssl_send(const char *fmt, ...)
{
    va_list     ap;
    char       *buffer;
    const char  message_terminate[] = "\r\n";

    if (!ssl)
	return -1;

    va_start(ap, fmt);
    buffer = Strdup_vprintf(fmt, ap);
    va_end(ap);

    realloc_strcat(&buffer, message_terminate);

    int total_written = 0;
    int ret = 0;
    while (total_written < strlen(buffer)) {
	if ((ret = SSL_write(ssl, &buffer[total_written], strlen(buffer) - total_written)) <= 0) {
	    free(buffer);
	    return -1;
	} else {
	    total_written += ret;
	}
    }

    free(buffer);
    return total_written;
}

int
net_ssl_recv(struct network_recv_context *ctx, char *recvbuf, int recvbuf_size)
{
#ifdef UNIX
#define SOCKET_ERROR -1
#endif
    fd_set		readset;
    struct timeval	tv;
    const int		maxfdp1 = ctx->sock + 1;

    FD_ZERO(&readset);
    FD_SET(ctx->sock, &readset);

    tv.tv_sec  = ctx->sec;
    tv.tv_usec = ctx->microsec;

    errno = 0;

    if (select(maxfdp1, &readset, NULL, NULL, &tv) == SOCKET_ERROR) {
	return errno == EINTR ? 0 : -1;
    } else if (!FD_ISSET(ctx->sock, &readset)) {
	return 0;
    } else {
	int ret = 0;
	int total_read = 0;

	ret = SSL_read(ssl, recvbuf, recvbuf_size);
	switch (SSL_get_error(ssl, ret)) {
	case SSL_ERROR_NONE:
	    total_read = ret;
	    break;
	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_WRITE:
	    break;
	default:
	    return -1;
	}

	return total_read;
    }

    /* NOTREACHED */
    sw_assert_not_reached();
    return -1;
}
