#ifndef CRYPT_H
#define CRYPT_H

#include <openssl/evp.h>

typedef unsigned char cryptarray_t[];
typedef const unsigned char *cryptstr_const_t;
typedef unsigned char *cryptstr_t;

typedef struct tagCRYPT_CTX {
	unsigned char	key[EVP_MAX_KEY_LENGTH];
	unsigned char	iv[EVP_MAX_IV_LENGTH];
} CRYPT_CTX, *PCRYPT_CTX;

/*lint -sem(crypt_decrypt_str, r_null) */
/*lint -sem(crypt_encrypt_str, r_null) */

__SWIRC_BEGIN_DECLS
char	*crypt_decrypt_str(const char *, cryptstr_const_t, const bool);
char	*crypt_encrypt_str(cryptstr_const_t, cryptstr_const_t, const bool);
int	 crypt_get_base64_decode_length(const char *);
int	 crypt_get_base64_encode_length(const int);
int	 crypt_get_key_and_iv(cryptstr_const_t password, PCRYPT_CTX);
int	 crypt_strlen(cryptstr_const_t);
__SWIRC_END_DECLS

#endif
