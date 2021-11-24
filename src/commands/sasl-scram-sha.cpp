/* SASL auth mechanism SCRAM-SHA-256
   Copyright (C) 2019-2021 Markus Uhlin. All rights reserved.

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

#ifndef _lint
#include <random>
#endif
#include <stdexcept>

#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "../assertAPI.h"
#include "../config.h"
#include "../dataClassify.h"
#include "../errHand.h"
#include "../libUtils.h"
#include "../network.h"
#include "../strHand.h"
#include "../strdup_printf.h"

#include "sasl-scram-sha.h"

volatile bool g_sasl_scram_sha_got_first_msg = false;

static char	*complete_nonce = NULL;
static char	 nonce[64] = { '\0' };

static unsigned char	 signature_expected[EVP_MAX_MD_SIZE] = { '\0' };
static unsigned int	 signature_expected_len = 0;

/*lint -sem(get_decoded_msg, r_null) */
/*lint -sem(get_salted_password, r_null) */

static void
generate_and_store_nonce()
{
#ifndef _lint
    const char legal_index[] =
	"!\"#$%&'()*+-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
	"abcdefghijklmnopqrstuvwxyz{|}~";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, strlen(legal_index) - 1);

    for (size_t i = 0; i < ARRAY_SIZE(nonce); i++)
	nonce[i] = legal_index[dist(gen)];

    nonce[ARRAY_SIZE(nonce) - 1] = '\0';
    debug("generate_and_store_nonce: nonce: %s", nonce);
#endif
}

static const char *
get_encoded_msg(const char *source)
{
    static char encoded_msg[4096];

    memset(encoded_msg, 0, ARRAY_SIZE(encoded_msg));

    if (b64_encode(reinterpret_cast<const uint8_t *>(source), strlen(source),
	encoded_msg, ARRAY_SIZE(encoded_msg)) == -1)
	return "";

    return &encoded_msg[0];
}

/* C: n,,n=user,r=rOprNGfwEbeRWgbNEkqO */
int
sasl_scram_sha_send_client_first_msg(void)
{
    char	*msg      = NULL;
    const char	*username = Config("sasl_username");

    if (!is_valid_username(username))
	return -1;

    generate_and_store_nonce();

    msg = strdup_printf("n,,n=%s,r=%s", username, nonce);
    const char *encoded_msg = get_encoded_msg(msg);
    free(msg);

    if (strings_match(encoded_msg, "") ||
	net_send("AUTHENTICATE %s", encoded_msg) < 0)
	return -1;

    return 0;
}

/* C: c=biws,r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0,
      p=dHzbZapWIk4jUhN+Ute9ytag9zjfMHgsqmmiz7AndVQ= */
int
sasl_scram_sha_send_client_final_msg(const char *proof)
{
    std::string str("c=biws,r=");

    str.append(complete_nonce);
    str.append(",p=");
    str.append(proof);

    const size_t size = str.length() + 1;
    char *cli_final_msg = new char[size];

    if (sw_strcpy(cli_final_msg, str.c_str(), size) != 0 ||
	net_send("AUTHENTICATE %s", get_encoded_msg(cli_final_msg)) < 0) {
	delete[] cli_final_msg;
	return -1;
    }

    debug("sasl_scram_sha_send_client_final_msg: C: %s", cli_final_msg);
    delete[] cli_final_msg;
    return 0;
}

static char *
get_decoded_msg(const char *source, int *outlen)
{
    char *decoded_msg = NULL;
    int length_needed = b64_decode(source, NULL, 0);

    if (length_needed < 0)
	return NULL;
    if (outlen)
	*outlen = length_needed;

    length_needed += 1;
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
get_sfm_components(const char *msg, unsigned char **salt, int *saltlen,
		   int *iter)
{
    char	*decoded_msg = NULL;
    bool	 ok = false;

    *salt = NULL;
    *saltlen = 0;
    *iter = PKCS5_DEFAULT_ITER;

    try {
	if ((decoded_msg = get_decoded_msg(msg, NULL)) == NULL)
	    throw std::runtime_error("unable to get decoded message");

	debug("get_sfm_components: S: %s", decoded_msg);
	char *cp = decoded_msg;

	if (strncmp(cp, "r=", 2) != STRINGS_MATCH)
	    throw std::runtime_error("expected nonce");

	cp += 2;

	if (strncmp(cp, nonce, strlen(nonce)) != STRINGS_MATCH)
	    throw std::runtime_error("nonce mismatch");

	size_t n = strcspn(cp, ",");
	cp[n] = '\0';
	free(complete_nonce);
	complete_nonce = sw_strdup(cp);
	cp[n] = ',';

	if ((cp = strstr(cp, ",s=")) == NULL)
	    throw std::runtime_error("no base64-encoded salt");

	cp += 3;
	n = strcspn(cp, ",");
	cp[n] = '\0';
	char *b64salt = sw_strdup(cp);
	cp[n] = ',';
	*salt =
	    reinterpret_cast<unsigned char *>(get_decoded_msg(b64salt, saltlen));
	free(b64salt);

	if (*salt == NULL)
	    throw std::runtime_error("unable to decode salt");
	else if ((cp = strstr(cp, ",i=")) == NULL)
	    throw std::runtime_error("no iteration count");

	cp += 3;

	if (!is_numeric(cp))
	    throw std::runtime_error("iteration count not numeric");

	*iter = (int) strtol(cp, NULL, 10);
	ok = true;
    } catch (std::runtime_error &e) {
	delete[] *salt;
	*salt = NULL;
	*saltlen = 0;
	*iter = PKCS5_DEFAULT_ITER;
	err_log(0, "get_sfm_components: %s", e.what());
    }

    if (decoded_msg)
	delete[] decoded_msg;
    return ok ? 0 : -1;
}

/*
 * SaltedPassword: Hi(Normalize(password), salt, i)
 */
static unsigned char *
get_salted_password(const unsigned char *salt, int saltlen, int iter,
		    int *outsize)
{
    unsigned char *out = NULL;

    try {
	if (*outsize = EVP_MD_size(EVP_sha256()), *outsize < 0)
	    throw std::runtime_error("message digest size negative");

	out = new unsigned char[*outsize];
	const char *pass = config_get_normalized_sasl_password();

	if (pass == NULL)
	    throw std::runtime_error("unable to get normalized sasl password");
	else if (!PKCS5_PBKDF2_HMAC(pass, -1, salt, saltlen, iter, EVP_sha256(),
	    *outsize, out))
	    throw std::runtime_error("unable to get salted password");
    } catch (std::runtime_error &e) {
	*outsize = 0;
	delete[] out;
	err_log(0, "get_salted_password: %s", e.what());
	return NULL;
    }

    return out;
}

struct digest_context {
    unsigned char	*key;
    int			 key_len;
    const unsigned char	*d;
    size_t		 n;
    unsigned char	 md[EVP_MAX_MD_SIZE];
    unsigned int	 md_len;

    digest_context(unsigned char *key, int key_len, const unsigned char *d,
		   size_t n)
	{
	    this->key = key;
	    this->key_len = key_len;
	    this->d = d;
	    this->n = n;
	    memset(this->md, 0, ARRAY_SIZE(this->md));
	    this->md_len = 0;
	}
};

static int
get_digest(struct digest_context *ctx)
{
    if (HMAC(EVP_sha256(), ctx->key, ctx->key_len, ctx->d, ctx->n, ctx->md,
	     & (ctx->md_len)) == NULL)
	return -1;
    return 0;
}

static char *
get_client_first_msg_bare()
{
    std::string str("n=");

    str.append(Config("sasl_username"));
    str.append(",r=");
    str.append(nonce);

    const size_t size = str.length() + 1;
    char *msg_bare = new char[size];
    (void) sw_strcpy(msg_bare, str.c_str(), size);
    return msg_bare;
}

static char *
get_client_final_msg_wo_proof()
{
	char		*msg_wo_proof;
	size_t		 size;
	std::string	 str("c=biws,r=");

	(void) str.append(complete_nonce);

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
static unsigned char *
get_auth_msg(const char *b64msg, size_t *auth_msg_len)
{
	char	*msg_bare, *msg_wo_proof, *serv_first_msg;
	char	*out;

	msg_bare = get_client_first_msg_bare();
	msg_wo_proof = get_client_final_msg_wo_proof();
	serv_first_msg = get_decoded_msg(b64msg, NULL);

	out = strdup_printf("%s,%s,%s", msg_bare, serv_first_msg, msg_wo_proof);

	delete[] msg_bare;
	delete[] msg_wo_proof;
	delete[] serv_first_msg;

	*auth_msg_len = strlen(out);
	return reinterpret_cast<unsigned char *>(out);
}

int
sasl_scram_sha_handle_serv_first_msg(const char *msg)
{
	char		 proof[EVP_MAX_MD_SIZE] = { '\0' };
	int		 iter = PKCS5_DEFAULT_ITER;
	int		 passwdlen = 0;
	int		 saltlen = 0;
	size_t		 auth_msg_len;
	unsigned char	*auth_msg;
	unsigned char	*pass = NULL;
	unsigned char	*salt = NULL;
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

	struct digest_context client_key(pass, passwdlen,
	    reinterpret_cast<const unsigned char *>("Client Key"), 10);
	struct digest_context server_key(pass, passwdlen,
	    reinterpret_cast<const unsigned char *>("Server Key"), 10);

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
	for (unsigned int i = 0; i < MIN(client_key.md_len,
	    client_signature.md_len); i++)
		proof[i] = client_key.md[i] ^ client_signature.md[i];

	return sasl_scram_sha_send_client_final_msg(get_encoded_msg(proof));
}

/*
 * S: v=6rriTRBi23WpRR/wtup+mMhUZUn/dB5nLTJRsjl95G4=
 */
int
sasl_scram_sha_handle_serv_final_msg(const char *msg)
{
	bool	 signature_ok = false;
	char	*decoded_msg = NULL;

	try {
		char		*cp;
		unsigned char	*signature;

		if ((decoded_msg = get_decoded_msg(msg, NULL)) == NULL) {
			throw std::runtime_error("unable to get decoded "
			    "message");
		} else if (strncmp(decoded_msg, "v=", 2) != STRINGS_MATCH) {
			throw std::runtime_error("expected server signature");
		}

		debug("sasl_scram_sha_handle_serv_final_msg: S: %s",
		    decoded_msg);

		cp = decoded_msg;
		cp += 2;

		if ((signature =
		    reinterpret_cast<unsigned char *>(get_decoded_msg(cp,
		    NULL))) == NULL) {
			throw std::runtime_error("cannot decode server "
			    "signature!");
		}

		signature_ok = (memcmp(signature, signature_expected,
		    signature_expected_len) == 0);

		delete[] signature;
	} catch (const std::runtime_error& e) {
		delete[] decoded_msg;
		err_log(0, "sasl_scram_sha_handle_serv_final_msg: %s",
		    e.what());
		return -1;
	}

	delete[] decoded_msg;

	free_and_null(&complete_nonce);

	return (signature_ok ? 0 : -1);
}
