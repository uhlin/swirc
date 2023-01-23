#ifndef GET_X509_FINGERPRINT_HPP
#define GET_X509_FINGERPRINT_HPP

#include <openssl/bio.h>
#include <openssl/x509.h>

class x509_fingerprint {
public:
	x509_fingerprint();
	explicit x509_fingerprint(const char *);
	~x509_fingerprint();

	void show_fp(void) const;

private:
	BIO		*bio;
	X509		*cert;
	const EVP_MD	*alg;
	unsigned char	 md[EVP_MAX_MD_SIZE];
	unsigned int	 md_len;
};

#endif
