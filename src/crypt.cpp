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

char *
crypt_decrypt_str(cryptstr_const_t str, cryptstr_const_t password,
    const bool rot13)
{
	UNUSED_PARAM(str);
	UNUSED_PARAM(password);
	UNUSED_PARAM(rot13);
	return NULL;
}

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
		if (crypt_get_key_and_iv(password, &crypt_ctx) == -1) {
			throw std::runtime_error("unable to get key/iv");
		} else if ((cipher_ctx = EVP_CIPHER_CTX_new()) == NULL) {
			err_exit(ENOMEM, "crypt_encrypt_str: "
			    "EVP_CIPHER_CTX_new");
		} else if (!EVP_EncryptInit_ex(cipher_ctx, EVP_chacha20(), NULL,
		    addrof(crypt_ctx.key[0]), addrof(crypt_ctx.iv[0]))) {
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

	OPENSSL_cleanse(crypt_ctx.key, sizeof crypt_ctx.key);
	OPENSSL_cleanse(crypt_ctx.iv, sizeof crypt_ctx.iv);
	if (cipher_ctx)
		EVP_CIPHER_CTX_free(cipher_ctx);
	if (encdat != NULL && encdat_size > 0) {
		OPENSSL_cleanse(encdat, encdat_size);
		free(encdat);
	}
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
	return b64_decode(str, NULL, 0) + 1;
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
	return (EVP_BytesToKey(EVP_chacha20(), EVP_sha256(), NULL, password,
	    crypt_strlen(password), PKCS5_DEFAULT_ITER, addrof(ctx->key[0]),
	    addrof(ctx->iv[0])) > 0 ? 0 : -1);
}

int
crypt_strlen(cryptstr_const_t str)
{
	return size_to_int(strlen(reinterpret_cast<const char *>(str)));
}
