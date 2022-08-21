/* crypt.cpp
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

#include <cmath>
#include <stdexcept>

#include "base64.h"
#include "crypt.h"
#include "errHand.h"
#include "libUtils.h"
#include "strHand.h"

static void
clean_up(EVP_CIPHER_CTX *ctx1, PCRYPT_CTX ctx2)
{
	if (ctx1)
		EVP_CIPHER_CTX_free(ctx1);
	if (ctx2) {
		OPENSSL_cleanse(ctx2->key, sizeof ctx2->key);
		OPENSSL_cleanse(ctx2->iv, sizeof ctx2->iv);
	}
}

static const EVP_CIPHER *
get_encrypt_alg()
{
	return EVP_chacha20();
}

/**
 * Decrypts a string. The storage is allocated on the heap and must be free()'d.
 *
 * @param[in]  str       The string to be decrypted which must be null-
 *                       terminated and encoded with base64 (and possibly with a
 *                       rot13 layer)
 * @param[in]  password  Decryption password
 * @param[in]  rot13     Shall rot13 be ran on the string initially, yes/no?
 *
 * @return     The decrypted string, or NULL on error.
 */
char *
crypt_decrypt_str(const char *str, cryptstr_const_t password, const bool rot13)
{
	CRYPT_CTX	 crypt_ctx;		/* Key and IV                */
	EVP_CIPHER_CTX	*cipher_ctx = NULL;	/* Cipher context            */
	bool		 error = false;		/* True if an error occurred */
	char		*str_copy = NULL;	/* Non-const copy of 'str'   */
	cryptstr_t	 decdat = NULL;		/* Decrypted data            */
	cryptstr_t	 decoded_str = NULL;	/* Base64 decoded string     */
	cryptstr_t	 out_str = NULL;	/* Returned on success       */
	int		 decdat_len = 0,	/* Decrypted data length     */
			 decdat_size = 0;	/* 'decdat' size             */
	int		 decode_len = 0,	/* 'decoded_str' size        */
			 decode_ret = -1;	/* Retval of b64_decode()    */
	int		 rem_bytes = 0;		/* Remaining bytes           */

	try {
		if (str == NULL || password == NULL)
			throw std::runtime_error("invalid args");

		str_copy = sw_strdup(str);

		if (rot13)
			(void) rot13_str(str_copy);
		if ((decode_len = crypt_get_base64_decode_length(str_copy)) <=
		    0)
			throw std::runtime_error("length error");

		decoded_str = static_cast<cryptstr_t>(xmalloc(decode_len));

		if ((decode_ret = b64_decode(str, decoded_str, decode_len)) ==
		    -1 || decode_ret != decode_len)
			throw std::runtime_error("base 64 error");

		free_and_null(&str_copy);

		if (crypt_get_key_and_iv(password, &crypt_ctx) == -1) {
			throw std::runtime_error("unable to get key/iv");
		} else if ((cipher_ctx = EVP_CIPHER_CTX_new()) == NULL) {
			err_exit(ENOMEM, "crypt_decrypt_str: "
			    "EVP_CIPHER_CTX_new");
		} else if (!EVP_DecryptInit_ex(cipher_ctx, get_encrypt_alg(),
		    NULL, addrof(crypt_ctx.key[0]), addrof(crypt_ctx.iv[0]))) {
			throw std::runtime_error("evp decrypt initialization "
			    "failed");
		}

		decdat_size = decode_len +
		    EVP_CIPHER_CTX_block_size(cipher_ctx);
		decdat = static_cast<cryptstr_t>(xmalloc(decdat_size));

		if (!EVP_DecryptUpdate(cipher_ctx, decdat, &decdat_len,
		    decoded_str, decode_len)) {
			throw std::runtime_error("decryption failed!");
		} else if (!EVP_DecryptFinal_ex(cipher_ctx,
		    addrof(decdat[decdat_len]), &rem_bytes)) {
			throw std::runtime_error("decryption finalization "
			    "failed!");
		}

		decdat_len += rem_bytes;
		debug("decdat_len: %d", decdat_len);
		debug("decdat_size: %d", decdat_size);

		EVP_CIPHER_CTX_free(cipher_ctx);
		cipher_ctx = NULL;

		out_str = static_cast<cryptstr_t>(xmalloc(int_sum(decdat_len,
		    1)));
		out_str[decdat_len] = '\0';
		memcpy(out_str, decdat, decdat_len);
	} catch (const std::runtime_error &e) {
		error = true;
		err_log(0, "crypt_decrypt_str: %s", e.what());
		/* FALLTHROUGH */
	}

	clean_up(cipher_ctx, &crypt_ctx);
	free(str_copy);
	if (decdat != NULL && decdat_size > 0)
		OPENSSL_cleanse(decdat, decdat_size);
	free(decdat);
	free(decoded_str);
	if (error) {
		free(out_str);
		return NULL;
	}
	return reinterpret_cast<char *>(out_str);
}

/**
 * Encrypts a string. The storage is allocated on the heap and must be free()'d.
 *
 * @param[in]  str       Null-terminated plaintext
 * @param[in]  password  Encryption password
 * @param[in]  rot13     Shall rot13 be used?
 *
 * @return     The encrypted string, or NULL on error.
 */
char *
crypt_encrypt_str(cryptstr_const_t str, cryptstr_const_t password,
    const bool rot13)
{
	CRYPT_CTX	 crypt_ctx;
	EVP_CIPHER_CTX	*cipher_ctx = NULL;
	bool		 error = false;
	char		*b64str = NULL;
	cryptstr_t	 encdat = NULL;
	int		 encdat_len = 0,
			 encdat_size = 0;
	int		 rem_bytes = 0;
	int		 size = 0;

	try {
		if (str == NULL || password == NULL) {
			throw std::runtime_error("invalid args");
		} else if (crypt_get_key_and_iv(password, &crypt_ctx) == -1) {
			throw std::runtime_error("unable to get key/iv");
		} else if ((cipher_ctx = EVP_CIPHER_CTX_new()) == NULL) {
			err_exit(ENOMEM, "crypt_encrypt_str: "
			    "EVP_CIPHER_CTX_new");
		} else if (!EVP_EncryptInit_ex(cipher_ctx, get_encrypt_alg(),
		    NULL, addrof(crypt_ctx.key[0]), addrof(crypt_ctx.iv[0]))) {
			throw std::runtime_error("evp encrypt initialization "
			    "failed");
		}

		encdat_size = crypt_strlen(str) +
		    EVP_CIPHER_CTX_block_size(cipher_ctx) + 1;
		encdat = static_cast<cryptstr_t>(xmalloc(encdat_size));

		if (!EVP_EncryptUpdate(cipher_ctx, encdat, &encdat_len, str,
		    crypt_strlen(str))) {
			throw std::runtime_error("encryption failed!");
		} else if (encdat_len >= encdat_size) {
			errno = EOVERFLOW;
			err_dump("crypt_encrypt_str");
		} else if (!EVP_EncryptFinal_ex(cipher_ctx,
		    addrof(encdat[encdat_len]), &rem_bytes)) {
			throw std::runtime_error("encryption finalization "
			    "failed!");
		}

		encdat_len += rem_bytes;
		debug("crypt_encrypt_str: encdat_len:  %d", encdat_len);
		debug("crypt_encrypt_str: encdat_size: %d", encdat_size);

		EVP_CIPHER_CTX_free(cipher_ctx);
		cipher_ctx = NULL;

		size = crypt_get_base64_encode_length(encdat_len);
		b64str = static_cast<char *>(xmalloc(size));
		b64str[size - 1] = '\0';

		if (b64_encode(encdat, encdat_len, b64str, size) == -1)
			throw std::runtime_error("base64 error");
	} catch (const std::runtime_error &e) {
		error = true;
		err_log(0, "crypt_encrypt_str: %s", e.what());
		/* FALLTHROUGH */
	}

	clean_up(cipher_ctx, &crypt_ctx);
	if (encdat != NULL && encdat_size > 0)
		OPENSSL_cleanse(encdat, encdat_size);
	free(encdat);
	if (error) {
		free(b64str);
		return NULL;
	}
	return (rot13 ? rot13_str(b64str) : b64str);
}

int
crypt_get_base64_decode_length(const char *str)
{
	if (str == NULL)
		return 0;
	return b64_decode(str, NULL, 0);
}

int
crypt_get_base64_encode_length(const int n)
{
	double	d;

	d = static_cast<double>(n);
	return static_cast<int>(ceil(4.0 * d / 3.0)) + 1;
}

int
crypt_get_key_and_iv(cryptstr_const_t password, PCRYPT_CTX ctx)
{
	if (password == NULL || ctx == NULL)
		return -1;
	return (EVP_BytesToKey(get_encrypt_alg(), EVP_sha256(), NULL, password,
	    crypt_strlen(password), PKCS5_DEFAULT_ITER, addrof(ctx->key[0]),
	    addrof(ctx->iv[0])) > 0 ? 0 : -1);
}

int
crypt_strlen(cryptstr_const_t str)
{
	return size_to_int(strlen(reinterpret_cast<const char *>(str)));
}
