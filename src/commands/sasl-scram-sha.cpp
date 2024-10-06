/* SASL auth mechanism SCRAM-SHA-256
   Copyright (C) 2019-2024 Markus Uhlin. All rights reserved.

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

#include "base64.h"
#include "common.h"

#if !defined(BSD) && !defined(WIN32)
#include <random>
#endif
#include <stdexcept>
#include <string>

#include <openssl/evp.h>
#include <openssl/hmac.h>

#if WIN32
extern "C" {
#include "compat/stdlib.h" /* arc4random_uniform() */
}
#endif

#include "../assertAPI.h"
#include "../config.h"
#include "../dataClassify.h"
#include "../errHand.h"
#include "../libUtils.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../strdup_printf.h"

#include "sasl-scram-sha.h"

struct digest_context {
	UCHARPTR		 key;
	int			 key_len;
	const unsigned char	*d;
	size_t			 n;
	unsigned char		 md[EVP_MAX_MD_SIZE];
	unsigned int		 md_len;

	digest_context(UCHARPTR, int, const unsigned char *, size_t);
};

digest_context::digest_context(UCHARPTR key, int key_len,
    const unsigned char *d, size_t n)
{
	this->key = key;
	this->key_len = key_len;
	this->d = d;
	this->n = n;
	BZERO(this->md, sizeof this->md);
	this->md_len = 0;
}

volatile bool	g_sasl_scram_sha_got_first_msg = false;

static STRING	 complete_nonce = NULL;
static char	 nonce[64] = { '\0' };

static const unsigned char	client_key_str[] = "Client Key";
static const unsigned char	server_key_str[] = "Server Key";
static unsigned char		signature_expected[EVP_MAX_MD_SIZE] = { '\0' };
static unsigned int		signature_expected_len = 0;

/*lint -sem(get_decoded_msg, r_null) */
/*lint -sem(get_salted_password, r_null) */

static void
generate_and_store_nonce(void)
{
	static const char legal_index[] =
	    "!\"#$%&'()*+-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
	    "abcdefghijklmnopqrstuvwxyz{|}~";

#if defined(BSD) || defined(WIN32)
	for (size_t i = 0; i < ARRAY_SIZE(nonce); i++) {
		nonce[i] = legal_index[arc4random_uniform
		    (sizeof legal_index - 1)];
	}
#else
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<size_t> dist(0, strlen(legal_index) - 1);

	for (size_t i = 0; i < ARRAY_SIZE(nonce); i++)
		nonce[i] = legal_index[dist(gen)];
#endif

	nonce[ARRAY_SIZE(nonce) - 1] = '\0';
	debug("%s: nonce: %s", __func__, nonce);
}

static CSTRING
get_encoded_msg(CSTRING source)
{
	static char	encoded_msg[4096] = { '\0' };

	if (b64_encode(reinterpret_cast<const uint8_t *>(source),
	    strlen(source), encoded_msg, ARRAY_SIZE(encoded_msg)) == -1)
		return "";
	return (&encoded_msg[0]);
}

/* C: n,,n=user,r=rOprNGfwEbeRWgbNEkqO */
int
sasl_scram_sha_send_client_first_msg(void)
{
	CSTRING encoded_msg;
	CSTRING username = Config("sasl_username");
	STRING msg;

	if (!is_valid_username(username))
		return -1;

	generate_and_store_nonce();

	msg = strdup_printf("n,,n=%s,r=%s", username, nonce);
	encoded_msg = get_encoded_msg(msg);
	free(msg);

	if (strings_match(encoded_msg, "") || net_send("AUTHENTICATE %s",
	    encoded_msg) < 0)
		return -1;
	return 0;
}

/* C: c=biws,r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0,
      p=dHzbZapWIk4jUhN+Ute9ytag9zjfMHgsqmmiz7AndVQ= */
int
sasl_scram_sha_send_client_final_msg(CSTRING proof)
{
	STRING		 cli_final_msg;
	size_t		 size;
	std::string	 str("c=biws,r=");

	if (complete_nonce == NULL || proof == NULL)
		return -1;

	(void) str.append(complete_nonce);
	(void) str.append(",p=").append(proof);

	size = str.length() + 1;
	cli_final_msg = new char[size];

	if (sw_strcpy(cli_final_msg, str.c_str(), size) != 0 ||
	    net_send("AUTHENTICATE %s", get_encoded_msg(cli_final_msg)) < 0) {
		delete[] cli_final_msg;
		return -1;
	}

	debug("%s: C: %s", __func__, cli_final_msg);
	delete[] cli_final_msg;
	return 0;
}

static STRING
get_decoded_msg(CSTRING source, int *outlen)
{
	STRING	 decoded_msg = NULL;
	int	 length_needed, tmp;

	if (source == NULL || strings_match(source, "") ||
	    (tmp = b64_decode(source, NULL, 0)) < 0)
		return NULL;
	if (outlen)
		*outlen = tmp;

	length_needed = int_sum(tmp, 1);
	decoded_msg = new char[length_needed];
	decoded_msg[length_needed - 1] = '\0';

	if (b64_decode(source, reinterpret_cast<uint8_t *>(decoded_msg),
	    length_needed) == -1) {
		delete[] decoded_msg;
		return NULL;
	}

	return decoded_msg;
}

/* S: r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0,
      s=W22ZaJ0SNY7soEsUEjb6gQ==,i=4096 */
static int
get_sfm_components(CSTRING msg, unsigned char **salt, int *saltlen,
    int *iter)
{
	STRING	 decoded_msg = NULL;
	bool	 ok = false;

	*salt = NULL;
	*saltlen = 0;
	*iter = PKCS5_DEFAULT_ITER;

	try {
		char *cp, *b64salt;
		size_t n;

		if ((decoded_msg = get_decoded_msg(msg, NULL)) == NULL) {
			throw std::runtime_error("unable to get decoded "
			    "message");
		}

		debug("%s: S: %s", __func__, decoded_msg);
		cp = decoded_msg;

		if (strncmp(cp, "r=", 2) != STRINGS_MATCH)
			throw std::runtime_error("expected nonce");

		cp += 2;

		if (strncmp(cp, nonce, strlen(nonce)) != STRINGS_MATCH)
			throw std::runtime_error("nonce mismatch");

		n = strcspn(cp, ",");
		cp[n] = '\0';
		free(complete_nonce);
		complete_nonce = sw_strdup(cp);
		cp[n] = ',';

		if ((cp = strstr(cp, ",s=")) == NULL)
			throw std::runtime_error("no base64-encoded salt");

		cp += 3;
		n = strcspn(cp, ",");
		cp[n] = '\0';
		b64salt = sw_strdup(cp);
		cp[n] = ',';
		*salt = reinterpret_cast<UCHARPTR>(get_decoded_msg(b64salt,
		    saltlen));
		free(b64salt);

		if (*salt == NULL)
			throw std::runtime_error("unable to decode salt");
		else if ((cp = strstr(cp, ",i=")) == NULL)
			throw std::runtime_error("no iteration count");

		cp += 3;

		if (!is_numeric(cp))
			throw std::runtime_error("iteration count not numeric");

		*iter = static_cast<int>(strtol(cp, NULL, 10));
		ok = true;
	} catch (const std::runtime_error &e) {
		delete[] *salt;
		*salt = NULL;
		*saltlen = 0;
		*iter = PKCS5_DEFAULT_ITER;
		err_log(0, "%s: %s", __func__, e.what());
	}

	if (decoded_msg)
		delete[] decoded_msg;
	return (ok ? 0 : -1);
}

/*
 * SaltedPassword: Hi(Normalize(password), salt, i)
 */
static UCHARPTR
get_salted_password(const unsigned char *salt, int saltlen, int iter,
    int *outsize)
{
	UCHARPTR out = NULL;

	try {
		CSTRING pass;

		if (*outsize = EVP_MD_size(EVP_sha256()), *outsize < 0) {
			throw std::runtime_error("message digest size "
			    "negative");
		}

		out = new unsigned char[*outsize];

		if ((pass = config_get_normalized_sasl_password()) == NULL) {
			throw std::runtime_error("unable to get normalized "
			    "sasl password");
		} else if (!PKCS5_PBKDF2_HMAC(pass, -1, salt, saltlen, iter,
		    EVP_sha256(), *outsize, out)) {
			throw std::runtime_error("unable to get salted "
			    "password");
		}
	} catch (const std::bad_alloc &e) {
		err_exit(ENOMEM, "%s: %s", __func__, e.what());
	} catch (const std::runtime_error &e) {
		*outsize = 0;
		delete[] out;
		err_log(0, "%s: %s", __func__, e.what());
		return NULL;
	} catch (...) {
		sw_assert_not_reached();
	}

	return out;
}

static int
get_digest(struct digest_context *ctx)
{
	if (HMAC(EVP_sha256(), ctx->key, ctx->key_len, ctx->d, ctx->n, ctx->md,
	    & (ctx->md_len)) == NULL)
		return -1;
	return 0;
}

static STRING
get_client_first_msg_bare()
{
	STRING		 msg_bare;
	size_t		 size;
	std::string	 str("n=");

	(void) str.append(Config("sasl_username"));
	(void) str.append(",r=").append(nonce);

	size = str.length() + 1;
	msg_bare = new char[size];
	errno = sw_strcpy(msg_bare, str.c_str(), size);
	sw_assert_perror(errno);

	return msg_bare;
}

static STRING
get_client_final_msg_wo_proof()
{
	STRING		 msg_wo_proof;
	size_t		 size;
	std::string	 str("c=biws,r=");

	(void) str.append(complete_nonce ? complete_nonce : "");

	size = str.length() + 1;
	msg_wo_proof = new char[size];
	errno = sw_strcpy(msg_wo_proof, str.c_str(), size);
	sw_assert_perror(errno);

	return msg_wo_proof;
}

/*
 * AuthMessage: client-first-message-bare + "," +
 *              server-first-message + "," +
 *              client-final-message-without-proof
 */
static UCHARPTR
get_auth_msg(CSTRING b64msg, size_t *auth_msg_len)
{
	STRING msg_bare, msg_wo_proof, serv_first_msg;
	STRING out;

	try {
		msg_bare = get_client_first_msg_bare();
		msg_wo_proof = get_client_final_msg_wo_proof();
		serv_first_msg = get_decoded_msg(b64msg, NULL);
	} catch (const std::bad_alloc &e) {
		err_exit(ENOMEM, "%s: fatal: %s", __func__, e.what());
	}

	out = strdup_printf("%s,%s,%s", msg_bare, serv_first_msg, msg_wo_proof);

	delete[] msg_bare;
	delete[] msg_wo_proof;
	delete[] serv_first_msg;

	*auth_msg_len = strlen(out);
	return reinterpret_cast<UCHARPTR>(out);
}

int
sasl_scram_sha_handle_serv_first_msg(CSTRING msg)
{
	char		 proof[EVP_MAX_MD_SIZE] = { '\0' };
	int		 iter = PKCS5_DEFAULT_ITER;
	int		 passwdlen = 0;
	int		 saltlen = 0;
	size_t		 auth_msg_len;
	UCHARPTR	 auth_msg;
	UCHARPTR	 pass = NULL;
	UCHARPTR	 salt = NULL;
	unsigned char	 stored_key[SHA256_DIGEST_LENGTH];

	if (get_sfm_components(msg, &salt, &saltlen, &iter) == -1 ||
	    (pass = get_salted_password(salt, saltlen, iter, &passwdlen)) ==
	    NULL) {
		delete[] salt;
		return -1;
	}

	delete[] salt;

/***************************************************
 *
 * ClientKey: HMAC(SaltedPassword, "Client Key")
 * ServerKey: HMAC(SaltedPassword, "Server Key")
 *
 */

	struct digest_context client_key(pass, passwdlen, client_key_str, 10);
	struct digest_context server_key(pass, passwdlen, server_key_str, 10);

	if (get_digest(&client_key) == -1 || get_digest(&server_key) == -1) {
		delete[] pass;
		return -1;
	}

	delete[] pass;

	/*
	 * StoredKey: H(ClientKey)
	 */
	if (SHA256(client_key.md, client_key.md_len, stored_key) == NULL)
		return -1;

	auth_msg_len = 0;
	auth_msg = get_auth_msg(msg, &auth_msg_len);

/***************************************************
 *
 * ClientSignature: HMAC(StoredKey, AuthMessage)
 * ServerSignature: HMAC(ServerKey, AuthMessage)
 *
 */

	struct digest_context client_signature(stored_key, SHA256_DIGEST_LENGTH,
	    auth_msg, auth_msg_len);
	struct digest_context server_signature(server_key.md, server_key.md_len,
	    auth_msg, auth_msg_len);

	if (get_digest(&client_signature) == -1 ||
	    get_digest(&server_signature) == -1) {
		free(auth_msg);
		return -1;
	}

	free(auth_msg);

	(void) memcpy(signature_expected, server_signature.md,
	    server_signature.md_len);
	signature_expected_len = server_signature.md_len;

	/*
	 * ClientProof: ClientKey XOR ClientSignature
	 */
	for (unsigned int i = 0;
	    i < MIN(client_key.md_len, client_signature.md_len);
	    i++)
		proof[i] = client_key.md[i] ^ client_signature.md[i];

	return sasl_scram_sha_send_client_final_msg(get_encoded_msg(proof));
}

/*
 * S: v=6rriTRBi23WpRR/wtup+mMhUZUn/dB5nLTJRsjl95G4=
 */
int
sasl_scram_sha_handle_serv_final_msg(CSTRING msg)
{
	STRING	 decoded_msg = NULL;
	bool	 signature_ok = false;

	try {
		char		*cp;
		UCHARPTR	 signature;

		if ((decoded_msg = get_decoded_msg(msg, NULL)) == NULL) {
			throw std::runtime_error("unable to get decoded "
			    "message");
		} else if (strncmp(decoded_msg, "v=", 2) != STRINGS_MATCH) {
			throw std::runtime_error("expected server signature");
		}

		debug("%s: S: %s", __func__, decoded_msg);

		cp = decoded_msg;
		cp += 2;

		if ((signature =
		    reinterpret_cast<UCHARPTR>(get_decoded_msg(cp, NULL))) ==
		    NULL) {
			throw std::runtime_error("cannot decode server "
			    "signature!");
		}

		signature_ok = (memcmp(signature, signature_expected,
		    signature_expected_len) == 0);

		delete[] signature;
	} catch (const std::runtime_error &e) {
		delete[] decoded_msg;
		err_log(0, "%s: %s", __func__, e.what());
		return -1;
	}

	delete[] decoded_msg;

	free_and_null(&complete_nonce);

	return (signature_ok ? 0 : -1);
}
