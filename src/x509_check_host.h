#ifndef GUARD_CHECK_HOST_H
#define GUARD_CHECK_HOST_H

#include <openssl/opensslv.h>
#include <openssl/x509v3.h>

#if OPENSSL_VERSION_NUMBER < 0x1000200fL
#define X509_CHECK_FLAG_ALWAYS_CHECK_SUBJECT	0x1
#define X509_CHECK_FLAG_MULTI_LABEL_WILDCARDS	0x8
#define X509_CHECK_FLAG_NEVER_CHECK_SUBJECT	0x20
#define X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS	0x4
#define X509_CHECK_FLAG_NO_WILDCARDS		0x2
#define X509_CHECK_FLAG_SINGLE_LABEL_SUBDOMAINS 0x10

#define _X509_CHECK_FLAG_DOT_SUBDOMAINS 0x8000

int X509_check_host(X509 *x, const char *chk, size_t chklen, unsigned int flags,
		    char **peername);
#else
#define HAVE_X509_CHECK_HOST 1
#endif
#endif /* GUARD_CHECK_HOST_H */
