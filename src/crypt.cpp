/* crypt.cpp
   Copyright (C) 2022-2025 Markus Uhlin. All rights reserved.

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

#include <climits>
#include <stdexcept>

#include "base64.h"
#include "crypt.h"
#include "errHand.h"
#include "libUtils.h"
#include "strHand.h"

static void
clean_up(EVP_CIPHER_CTX *ctx1, PCRYPT_CTX ctx2, cryptstr_t str, size_t size)
{
	if (ctx1 != nullptr)
		EVP_CIPHER_CTX_free(ctx1);
	if (ctx2 != nullptr) {
		OPENSSL_cleanse(ctx2->key, sizeof ctx2->key);
		OPENSSL_cleanse(ctx2->iv, sizeof ctx2->iv);
	}
	if (str != nullptr && size > 0)
		OPENSSL_cleanse(str, size);
	free(str);
}

static const EVP_CIPHER *
get_encrypt_alg()
{
	return EVP_aes_256_ctr();
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
 * @return     The decrypted string, or nullptr on error.
 */
STRING
crypt_decrypt_str(CSTRING str, cryptstr_const_t password, const bool rot13)
{
	CRYPT_CTX	 crypt_ctx;		/* Key and IV                */
	EVP_CIPHER_CTX	*cipher_ctx = nullptr;	/* Cipher context            */
	STRING		 str_copy = nullptr;	/* Non-const copy of 'str'   */
	bool		 error = false;		/* True if an error occurred */
	cryptstr_t	 decdat = nullptr;	/* Decrypted data            */
	cryptstr_t	 decoded_str = nullptr;	/* Base64 decoded string     */
	cryptstr_t	 out_str = nullptr;	/* Returned on success       */
	int		 decdat_len = 0;	/* Decrypted data length     */
	int		 decdat_size = 0;	/* 'decdat' size             */
	int		 rem_bytes = 0;		/* Remaining bytes           */

	try {
		int	 decode_len = 0;	/* 'decoded_str' size     */
		int	 decode_ret = -1;	/* Retval of b64_decode() */

		if (str == nullptr || password == nullptr)
			throw std::runtime_error("invalid args");

		str_copy = sw_strdup(str);

		if (rot13)
			(void) rot13_str(str_copy);
		if ((decode_len =
		    crypt_get_base64_decode_length(str_copy)) <= 0)
			throw std::runtime_error("length error");

		decoded_str = static_cast<cryptstr_t>(xcalloc(decode_len, 1));

		if ((decode_ret = b64_decode(str_copy, decoded_str, static_cast
		    <size_t>(decode_len))) == -1)
			throw std::runtime_error("base 64 error");

		debug("decode_ret = %d", decode_ret);
		free_and_null(&str_copy);

		if (crypt_get_key_and_iv(password, &crypt_ctx) == -1) {
			throw std::runtime_error("unable to get key/iv");
		} else if ((cipher_ctx = EVP_CIPHER_CTX_new()) == nullptr) {
			err_exit(ENOMEM, "%s: EVP_CIPHER_CTX_new", __func__);
		} else if (!EVP_DecryptInit_ex(cipher_ctx, get_encrypt_alg(),
		    nullptr,
		    addrof(crypt_ctx.key[0]),
		    addrof(crypt_ctx.iv[0]))) {
			throw std::runtime_error("evp decrypt initialization "
			    "failed");
		}

		decdat_size = decode_len +
		    EVP_CIPHER_CTX_block_size(cipher_ctx);
		decdat = static_cast<cryptstr_t>(xcalloc(decdat_size, 1));

		if (!EVP_DecryptUpdate(cipher_ctx, decdat, &decdat_len,
		    decoded_str, decode_len)) {
			throw std::runtime_error("decryption failed!");
		} else if (!EVP_DecryptFinal_ex(cipher_ctx,
		    addrof(decdat[decdat_len]), &rem_bytes)) {
			throw std::runtime_error("decryption finalization "
			    "failed!");
		}

		decdat_len += rem_bytes;
		debug("%s: decdat_len:  %d", __func__, decdat_len);
		debug("%s: decdat_size: %d", __func__, decdat_size);

		EVP_CIPHER_CTX_free(cipher_ctx);
		cipher_ctx = nullptr;

		out_str = static_cast<cryptstr_t>(xmalloc(int_sum(decdat_len,
		    1)));
		out_str[decdat_len] = '\0';
		memcpy(out_str, decdat, static_cast<size_t>(decdat_len));
	} catch (const std::runtime_error &e) {
		error = true;
		err_log(0, "%s: %s", __func__, e.what());
	} catch (...) {
		error = true;
		err_log(0, "%s: %s", __func__, "unknown exception!");
	}

	clean_up(cipher_ctx, &crypt_ctx, decdat, static_cast<size_t>
	    (decdat_size));

	free(str_copy);
	free(decoded_str);

	if (error) {
		free(out_str);
		return nullptr;
	}

	return reinterpret_cast<STRING>(out_str);
}

/**
 * Encrypts a string. The storage is allocated on the heap and must be free()'d.
 *
 * @param[in]  str       Null-terminated plaintext
 * @param[in]  password  Encryption password
 * @param[in]  rot13     Shall rot13 be used?
 *
 * @return     The encrypted string, or nullptr on error.
 */
STRING
crypt_encrypt_str(cryptstr_const_t str, cryptstr_const_t password,
    const bool rot13)
{
	CRYPT_CTX	 crypt_ctx;		/* Key and IV                */
	EVP_CIPHER_CTX	*cipher_ctx = nullptr;	/* Cipher context            */
	STRING		 b64str = nullptr;	/* Base64 string             */
	bool		 error = false;		/* True if an error occurred */
	cryptstr_t	 encdat = nullptr;	/* Encrypted data            */
	int		 encdat_len = 0;	/* Encrypted data length     */
	int		 encdat_size = 0;	/* 'encdat' size             */
	int		 rem_bytes = 0;		/* Remaining bytes           */

	try {
		uint32_t size = 0;

		if (str == nullptr || password == nullptr) {
			throw std::runtime_error("invalid args");
		} else if (crypt_get_key_and_iv(password, &crypt_ctx) == -1) {
			throw std::runtime_error("unable to get key/iv");
		} else if ((cipher_ctx = EVP_CIPHER_CTX_new()) == nullptr) {
			err_exit(ENOMEM, "%s: EVP_CIPHER_CTX_new", __func__);
		} else if (!EVP_EncryptInit_ex(cipher_ctx, get_encrypt_alg(),
		    nullptr,
		    addrof(crypt_ctx.key[0]),
		    addrof(crypt_ctx.iv[0]))) {
			throw std::runtime_error("evp encrypt initialization "
			    "failed");
		}

		encdat_size = crypt_strlen(str) +
		    EVP_CIPHER_CTX_block_size(cipher_ctx) + 1;
		encdat = static_cast<cryptstr_t>(xmalloc(encdat_size));

		if (!EVP_EncryptUpdate(cipher_ctx, encdat, &encdat_len, str,
		    crypt_strlen(str) + 1)) {
			throw std::runtime_error("encryption failed!");
		} else if (!EVP_EncryptFinal_ex(cipher_ctx,
		    addrof(encdat[encdat_len]), &rem_bytes)) {
			throw std::runtime_error("encryption finalization "
			    "failed!");
		}

		encdat_len += rem_bytes;
		debug("%s: encdat_len:  %d", __func__, encdat_len);
		debug("%s: encdat_size: %d", __func__, encdat_size);

		EVP_CIPHER_CTX_free(cipher_ctx);
		cipher_ctx = nullptr;

		if ((size = crypt_get_base64_encode_length(encdat_len)) == 0)
			throw std::runtime_error("base64 length error");
		b64str = static_cast<STRING>(xmalloc(size));
		b64str[size - 1] = '\0';

		if (b64_encode(encdat, encdat_len, b64str, size) == -1)
			throw std::runtime_error("base64 error");
	} catch (const std::runtime_error &e) {
		error = true;
		err_log(0, "%s: %s", __func__, e.what());
	} catch (...) {
		error = true;
		err_log(0, "%s: %s", __func__, "unknown exception!");
	}

	clean_up(cipher_ctx, &crypt_ctx, encdat, static_cast<size_t>
	    (encdat_size));

	if (error) {
		free(b64str);
		return nullptr;
	}

	return (rot13 ? rot13_str(b64str) : b64str);
}

void
crypt_freezero(void *vp, const size_t len)
{
	if (vp != nullptr && len > 0)
		OPENSSL_cleanse(vp, len);
	free(vp);
}

int
crypt_get_base64_decode_length(CSTRING str)
{
	int len;

	if (str == nullptr || strings_match(str, "") ||
	    (len = b64_decode(str, nullptr, 0)) < 0)
		return 0;
	return int_sum(len, 1);
}

uint32_t
crypt_get_base64_encode_length(const uint32_t n)
{
	return (((4 * n / 3) + 3) & ~3) + 1;
}

int
crypt_get_key_and_iv(cryptstr_const_t password, PCRYPT_CTX ctx)
{
	if (password == nullptr || ctx == nullptr)
		return -1;
	return (EVP_BytesToKey(get_encrypt_alg(), EVP_sha256(), nullptr,
	    password, crypt_strlen(password), PKCS5_DEFAULT_ITER,
	    addrof(ctx->key[0]), addrof(ctx->iv[0])) > 0 ? 0 : -1);
}

int
crypt_strlen(cryptstr_const_t str)
{
	cryptstr_const_t ptr = str;

	while (*ptr != '\0')
		ptr++;
	if ((ptr - str) > INT_MAX)
		throw std::overflow_error("integer maximum exceeded");
	return static_cast<int>(ptr - str);
}
