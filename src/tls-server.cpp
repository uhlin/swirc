/* TLS server
   Copyright (C) 2021-2022 Markus Uhlin. All rights reserved.

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

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>

#include <stdexcept>
#include <string>

#include "atomicops.h"
#include "config.h"
#include "errHand.h"
#include "libUtils.h"
#include "nestHome.h"
#include "printtext.h"
#include "strHand.h"
#include "strdup_printf.h"
#include "tls-server.h"

const char g_suite_secure[] = "TLSv1.3:TLSv1.2+AEAD+ECDHE:TLSv1.2+AEAD+DHE";
const char g_suite_compat[] = "HIGH:!aNULL";
const char g_suite_legacy[] = "ALL:!ADH:!EXP:!LOW:!MD5:@STRENGTH";
const char g_suite_all[] = "ALL:!aNULL:!eNULL";

volatile bool	g_accepting_new_connections = false;
volatile bool	g_tls_server_loop = false;

static DH	*dh2048 = NULL;
static DH	*dh4096 = NULL;

/*lint -sem(tmp_dh_callback, r_null) */
/*lint -sem(tls_server_setup_context, r_null) */

static char *
get_filename(const char *filename)
{
	std::string str(g_home_dir);

	(void) str.append(SLASH).append(filename);

	return sw_strdup(str.c_str());
}

static int
init_dhparams()
{
	char	*name1 = get_filename(DH_PEM1);
	char	*name2 = get_filename(DH_PEM2);

	try {
		BIO *bio;

		errno = 0;

		/*
		 * DH 2048
		 */
		if ((bio = BIO_new_file(name1, "r")) == NULL)
			throw std::runtime_error("cannot open file");
		dh2048 = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
		BIO_vfree(bio);
		if (dh2048 == NULL) {
			throw std::runtime_error("failed to read dh parameters "
			    "(2048)");
		}

		/*
		 * DH 4096
		 */
		if ((bio = BIO_new_file(name2, "r")) == NULL)
			throw std::runtime_error("cannot open file");
		dh4096 = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
		BIO_vfree(bio);
		if (dh4096 == NULL) {
			throw std::runtime_error("failed to read dh parameters "
			    "(4096)");
		}
	} catch (const std::runtime_error &e) {
		err_log(errno, "init_dhparams: %s", e.what());
		free(name1);
		free(name2);
		return -1;
	}

	free(name1);
	free(name2);
	return 0;
}

static void
set_server_ciphers(SSL_CTX *ctx, const char *list, bool &ok)
{
	if (!SSL_CTX_set_cipher_list(ctx, list))
		ok = false;
}

static DH *
tmp_dh_callback(SSL *ssl, int is_export, int keylength)
{
	DH *ret;

	/* unused */
	(void) ssl;
	(void) is_export;

	if (dh2048 == NULL || dh4096 == NULL) {
		if (init_dhparams() == -1)
			return NULL;
	}

	switch (keylength) {
	case 2048:
		ret = dh2048;
		debug("tmp_dh_callback: keylength = %d", keylength);
		break;
	case 4096:
		ret = dh4096;
		debug("tmp_dh_callback: keylength = %d", keylength);
		break;
	default:
		ret = dh2048;
		debug("tmp_dh_callback: keylength = %d: using 2048 ...",
		    keylength);
		break;
	}

	return ret;
}

static int
verify_callback(int ok, X509_STORE_CTX *ctx)
{
	if (!ok) {
		X509 *cert = X509_STORE_CTX_get_current_cert(ctx);
		char issuer[256]  = { '\0' };
		char subject[256] = { '\0' };
		const int depth = X509_STORE_CTX_get_error_depth(ctx);
		const int err   = X509_STORE_CTX_get_error(ctx);

		(void) X509_NAME_oneline(X509_get_issuer_name(cert), issuer,
		    sizeof issuer);
		(void) X509_NAME_oneline(X509_get_subject_name(cert), subject,
		    sizeof subject);

		debug("Error with certificate at depth: %d", depth);
		debug("  issuer  = %s", issuer);
		debug("  subject = %s", subject);
		debug("Reason: %s", X509_verify_cert_error_string(err));
	}

	return ok;
}

void
tls_server_accept_new_connections(const int port)
{
	BIO			*abio = NULL;
	BIO			*cbio = NULL;
	PRINTTEXT_CONTEXT	 ptext_ctx;
	SSL			*ssl = NULL;
	SSL_CTX			*ctx = NULL;

	printtext_context_init(&ptext_ctx, g_status_window, TYPE_SPEC1_FAILURE,
	    true);

	if (atomic_load_bool(&g_accepting_new_connections)) {
		printtext(&ptext_ctx, "Already accepting connections...");
		return;
	} else {
		(void) atomic_swap_bool(&g_accepting_new_connections, true);
	}

	try {
		if ((ctx = tls_server_setup_context()) == NULL) {
			throw std::runtime_error("Error setting up TLS server "
			    "context. Check the error log.");
		} else if ((abio = tls_server_get_accept_bio(port)) == NULL) {
			throw std::runtime_error("Operation failed");
		}
	} catch (const std::runtime_error &e) {
		SSL_CTX_free(ctx);
		printtext(&ptext_ctx, "%s", e.what());
		(void) atomic_swap_bool(&g_accepting_new_connections, false);
		return;
	}

	ptext_ctx.spec_type = TYPE_SPEC1_SUCCESS;
	printtext(&ptext_ctx, "Accepting connections at port: %d", port);

	do {
		if (BIO_do_accept(abio) <= 0) {
			(void) napms(100);
			continue;
		}

		cbio = BIO_pop(abio);

		if ((ssl = SSL_new(ctx)) == NULL)
			err_exit(ENOMEM, "Out of memory");
		SSL_set_accept_state(ssl);
		SSL_set_bio(ssl, cbio, cbio);
	} while (atomic_load_bool(&g_accepting_new_connections));

	BIO_vfree(abio);
	SSL_CTX_free(ctx);
	ptext_ctx.spec_type = TYPE_SPEC1_WARN;
	printtext(&ptext_ctx, "Stopped accepting connections");
}

void
tls_server_enter_loop(SSL *ssl)
{
	const char prefix[] = "tls_server_enter_loop: SSL_accept: ";

	switch (SSL_accept(ssl)) {
	case 0:
		debug("%sThe TLS/SSL handshake was not successful", prefix);
		return;
	case 1:
		debug("%sThe TLS/SSL handshake was successfully completed",
		    prefix);
		break;
	default:
		debug("%sThe TLS/SSL handshake was not successful because a "
		    "fatal error occurred", prefix);
		return;
	}

	(void) atomic_swap_bool(&g_tls_server_loop, true);

	while (atomic_load_bool(&g_tls_server_loop)) {
		/*
		 * TODO: Add code
		 */

		(void) napms(500);
	}
}

BIO *
tls_server_get_accept_bio(const int port)
{
	BIO	*bio = NULL;
	char	*port_str = NULL;

	try {
		if (port < SERVER_MIN_PORT || port > SERVER_MAX_PORT)
			throw std::runtime_error("Port out of range");
		port_str = strdup_printf("%d", port);
		if ((bio = BIO_new_accept(port_str)) == NULL)
			throw std::runtime_error("Error creating accept BIO");
		free_and_null(&port_str);
		if (BIO_set_nbio_accept(bio, 1) != 1) {
			throw std::runtime_error("Error setting accept socket "
			    "to non-blocking mode");
		} else if (BIO_do_accept(bio) != 1) {
			throw std::runtime_error("Error creating accept socket "
			    "or bind an address to it");
		}
	} catch (const std::runtime_error &e) {
		PRINTTEXT_CONTEXT ctx;

		BIO_vfree(bio);
		free(port_str);
		printtext_context_init(&ctx, g_status_window,
		    TYPE_SPEC1_FAILURE, true);
		printtext(&ctx, "%s", e.what());
		return NULL;
	}

	return bio;
}

SSL_CTX *
tls_server_setup_context(void)
{
	SSL_CTX	*ctx = NULL;
	char	*cafile = NULL;
	char	*certfile = NULL;

	ERR_clear_error();

	try {
		bool		 assignment_ok;
		const char	*cs;
		int		 mode;
		long int	 opts;

		cafile = get_filename(ROOT_PEM);

		if ((ctx = SSL_CTX_new(TLS_server_method())) == NULL) {
			throw std::runtime_error("Out of memory");
		} else if (!SSL_CTX_load_verify_locations(ctx, cafile, NULL)) {
			throw std::runtime_error("Error loading CA file and/or "
			    "directory");
		} else if (!SSL_CTX_set_default_verify_paths(ctx)) {
			throw std::runtime_error("Error loading default "
			    "CA file and/or directory");
		}

		free_and_null(&cafile);
		certfile = get_filename(SERVER_PEM);

		if (SSL_CTX_use_certificate_chain_file(ctx, certfile) != 1) {
			throw std::runtime_error("Error loading certificate "
			    "from file");
		} else if (SSL_CTX_use_PrivateKey_file(ctx, certfile,
		    SSL_FILETYPE_PEM) != 1) {
			throw std::runtime_error("Error loading private key "
			    "from file");
		}

		free_and_null(&certfile);
		mode = (SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT);
		SSL_CTX_set_verify(ctx, mode, verify_callback);
		SSL_CTX_set_verify_depth(ctx, 4);

		opts = SSL_OP_ALL;
		opts |= SSL_OP_NO_SSLv2;
		opts |= SSL_OP_NO_SSLv3;
		opts |= SSL_OP_SINGLE_DH_USE;
		(void) SSL_CTX_set_options(ctx, opts);

		if (!SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION)) {
			throw std::runtime_error("Error setting minimum "
			    "supported protocol version");
		}

		SSL_CTX_set_tmp_dh_callback(ctx, tmp_dh_callback);
		assignment_ok = true;
		cs = Config("server_cipher_suite");

		if (strings_match(cs, "secure") || strings_match(cs, "SECURE"))
			set_server_ciphers(ctx, g_suite_secure, assignment_ok);
		else if (strings_match(cs, "compat") || strings_match(cs,
		    "COMPAT"))
			set_server_ciphers(ctx, g_suite_compat, assignment_ok);
		else if (strings_match(cs, "legacy") || strings_match(cs,
		    "LEGACY"))
			set_server_ciphers(ctx, g_suite_legacy, assignment_ok);
		else if (strings_match(cs, "all") || strings_match(cs, "ALL") ||
		    strings_match(cs, "insecure") || strings_match(cs,
		    "INSECURE"))
			set_server_ciphers(ctx, g_suite_all, assignment_ok);
		else
			set_server_ciphers(ctx, g_suite_compat, assignment_ok);

		if (!assignment_ok)
			throw std::runtime_error("no valid ciphers");
	} catch (const std::runtime_error &ex) {
		const unsigned long int err = ERR_peek_last_error();

		if (err)
			err_log(0, "%s", ERR_error_string(err, NULL));
		err_log(0, "tls_server_setup_context: %s", ex.what());
		SSL_CTX_free(ctx);
		free(cafile);
		free(certfile);
		return NULL;
	}

	return ctx;
}
